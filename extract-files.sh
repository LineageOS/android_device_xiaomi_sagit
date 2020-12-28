#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2020 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

function blob_fixup() {
    case "${1}" in
        vendor/etc/sensors/hals.conf)
            sed -i '/sensors.elliptic.so/d' "${2}"
            ;;
        vendor/lib64/com.fingerprints.extension@1.0.so)
            "${PATCHELF}" --remove-needed "android.hidl.base@1.0.so" "${2}"
            ;;
        vendor/lib64/vendor.goodix.hardware.fingerprintextension@1.0.so)
            "${PATCHELF}" --remove-needed "android.hidl.base@1.0.so" "${2}"
            ;;
    esac
}

# If we're being sourced by the common script that we called,
# stop right here. No need to go down the rabbit hole.
if [ "${BASH_SOURCE[0]}" != "${0}" ]; then
    return
fi

set -e

export DEVICE=sagit
export DEVICE_COMMON=msm8998-common
export VENDOR=xiaomi

"./../../${VENDOR}/${DEVICE_COMMON}/extract-files.sh" "$@"
