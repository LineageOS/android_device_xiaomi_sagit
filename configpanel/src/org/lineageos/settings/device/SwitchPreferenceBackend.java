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

public abstract class SwitchPreferenceBackend {
    protected Boolean mDefaultValue;
    protected Boolean mValue;

    public SwitchPreferenceBackend(Boolean defaultValue) {
        mDefaultValue = defaultValue;
        mValue = defaultValue;
    }

    public Boolean getValue() {
        return mValue;
    }

    public Boolean getDefaultValue() {
        return mDefaultValue;
    }

    public void setDefaultValue() {
        setValue(mDefaultValue);
    }

    public abstract void setValue(Boolean value);

    public abstract Boolean isValid();
}
