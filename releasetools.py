# Copyright (C) 2009 The Android Open Source Project
# Copyright (c) 2011, The Linux Foundation. All rights reserved.
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

import hashlib
import common
import re

def FullOTA_Assertions(info):
  AddTrustZoneAssertion(info, info.input_zip)
  AddFileEncryptionAssertion(info)
  return

def IncrementalOTA_Assertions(info):
  AddTrustZoneAssertion(info, info.target_zip)
  AddFileEncryptionAssertion(info)
  return

def AddTrustZoneAssertion(info, input_zip):
  android_info = info.input_zip.read("OTA/android-info.txt")
  m = re.search(r'require\s+version-trustzone\s*=\s*(\S+)', android_info)
  if m:
    versions = m.group(1).split('|')
    if len(versions) and '*' not in versions:
      cmd = 'assert(sagit.verify_trustzone(' + ','.join(['"%s"' % tz for tz in versions]) + ') == "1");'
      info.script.AppendExtra(cmd)
  return

def AddFileEncryptionAssertion(info):
  info.script.AppendExtra('package_extract_file("install/bin/fbe_check.sh", "/tmp/fbe_check.sh");');
  info.script.AppendExtra('set_metadata("/tmp/fbe_check.sh", "uid", 0, "gid", 0, "mode", 0755);');
  info.script.AppendExtra('if !is_mounted("/data") then');
  info.script.Mount("/data");
  info.script.AppendExtra('endif;');
  info.script.AppendExtra('if run_program("/tmp/fbe_check.sh") != 0 then');
  info.script.AppendExtra('ui_print("===========================================");');
  info.script.AppendExtra('ui_print("|              !!! ERROR !!!               ");');
  info.script.AppendExtra('ui_print("|                                          ");');
  info.script.AppendExtra('ui_print("| File-based Encryption is required.       ");');
  info.script.AppendExtra('ui_print("|                                          ");');
  info.script.AppendExtra('ui_print("| Backup your data (including internal     ");');
  info.script.AppendExtra('ui_print("| storage) and format the data partition.  ");');
  info.script.AppendExtra('ui_print("|                                          ");');
  info.script.AppendExtra('ui_print("| Chinese: http://t.cn/R92kLm3             ");');
  info.script.AppendExtra('ui_print("| English: https://t.co/7jDlsf2y6v         ");');
  info.script.AppendExtra('ui_print("===========================================");');
  info.script.AppendExtra('abort("FBE check failed.");');
  info.script.AppendExtra('endif;');
  info.script.Unmount("/data");
