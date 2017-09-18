# Copyright (C) 2014 The CyanogenMod Project
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

# cryptfs hw
# TODO: Readjust path after splitup of sony-common
TARGET_CRYPTFS_HW_PATH := device/sony/common/cryptfs_hw
PRODUCT_PACKAGES += \
    libcryptfs_hw

# Variant linking script
PRODUCT_COPY_FILES += \
    device/sony/msm8974-common/releasetools/updater.sh:utilities/updater.sh

# SELinux
PRODUCT_PROPERTY_OVERRIDES += \
    ro.build.selinux=1

# inherit from msm8974.mk
include device/sony/msm8974-common/msm8974.mk

# Omni custom config
$(call inherit-product, vendor/omni/config/common.mk)
