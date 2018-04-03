/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <cutils/uevent.h>
#include <errno.h>
#include <sys/poll.h>
#include <pthread.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <stdbool.h>

#define LOG_TAG "PowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#include "power-set.h"
#include "utils.h"
#include "metadata-defs.h"
#include "hint-data.h"
#include "performance.h"
#include "power-common.h"

#define STATE_ON "state=1"
#define STATE_OFF "state=0"
#define STATE_HDR_ON "state=2"
#define STATE_HDR_OFF "state=3"

#define MAX_LENGTH         50

#define debug 0

#define UEVENT_MSG_LEN 1024
#define TOTAL_CPUS 4
#define RETRY_TIME_CHANGING_FREQ 20
#define SLEEP_USEC_BETWN_RETRY 200
#define LOW_POWER_MAX_FREQ "729600"
#define LOW_POWER_MIN_FREQ "300000"
#define NORMAL_MAX_FREQ "2265600"
#define UEVENT_STRING "online@/devices/system/cpu/"

/* RPM runs at 19.2Mhz. Divide by 19200 for msec */
/*
 * TODO: Control those values
 */
#define RPM_CLK 19200
#define USINSEC 1000000L
#define NSINUS 1000L

static int client_sockfd;
static struct sockaddr_un client_addr;

static struct pollfd pfd;
static char *cpu_path_min[] = {
    "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq",
    "/sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq",
    "/sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq",
    "/sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq",
};
static char *cpu_path_max[] = {
    "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq",
    "/sys/devices/system/cpu/cpu1/cpufreq/scaling_max_freq",
    "/sys/devices/system/cpu/cpu2/cpufreq/scaling_max_freq",
    "/sys/devices/system/cpu/cpu3/cpufreq/scaling_max_freq",
};
static bool freq_set[TOTAL_CPUS];
static bool low_power_mode = false;
static pthread_mutex_t low_power_mode_lock = PTHREAD_MUTEX_INITIALIZER;

bool display_boost = false;
static int saved_interactive_mode = -1;

//interaction boost global variables
static pthread_mutex_t s_interaction_lock = PTHREAD_MUTEX_INITIALIZER;
static struct timespec s_previous_boost_timespec;


void set_feature(__attribute__((unused))struct power_module *module, feature_t feature, int state)
{
#ifdef TAP_TO_WAKE_NODE
    if (feature == POWER_FEATURE_DOUBLE_TAP_TO_WAKE) {
            ALOGI("Double tap to wake is %s.", state ? "enabled" : "disabled");
            sysfs_write(TAP_TO_WAKE_NODE, state ? "1" : "0");
        return;
    }
#endif
}

long long calc_timespan_us(struct timespec start, struct timespec end) {
    long long diff_in_us = 0;
    diff_in_us += (end.tv_sec - start.tv_sec) * USINSEC;
    diff_in_us += (end.tv_nsec - start.tv_nsec) / NSINUS;
    return diff_in_us;
}

int __attribute__ ((weak)) set_interactive_override(__attribute__((unused)) struct power_module *module, __attribute__((unused)) int on)
{
    return HINT_NONE;
}

static void power_set_interactive(struct power_module *module, int on)
{
    char governor[80];
    int rc = 255;

    if (set_interactive_override(module, on) == HINT_HANDLED) {
        return;
    }
    ALOGI("Got set_interactive hint");

    if (get_scaling_governor(governor, sizeof(governor)) == -1) {
        ALOGE("Can't obtain scaling governor.");
        return;
    }

    if (!on) {
        rc = set_interactive_off(governor, saved_interactive_mode); 
    } else {
        rc = set_interactive_on(governor, saved_interactive_mode);
    }

    saved_interactive_mode = !!on;

    ALOGV("%s %s", __func__, (on ? "ON" : "OFF"));
#if (debug)
    ALOGE("%s TODO", __func__);
#endif
}

static void process_video_encode_hint(void *metadata)
{
#if (debug)
    ALOGE("%s TODO", __func__);
#endif
}

static int process_interaction_hint(int lock_handle, void *data) {
    int duration = 500, duration_hint = 0;
    static struct timespec s_previous_boost_timespec;
    struct timespec cur_boost_timespec;
    long long elapsed_time;

    if (data) {
        duration_hint = *((int *)data);
    }

    duration = duration_hint > 0 ? duration_hint : 500;

    clock_gettime(CLOCK_MONOTONIC, &cur_boost_timespec);
    elapsed_time = calc_timespan_us(s_previous_boost_timespec, cur_boost_timespec);
    if (elapsed_time > 750000) {
        elapsed_time = 750000;
    }
    // don't hint if it's been less than 250ms since last boost
    // also detect if we're doing anything resembling a fling
    // support additional boosting in case of flings
    else if (elapsed_time < 250000 && duration <= 750) {
        return lock_handle;
    }

    s_previous_boost_timespec = cur_boost_timespec;

    int resources[] = { (duration >= 2000 ? CPUS_ONLINE_MIN_3 : CPUS_ONLINE_MIN_2),
            0x20F, 0x30F, 0x40F, 0x50F };

    if (duration) {
        lock_handle = interaction(lock_handle, duration, ARRAY_SIZE(resources),
                                  resources);
    }

    return lock_handle;
}

