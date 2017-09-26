#
# Copyright (C) 2017 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# WARNING: Everything listed here will be built on ALL platforms,
# including x86, the emulator, and the SDK.  Modules must be uniquely
# named (liblights.tuna), and must build everywhere, or limit themselves
# to only building on ARM if they include assembly. Individual makefiles
# are responsible for having their own logic, for fine-grained control.

LOCAL_PATH := $(call my-dir)

ifneq ($(filter sagit chiron,$(TARGET_DEVICE)),)

include $(call all-makefiles-under,$(LOCAL_PATH))

# Create a link for the WCNSS config file
$(shell mkdir -p $(TARGET_OUT)/etc/firmware/wlan/qca_cld; \
    ln -sf /system/etc/wifi/WCNSS_qcom_cfg.ini \
	    $(TARGET_OUT)/etc/firmware/wlan/qca_cld/WCNSS_qcom_cfg.ini; \
    ln -sf /persist/wlan_mac.bin \
	    $(TARGET_OUT_ETC)/firmware/wlan/qca_cld/wlan_mac.bin)

# GPS symlinks
$(shell ln -s /system/vendor/etc/gps.conf $(TARGET_OUT_ETC)/gps.conf)

# RFS folder structure
$(shell rm -rf $(TARGET_OUT)/rfs/)

# MSM folders
$(shell mkdir -p $(TARGET_OUT)/rfs/msm/adsp/readonly)
$(shell mkdir -p $(TARGET_OUT)/rfs/msm/mpss/readonly)

$(shell ln -s /data/tombstones/lpass $(TARGET_OUT)/rfs/msm/adsp/ramdumps)
$(shell ln -s /persist/rfs/msm/adsp $(TARGET_OUT)/rfs/msm/adsp/readwrite)
$(shell ln -s /persist/rfs/shared $(TARGET_OUT)/rfs/msm/adsp/shared)
$(shell ln -s /persist/hlos_rfs/shared $(TARGET_OUT)/rfs/msm/adsp/hlos)
$(shell ln -s /firmware $(TARGET_OUT)/rfs/msm/adsp/readonly/firmware)

$(shell ln -s /data/tombstones/modem $(TARGET_OUT)/rfs/msm/mpss/ramdumps)
$(shell ln -s /persist/rfs/msm/mpss $(TARGET_OUT)/rfs/msm/mpss/readwrite)
$(shell ln -s /persist/rfs/shared $(TARGET_OUT)/rfs/msm/mpss/shared)
$(shell ln -s /persist/hlos_rfs/shared $(TARGET_OUT)/rfs/msm/mpss/hlos)
$(shell ln -s /firmware $(TARGET_OUT)/rfs/msm/mpss/readonly/firmware)

# MDM folders
$(shell mkdir -p $(TARGET_OUT)/rfs/mdm/adsp/readonly)
$(shell mkdir -p $(TARGET_OUT)/rfs/mdm/mpss/readonly)
$(shell mkdir -p $(TARGET_OUT)/rfs/mdm/sparrow/readonly)

$(shell ln -s /data/tombstones/lpass $(TARGET_OUT)/rfs/mdm/adsp/ramdumps)
$(shell ln -s /persist/rfs/mdm/adsp $(TARGET_OUT)/rfs/mdm/adsp/readwrite)
$(shell ln -s /persist/rfs/shared $(TARGET_OUT)/rfs/mdm/adsp/shared)
$(shell ln -s /persist/hlos_rfs/shared $(TARGET_OUT)/rfs/mdm/adsp/hlos)
$(shell ln -s /firmware $(TARGET_OUT)/rfs/mdm/adsp/readonly/firmware)

$(shell ln -s /data/tombstones/modem $(TARGET_OUT)/rfs/mdm/mpss/ramdumps)
$(shell ln -s /persist/rfs/mdm/mpss $(TARGET_OUT)/rfs/mdm/mpss/readwrite)
$(shell ln -s /persist/rfs/shared $(TARGET_OUT)/rfs/mdm/mpss/shared)
$(shell ln -s /persist/hlos_rfs/shared $(TARGET_OUT)/rfs/mdm/mpss/hlos)
$(shell ln -s /firmware $(TARGET_OUT)/rfs/mdm/mpss/readonly/firmware)

$(shell ln -s /data/tombstones/sparrow $(TARGET_OUT)/rfs/mdm/sparrow/ramdumps)
$(shell ln -s /persist/rfs/mdm/sparrow $(TARGET_OUT)/rfs/mdm/sparrow/readwrite)
$(shell ln -s /persist/rfs/shared $(TARGET_OUT)/rfs/mdm/sparrow/shared)
$(shell ln -s /persist/hlos_rfs/shared $(TARGET_OUT)/rfs/mdm/sparrow/hlos)
$(shell ln -s /firmware $(TARGET_OUT)/rfs/mdm/sparrow/readonly/firmware)

# APQ folders
$(shell mkdir -p $(TARGET_OUT)/rfs/apq/gnss/readonly)

$(shell ln -s /data/tombstones/modem $(TARGET_OUT)/rfs/apq/gnss/ramdumps)
$(shell ln -s /persist/rfs/apq/gnss $(TARGET_OUT)/rfs/apq/gnss/readwrite)
$(shell ln -s /persist/rfs/shared $(TARGET_OUT)/rfs/apq/gnss/shared)
$(shell ln -s /persist/hlos_rfs/shared $(TARGET_OUT)/rfs/apq/gnss/hlos)
$(shell ln -s /firmware $(TARGET_OUT)/rfs/apq/gnss/readonly/firmware)
# END RFS folder structure

endif
