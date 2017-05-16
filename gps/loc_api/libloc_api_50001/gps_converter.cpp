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

#include <gps_converter.h>

void A2Q_DerEncodedCertificate(const DerEncodedCertificate& in, LocDerEncodedCertificate& out) {
    memset(&out, 0, sizeof(LocDerEncodedCertificate));
    out.length = in.length;
    out.data = in.data;
}

void Q2A_GpsLocation(const LocGpsLocation& in, GpsLocation& out) {
    memset(&out, 0, sizeof(GpsLocation));
    out.size = sizeof(GpsLocation);
    out.flags = (LocGpsLocationFlags)in.flags;
    out.latitude = in.latitude;
    out.longitude = in.longitude;
    out.altitude = in.altitude;
    out.speed = in.speed;
    out.bearing = in.bearing;
    out.accuracy = in.accuracy;
    out.timestamp = Q2A_GpsUtcTime(in.timestamp);
}

void Q2A_GpsSvInfo(const LocGpsSvInfo& in, GpsSvInfo& out) {
    memset(&out, 0, sizeof(GpsSvInfo));
    out.size = sizeof(GpsSvInfo);
    out.prn = in.prn;
    out.snr = in.snr;
    out.elevation = in.elevation;
    out.azimuth = in.azimuth;
}

void Q2A_GpsSvStatus(const LocGpsSvStatus& in, GpsSvStatus& out) {
    memset(&out, 0, sizeof(GpsSvStatus));
    out.size = sizeof(GpsSvStatus);
    out.num_svs = in.num_svs;
    int len = LOC_GPS_MAX_SVS < GPS_MAX_SVS ? LOC_GPS_MAX_SVS : GPS_MAX_SVS;
    for (int i = 0; i < len; i++) {
        Q2A_GpsSvInfo(in.sv_list[i], out.sv_list[i]);
    }
    out.ephemeris_mask = in.ephemeris_mask;
    out.almanac_mask = in.almanac_mask;
    out.used_in_fix_mask = in.used_in_fix_mask;
}

void Q2A_GnssSvInfo(const LocGnssSvInfo& in, GnssSvInfo& out) {
    memset(&out, 0, sizeof(GnssSvInfo));
    out.size = sizeof(GnssSvInfo);
    out.svid = in.svid;
    out.constellation = (GnssConstellationType)in.constellation;
    out.c_n0_dbhz = in.c_n0_dbhz;
    out.elevation = in.elevation;
    out.azimuth = in.azimuth;
    out.flags = (GnssSvFlags)in.flags;
}

void Q2A_GnssSvStatus(const LocGnssSvStatus& in, GnssSvStatus& out) {
    memset(&out, 0, sizeof(GnssSvStatus));
    out.size = sizeof(GnssSvStatus);
    out.num_svs = in.num_svs;
    int len = LOC_GNSS_MAX_SVS < GNSS_MAX_SVS ? LOC_GNSS_MAX_SVS : GNSS_MAX_SVS;
    for (int i = 0; i < len; i++) {
        Q2A_GnssSvInfo(in.gnss_sv_list[i], out.gnss_sv_list[i]);
    }
}

void Q2A_GpsNiNotification(const LocGpsNiNotification& in, GpsNiNotification& out) {
    memset(&out, 0, sizeof(GpsNiNotification));
    out.size = sizeof(GpsNiNotification);
    out.notification_id = in.notification_id;
    out.ni_type = (GpsNiType)in.ni_type;
    out.notify_flags = (GpsNiNotifyFlags)in.notify_flags;
    out.timeout = in.timeout;
    out.default_response = (GpsUserResponseType)in.default_response;
    int len = LOC_GPS_NI_SHORT_STRING_MAXLEN < GPS_NI_SHORT_STRING_MAXLEN
        ? LOC_GPS_NI_SHORT_STRING_MAXLEN : GPS_NI_SHORT_STRING_MAXLEN;
    for (int i = 0; i < len; i++) {
        out.requestor_id[i] = in.requestor_id[i];
    }
    len = LOC_GPS_NI_LONG_STRING_MAXLEN < GPS_NI_LONG_STRING_MAXLEN
        ? LOC_GPS_NI_LONG_STRING_MAXLEN : GPS_NI_LONG_STRING_MAXLEN;
    for (int i = 0; i < len; i++) {
        out.text[i] = in.text[i];
    }
    out.requestor_id_encoding = (GpsNiEncodingType)in.requestor_id_encoding;
    out.text_encoding = (GpsNiEncodingType)in.text_encoding;
    len = LOC_GPS_NI_LONG_STRING_MAXLEN < GPS_NI_LONG_STRING_MAXLEN
        ? LOC_GPS_NI_LONG_STRING_MAXLEN : GPS_NI_LONG_STRING_MAXLEN;
    for (int i = 0; i < len; i++) {
        out.extras[i] = in.extras[i];
    }
}