static void process_low_power_hint(void *data) {
    int cpu, ret;
    bool low_power_mode;

    pthread_mutex_lock(&low_power_mode_lock);
    if (data) {
        low_power_mode = true;
        for (cpu = 0; cpu < TOTAL_CPUS; cpu++) {
            sysfs_write(cpu_path_min[cpu], LOW_POWER_MIN_FREQ);
            ret = sysfs_write(cpu_path_max[cpu], LOW_POWER_MAX_FREQ);
            if (!ret) {
                freq_set[cpu] = true;
            }
        }
        // reduces the refresh rate
        system("service call SurfaceFlinger 1016");
    } else {
        low_power_mode = false;
        for (cpu = 0; cpu < TOTAL_CPUS; cpu++) {
            ret = sysfs_write(cpu_path_max[cpu], NORMAL_MAX_FREQ);
            if (!ret) {
                freq_set[cpu] = false;
            }
        }
        // restores the refresh rate
        system("service call SurfaceFlinger 1017");
    }
    pthread_mutex_unlock(&low_power_mode_lock);
}

static int process_launch_hint(int lock_handle) {
    int duration = 2000;
    int resources[] = { CPUS_ONLINE_MIN_3,
        CPU0_MIN_FREQ_TURBO_MAX, CPU1_MIN_FREQ_TURBO_MAX,
        CPU2_MIN_FREQ_TURBO_MAX, CPU3_MIN_FREQ_TURBO_MAX };

    return interaction(lock_handle, duration, ARRAY_SIZE(resources), resources);
}

static void power_hint(__attribute__((unused)) struct power_module *module,
                      power_hint_t hint, __attribute__((unused)) void *data)
{
    static int lock_handle = 0;

    switch (hint) {
        case POWER_HINT_INTERACTION:
            process_interaction_hint(lock_handle, data);
            break;
            
        case POWER_HINT_VIDEO_ENCODE:
            process_video_encode_hint(data);
            break;

        case POWER_HINT_LOW_POWER:
            process_low_power_hint(data);
            break;
             
        case POWER_HINT_VSYNC:
            break;
            
        case POWER_HINT_DISABLE_TOUCH:
#if (debug)
            ALOGE("%s TODO: POWER_HINT_DISABLE_TOUCH", __func__);
#endif
            break;
        case POWER_HINT_LAUNCH:
            process_launch_hint(lock_handle);
            break;
        default:
#if (debug)
            ALOGE("%s TODO: hint id: %i", __func__, hint);
#endif
            break;
    }
}

static void power_init(__attribute__((unused)) struct power_module *module)
{
    ALOGI("%s", __func__);

    int fd;
    char buf[10] = {0};

    fd = open("/sys/devices/soc0/soc_id", O_RDONLY);
    if (fd >= 0) {
        if (read(fd, buf, sizeof(buf) - 1) == -1) {
            ALOGW("Unable to read soc_id");
        } else {
            int soc_id = atoi(buf);
            if (soc_id == 194 || (soc_id >= 208 && soc_id <= 218) || soc_id == 178) {
                display_boost = true;
                ALOGI("%s: Enabling display boost", __func__);
            } else {
                ALOGI("%s: Soc %d not in list, disabling display boost", __func__, soc_id);
            }
        }
        close(fd);
    }
}

static int power_open(const hw_module_t* module, const char* name,
                    hw_device_t** device)
{
    ALOGD("%s: enter; name=%s", __FUNCTION__, name);
    int retval = 0; /* 0 is ok; -1 is error */

    if (strcmp(name, POWER_HARDWARE_MODULE_ID) == 0) {
        power_module_t *dev = (power_module_t *)calloc(1,
                sizeof(power_module_t));

        if (dev) {
            /* Common hw_device_t fields */
            dev->common.tag = HARDWARE_DEVICE_TAG;
            dev->common.module_api_version = POWER_MODULE_API_VERSION_0_3;
            dev->common.hal_api_version = HARDWARE_HAL_API_VERSION;

            dev->init = power_init;
            dev->powerHint = power_hint;
            dev->setInteractive = power_set_interactive;
            dev->setFeature = set_feature;
            dev->get_number_of_platform_modes = NULL;
            dev->get_platform_low_power_stats = NULL;
            dev->get_voter_list = NULL;

            *device = (hw_device_t*)dev;
        } else
            retval = -ENOMEM;
    } else {
        retval = -EINVAL;
    }

    ALOGD("%s: exit %d", __FUNCTION__, retval);
    return retval;
}

static struct hw_module_methods_t power_module_methods = {
    .open = power_open,
};

struct power_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = POWER_MODULE_API_VERSION_0_2,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = POWER_HARDWARE_MODULE_ID,
        .name = "Sony 8974 Power HAL",
        .author = "The Android Open Source Project",
        .methods = &power_module_methods,
    },

    .init = power_init,
    .setInteractive = power_set_interactive,
    .powerHint = power_hint,
    .setFeature = set_feature,
};

