/*
 * Copyright (C) 2019 The LineageOS Project
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

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>

#include "KeyDisabler.h"

namespace vendor {
namespace lineage {
namespace touch {
namespace V1_0 {
namespace implementation {

constexpr const char kControlPath[] = "/proc/touchpanel/capacitive_keys_enable";
constexpr const char kFpcPath[] = "/sys/devices/soc/soc:fingerprint_fpc/enable_key_events";
constexpr const char kGoodixPath[] = "/sys/devices/soc/soc:fingerprint_goodix/enable_key_events";

KeyDisabler::KeyDisabler() {
    mHasKeyDisabler = !access(kControlPath, F_OK
            && !access(kFpcPath, F_OK) && !access(kGoodixPath, F_OK);
}

// Methods from ::vendor::lineage::touch::V1_0::IKeyDisabler follow.
Return<bool> KeyDisabler::isEnabled() {
    std::string controlBuf;
    std::string fpcBuf;
    std::string goodixBuf;

    if (!mHasKeyDisabler) return false;

    if (!android::base::ReadFileToString(kControlPath, &controlBuf)) {
        LOG(ERROR) << "Failed to read " << kControlPath;
        return false;
    }

    if (!android::base::ReadFileToString(kFpcPath, &fpcBuf)) {
        LOG(ERROR) << "Failed to read " << kFpcPath;
        return false;
    }

    if (!android::base::ReadFileToString(kGoodixPath, &goodixBuf)) {
        LOG(ERROR) << "Failed to read " << kGoodixPath;
        return false;
    }

    return std::stoi(android::base::Trim(controlBuf)) == 0
            || std::stoi(android::base::Trim(fpcBuf)) == 0
            || std::stoi(android::base::Trim(goodixBuf)) == 0;
}

Return<bool> KeyDisabler::setEnabled(bool enabled) {
    std::string buf = enabled ? "0" : "1";

    if (!mHasKeyDisabler) return false;

    if (!android::base::WriteStringToFile(buf, kControlPath)) {
        LOG(ERROR) << "Failed to write " << kControlPath;
        return false;
    }

    if (!android::base::WriteStringToFile(buf, kFpcPath)) {
        LOG(ERROR) << "Failed to write " << kFpcPath;
        return false;
    }

    if (!android::base::WriteStringToFile(buf, kGoodixPath)) {
        LOG(ERROR) << "Failed to write " << kGoodixPath;
        return false;
    }

    return true;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace touch
}  // namespace lineage
}  // namespace vendor