void Q2A_GpsStatus(const LocGpsStatus& in, GpsStatus& out) {
    memset(&out, 0, sizeof(GpsStatus));
    out.size = sizeof(GpsStatus);
    out.status = (GpsStatusValue)in.status;
}

void Q2A_GnssSystemInfo(const LocGnssSystemInfo& in, GnssSystemInfo& out) {
    memset(&out, 0, sizeof(GnssSystemInfo));
    out.size = sizeof(GnssSystemInfo);
    out.year_of_hw = in.year_of_hw;
}

void Q2A_AGpsStatus(const LocAGpsStatus& in, AGpsStatus& out) {
    memset(&out, 0, sizeof(AGpsStatus));
    out.size = sizeof(AGpsStatus);
    out.type = (AGpsType)in.type;
    out.status = (AGpsStatusValue)in.status;
    out.ipaddr = in.ipaddr;
    memcpy(&out.addr, &in.addr, sizeof(sockaddr_storage));
}

void Q2A_GpsMeasurement(const LocGpsMeasurement& in, GpsMeasurement& out) {
    memset(&out, 0, sizeof(GpsMeasurement));
    out.size = sizeof(GpsMeasurement);
    out.flags = (GpsMeasurementFlags)in.flags;
    out.prn = in.prn;
    out.time_offset_ns = in.time_offset_ns;
    out.state = (GpsMeasurementState)in.state;
    out.received_gps_tow_ns = in.received_gps_tow_ns;
    out.received_gps_tow_uncertainty_ns = in.received_gps_tow_uncertainty_ns;
    out.c_n0_dbhz = in.c_n0_dbhz;
    out.pseudorange_rate_mps = in.pseudorange_rate_mps;
    out.pseudorange_rate_uncertainty_mps = in.pseudorange_rate_uncertainty_mps;
    out.accumulated_delta_range_state = (GpsAccumulatedDeltaRangeState)in.accumulated_delta_range_state;
    out.accumulated_delta_range_m = in.accumulated_delta_range_m;
    out.accumulated_delta_range_uncertainty_m = in.accumulated_delta_range_uncertainty_m;
    out.pseudorange_m = in.pseudorange_m;
    out.pseudorange_uncertainty_m = in.pseudorange_uncertainty_m;
    out.code_phase_chips = in.code_phase_chips;
    out.code_phase_uncertainty_chips = in.code_phase_uncertainty_chips;
    out.carrier_frequency_hz = in.carrier_frequency_hz;
    out.carrier_cycles = in.carrier_cycles;
    out.carrier_phase = in.carrier_phase;
    out.carrier_phase_uncertainty = in.carrier_phase_uncertainty;
    out.loss_of_lock = (GpsLossOfLock)in.loss_of_lock;
    out.bit_number = in.bit_number;
    out.time_from_last_bit_ms = in.time_from_last_bit_ms;
    out.doppler_shift_hz = in.doppler_shift_hz;
    out.doppler_shift_uncertainty_hz = in.doppler_shift_uncertainty_hz;
    out.multipath_indicator = (GpsMultipathIndicator)in.multipath_indicator;
    out.snr_db = in.snr_db;
    out.elevation_deg = in.elevation_deg;
    out.elevation_uncertainty_deg = in.elevation_uncertainty_deg;
    out.azimuth_deg = in.azimuth_deg;
    out.azimuth_uncertainty_deg = in.azimuth_uncertainty_deg;
    out.used_in_fix = in.used_in_fix;
}

