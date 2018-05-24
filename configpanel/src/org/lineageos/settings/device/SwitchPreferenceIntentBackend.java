/*
 * Copyright (C) 2018 The LineageOS Project
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

package org.lineageos.settings.device;

import android.content.Context;
import android.content.Intent;
import android.os.UserHandle;

import org.lineageos.internal.util.FileUtils;
import org.lineageos.internal.util.PackageManagerUtils;

public class SwitchPreferenceIntentBackend extends SwitchPreferenceBackend {

    private Boolean mValid;
    private String mIntentName;
    private String mIntentExtra;
    private String mPackageCheck;

    public SwitchPreferenceIntentBackend(String packageCheck, String intentName,
            String intentExtra, Boolean defaultValue) {
        super(defaultValue);
        mPackageCheck = packageCheck;
        mIntentName = intentName;
        mIntentExtra = intentExtra;
        updateValidity();
    }

    @Override
    public void setValue(Boolean value) {
        final Intent intent = new Intent(mIntentName);
        intent.putExtra(mIntentExtra, value);
        context.sendBroadcastAsUser(intent, UserHandle.CURRENT);
    }

    @Override
    public Boolean isValid() {
        return mValid;
    }

    private void updateValidity() {
        mValid = PackageManagerUtils.isAppInstalled(getContext(), mPackageCheck);
    }
}
