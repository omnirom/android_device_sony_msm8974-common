#
# Copyright (C) 2015 The CyanogenMod Project
# Copyright (C) 2015 The OmniRom Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

gps_conf_dir := $(LOCAL_PATH)
gps_debug_conf := gps_debug.conf
gps_conf := gps.conf

$(gps_conf_dir)/$(gps_debug_conf):
		ln -sf $(gps_conf) $(TARGET_OUT_ETC)/$(gps_debug_conf)

ALL_DEFAULT_INSTALLED_MODULES += $(gps_conf_dir)/$(gps_debug_conf)

include $(call first-makefiles-under,$(call my-dir))