void Q2A_GpsClock(const LocGpsClock& in, GpsClock& out) {
    memset(&out, 0, sizeof(GpsClock));
    out.size = sizeof(GpsClock);
    out.flags = (GpsClockFlags)in.flags;
    out.leap_second = in.leap_second;
    out.type = (GpsClockType)in.type;
    out.time_ns = in.time_ns;
    out.time_uncertainty_ns = in.time_uncertainty_ns;
    out.full_bias_ns = in.full_bias_ns;
    out.bias_ns = in.bias_ns;
    out.bias_uncertainty_ns = in.bias_uncertainty_ns;
    out.drift_nsps = in.drift_nsps;
    out.drift_uncertainty_nsps = in.drift_uncertainty_nsps;
}

void Q2A_GpsData(const LocGpsData& in, GpsData& out) {
    memset(&out, 0, sizeof(GpsData));
    out.size = sizeof(GpsData);
    out.measurement_count = in.measurement_count;
    int len = LOC_GPS_MAX_MEASUREMENT < GPS_MAX_MEASUREMENT
        ? LOC_GPS_MAX_MEASUREMENT : GPS_MAX_MEASUREMENT;
    for (int i = 0; i < len; i++) {
        Q2A_GpsMeasurement(in.measurements[i], out.measurements[i]);
    }
    Q2A_GpsClock(in.clock, out.clock);
}

void Q2A_GnssMeasurement(const LocGnssMeasurement& in, GnssMeasurement& out) {
    memset(&out, 0, sizeof(GnssMeasurement));
    out.size = sizeof(GnssMeasurement);
    out.flags = (GpsMeasurementFlags)in.flags;
    out.svid = in.svid;
    out.constellation = (GnssConstellationType)in.constellation;
    out.time_offset_ns = in.time_offset_ns;
    out.state = (GnssMeasurementState)in.state;
    out.received_sv_time_in_ns = in.received_sv_time_in_ns;
    out.received_sv_time_uncertainty_in_ns = in.received_sv_time_uncertainty_in_ns;
    out.c_n0_dbhz = in.c_n0_dbhz;
    out.pseudorange_rate_mps = in.pseudorange_rate_mps;
    out.pseudorange_rate_uncertainty_mps = in.pseudorange_rate_uncertainty_mps;
    out.accumulated_delta_range_state = (GnssAccumulatedDeltaRangeState)in.accumulated_delta_range_state;
    out.accumulated_delta_range_m = in.accumulated_delta_range_m;
    out.accumulated_delta_range_uncertainty_m = in.accumulated_delta_range_uncertainty_m;
    out.carrier_frequency_hz = in.carrier_frequency_hz;
    out.carrier_cycles = in.carrier_cycles;
    out.carrier_phase = in.carrier_phase;
    out.carrier_phase_uncertainty = in.carrier_phase_uncertainty;
    out.multipath_indicator = (GnssMultipathIndicator)in.multipath_indicator;
    out.snr_db = in.snr_db;
}

void Q2A_GnssClock(const LocGnssClock& in, GnssClock& out) {
    memset(&out, 0, sizeof(GnssClock));
    out.size = sizeof(GnssClock);
    out.flags = (GnssClockFlags)in.flags;
    out.leap_second = in.leap_second;
    out.time_ns = in.time_ns;
    out.time_uncertainty_ns = in.time_uncertainty_ns;
    out.full_bias_ns = in.full_bias_ns;
    out.bias_ns = in.bias_ns;
    out.bias_uncertainty_ns = in.bias_uncertainty_ns;
    out.drift_nsps = in.drift_nsps;
    out.drift_uncertainty_nsps = in.drift_uncertainty_nsps;
    out.hw_clock_discontinuity_count = in.hw_clock_discontinuity_count;
}

void Q2A_GnssData(const LocGnssData& in, GnssData& out) {
    memset(&out, 0, sizeof(GnssData));
    out.size = sizeof(GnssData);
    out.measurement_count = in.measurement_count;
    int len = LOC_GNSS_MAX_MEASUREMENT < GNSS_MAX_MEASUREMENT
        ? LOC_GNSS_MAX_MEASUREMENT : GNSS_MAX_MEASUREMENT;
    for (int i = 0; i < len; i++) {
        Q2A_GnssMeasurement(in.measurements[i], out.measurements[i]);
    }
    Q2A_GnssClock(in.clock, out.clock);
}
