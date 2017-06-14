/*
 * Copyright (C) 2016 The CyanogenMod Project
 *               2017 The LineageOS Project
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

#define LOG_TAG "xiaomi_readmac"
#define LOG_NDEBUG 0

#include <cutils/log.h>

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#define MAC_ADDR_SIZE 6
#define WLAN_MAC_BIN "/persist/wlan_mac.bin"

#define LIB_QMINVAPI "libqminvapi.so"

static const char xiaomi_mac_prefix[] = { 0x34, 0x80, 0xb3 };

typedef int (*qmi_nv_read_wlan_mac_t)(char** mac);

static int check_wlan_mac_bin_file() {
    char content[1024];
    FILE* fp;

    fp = fopen(WLAN_MAC_BIN, "r");
    if (fp == NULL)
        return 0;

    memset(content, 0, sizeof(content));
    fread(content, 1, sizeof(content) - 1, fp);
    fclose(fp);

    if (strstr(content, "Intf0MacAddress") == NULL) {
        ALOGV("%s is missing Intf0MacAddress entry value", WLAN_MAC_BIN);
        return 0;
    }

    if (strstr(content, "Intf1MacAddress") == NULL) {
        ALOGV("%s is missing Intf1MacAddress entry value", WLAN_MAC_BIN);
        return 0;
    }

    return 1;
}

static int write_wlan_mac_bin_file(unsigned char wlan_addr[]) {
    FILE* fp;

    fp = fopen(WLAN_MAC_BIN, "w");
    if (fp == NULL)
        return 0;

    fprintf(fp, "Intf0MacAddress=%02X%02X%02X%02X%02X%02X\n", wlan_addr[0], wlan_addr[1],
            wlan_addr[2], wlan_addr[3], wlan_addr[4], wlan_addr[5]);
    fprintf(fp, "Intf1MacAddress=%02X%02X%02X%02X%02X%02X\n", wlan_addr[0], wlan_addr[1],
            wlan_addr[2], wlan_addr[3], wlan_addr[4], (unsigned char)(wlan_addr[5] + 1));
    fprintf(fp, "END\n");
    fclose(fp);

    return 1;
}

int main() {
    unsigned char wlan_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    char* nv_wlan_mac = NULL;
    int ret, i;

    // Store WLAN MAC address in the persist file, if needed
    if (check_wlan_mac_bin_file()) {
        ALOGV("%s already exists and is valid", WLAN_MAC_BIN);
        return 0;
    }

    void* handle = dlopen(LIB_QMINVAPI, RTLD_NOW);
    if (!handle) {
        ALOGE("%s", dlerror());
        goto generate_random;
    }

    qmi_nv_read_wlan_mac_t qmi_nv_read_wlan_mac =
        (qmi_nv_read_wlan_mac_t)dlsym(handle, "qmi_nv_read_wlan_mac");

    if (!qmi_nv_read_wlan_mac) {
        ALOGE("%s", dlerror());
        goto generate_random;
    }

    // Read WLAN MAC address from modem NV
    ret = qmi_nv_read_wlan_mac(&nv_wlan_mac);
    if (!nv_wlan_mac) {
        ALOGE("qmi_nv_read_wlan_mac error %d", ret);
        goto generate_random;
    }

    for (i = 0; i < MAC_ADDR_SIZE; i++) {
        wlan_addr[i] = nv_wlan_mac[i];
    }

    goto out;

generate_random:
    memcpy(wlan_addr, xiaomi_mac_prefix, sizeof(xiaomi_mac_prefix));

    // We don't need strong randomness, and if the NV is corrupted
    // any hardware values are suspect, so just seed it with the
    // current time
    srand(time(NULL));

    for (i = sizeof(xiaomi_mac_prefix) / sizeof(xiaomi_mac_prefix[0]); i < MAC_ADDR_SIZE; i++) {
        wlan_addr[i] = rand() % 255;
    }

    ALOGI("Using randomized MAC address");

out:
    if (handle)
        dlclose(handle);

    if (!write_wlan_mac_bin_file(wlan_addr)) {
        ALOGE("Failed to write %s", WLAN_MAC_BIN);
        return 1;
    }

    ALOGV("%s was successfully generated", WLAN_MAC_BIN);
    return 0;
}
