#!/sbin/sh
#
# Copyright (C) 2017 The MoKee Open Source Project
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

ENCRYPTION="/data/unencrypted/mode"

# Data partition is just formatted and empty
if [[ ! -d "/data/data" ]]; then
    return 0
fi

# No encryption mode file
if [[ ! -f $ENCRYPTION ]]; then
    return 1
fi

# Encryption is not FBE
if [[ "$(cat $ENCRYPTION)" != "ice" ]]; then
    return 2
fi

return 0
