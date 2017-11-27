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

# inherit from BoardConfigCommon.mk
include device/sony/msm8974-common/BoardConfigCommon.mk

# For omni use stock based kernel
TARGET_KERNEL_SOURCE := kernel/sony/msm8974

# Prevent building the unified kernel
BUILD_KERNEL := false

TARGET_INIT_VENDOR_LIB := libinit_msm

# Build embeded GPS HAL
USE_DEVICE_SPECIFIC_GPS := true
