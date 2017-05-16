/* Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __GPS_CONVERTER_H__
#define __GPS_CONVERTER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <ctype.h>
#include <hardware/gps.h>
#include <gps_extended.h>

#define A2Q_GpsAidingData(in)           (LocGpsAidingData)in
#define A2Q_GpsUserResponseType(in)     (LocGpsUserResponseType)in
#define A2Q_GpsPositionRecurrence(in)   (LocGpsPositionRecurrence)in
#define A2Q_GpsUtcTime(in)              (LocGpsUtcTime)in
#define A2Q_GpsPositionMode(in)         (LocGpsPositionMode)in
#define A2Q_GpsPositionRecurrence(in)   (LocGpsPositionRecurrence)in
#define A2Q_ApnIpType(in)               (LocApnIpType)in
#define A2Q_AGpsType(in)                (LocAGpsType)in
#define A2Q_GpsPositionRecurrence(in)   (LocGpsPositionRecurrence)in

#define Q2A_GpsUtcTime(in)              (GpsUtcTime)in

void A2Q_DerEncodedCertificate(const DerEncodedCertificate& in, LocDerEncodedCertificate& out);

void Q2A_GpsLocation(const LocGpsLocation& in, GpsLocation& out);
void Q2A_GpsSvStatus(const LocGpsSvStatus& in, GpsSvStatus& out);
void Q2A_GnssSvStatus(const LocGnssSvStatus& in, GnssSvStatus& out);
void Q2A_GpsNiNotification(const LocGpsNiNotification& in, GpsNiNotification& out);
void Q2A_GpsStatus(const LocGpsStatus& in, GpsStatus& out);
void Q2A_GnssSystemInfo(const LocGnssSystemInfo& in, GnssSystemInfo& out);
void Q2A_AGpsStatus(const LocAGpsStatus& in, AGpsStatus& out);
void Q2A_GpsData(const LocGpsData& in, GpsData& out);
void Q2A_GnssData(const LocGnssData& in, GnssData& out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //__GPS_CONVERTER_H__
