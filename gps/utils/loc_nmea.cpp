/* Copyright (c) 2012, 2016, The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundation nor the names of its
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

#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_nmea"
#include <loc_nmea.h>
#include <math.h>
#include <platform_lib_includes.h>

#define GLONASS_SV_ID_OFFSET 64
#define MAX_SATELLITES_IN_USE 12
typedef struct loc_nmea_sv_meta_s
{
    char talker[3];
    LocGnssConstellationType svType;
    uint32_t mask;
    uint32_t svCount;
    uint32_t svIdOffset;
} loc_nmea_sv_meta;

typedef struct loc_sv_cache_info_s
{
    uint32_t gps_used_mask;
    uint32_t glo_used_mask;
    uint32_t gal_used_mask;
    uint32_t gps_count;
    uint32_t glo_count;
    uint32_t gal_count;
    float hdop;
    float pdop;
    float vdop;
} loc_sv_cache_info;

static loc_sv_cache_info sv_cache_info;

/*===========================================================================
FUNCTION    loc_nmea_sv_meta_init

DESCRIPTION
   Init loc_nmea_sv_meta passed in

DEPENDENCIES
   NONE

RETURN VALUE
   Pointer to loc_nmea_sv_meta

SIDE EFFECTS
   N/A

===========================================================================*/
static loc_nmea_sv_meta* loc_nmea_sv_meta_init(loc_nmea_sv_meta& sv_meta,
                                               LocGnssConstellationType svType,
                                               bool needCombine)
{
    memset(&sv_meta, 0, sizeof(sv_meta));
    sv_meta.svType = svType;
    sv_meta.talker[0] = 'G';

    switch (svType)
    {
        case LOC_GNSS_CONSTELLATION_GPS:
            sv_meta.talker[1] = 'P';
            sv_meta.mask = sv_cache_info.gps_used_mask;
            sv_meta.svCount = sv_cache_info.gps_count;
            break;
        case LOC_GNSS_CONSTELLATION_GLONASS:
            sv_meta.talker[1] = 'L';
            sv_meta.mask = sv_cache_info.glo_used_mask;
            sv_meta.svCount = sv_cache_info.glo_count;
            // GLONASS SV ids are from 65-96
            sv_meta.svIdOffset = GLONASS_SV_ID_OFFSET;
            break;
        case LOC_GNSS_CONSTELLATION_GALILEO:
            sv_meta.talker[1] = 'A';
            sv_meta.mask = sv_cache_info.gal_used_mask;
            sv_meta.svCount = sv_cache_info.gal_count;
            break;
        default:
            LOC_LOGE("NMEA Error unknow constellation type: %d", svType);
            return NULL;
    }
    if (needCombine &&
                (sv_cache_info.gps_used_mask ? 1 : 0) +
                (sv_cache_info.glo_used_mask ? 1 : 0) +
                (sv_cache_info.gal_used_mask ? 1 : 0) > 1)
    {
        // If GPS, GLONASS, Galileo etc. are combined
        // to obtain the reported position solution,
        // talker shall be set to GN, to indicate that
        // the satellites are used in a combined solution
        sv_meta.talker[1] = 'N';
    }
    return &sv_meta;
}

/*===========================================================================
FUNCTION    loc_nmea_put_checksum

DESCRIPTION
   Generate NMEA sentences generated based on position report

DEPENDENCIES
   NONE

RETURN VALUE
   Total length of the nmea sentence

SIDE EFFECTS
   N/A

===========================================================================*/
static int loc_nmea_put_checksum(char *pNmea, int maxSize)
{
    uint8_t checksum = 0;
    int length = 0;
    if(NULL == pNmea)
        return 0;

    pNmea++; //skip the $
    while (*pNmea != '\0')
    {
        checksum ^= *pNmea++;
        length++;
    }

    // length now contains nmea sentence string length not including $ sign.
    int checksumLength = snprintf(pNmea,(maxSize-length-1),"*%02X\r\n", checksum);

    // total length of nmea sentence is length of nmea sentence inc $ sign plus
    // length of checksum (+1 is to cover the $ character in the length).
    return (length + checksumLength + 1);
}

/*===========================================================================
FUNCTION    loc_nmea_generate_GSA

DESCRIPTION
   Generate NMEA GSA sentences generated based on position report
   Currently below sentences are generated:
   - $GPGSA : GPS DOP and active SVs
   - $GLGSA : GLONASS DOP and active SVs
   - $GAGSA : GALILEO DOP and active SVs
   - $GNGSA : GNSS DOP and active SVs

DEPENDENCIES
   NONE

RETURN VALUE
   Number of SVs used

SIDE EFFECTS
   N/A

===========================================================================*/
static uint32_t loc_nmea_generate_GSA(const GpsLocationExtended &locationExtended,
                              char* sentence,
                              int bufSize,
                              loc_nmea_sv_meta* sv_meta_p,
                              std::vector<std::string> &nmeaArraystr)
{
    if (!sentence || bufSize <= 0 || !sv_meta_p)
    {
        LOC_LOGE("NMEA Error invalid arguments.");
        return 0;
    }

    char* pMarker = sentence;
    int lengthRemaining = bufSize;
    int length = 0;

    uint32_t svUsedCount = 0;
    uint32_t svUsedList[32] = {0};

    char fixType = '\0';

    const char* talker = sv_meta_p->talker;
    uint32_t svIdOffset = sv_meta_p->svIdOffset;
    uint32_t mask = sv_meta_p->mask;

    for (uint8_t i = 1; mask > 0 && svUsedCount < 32; i++)
    {
        if (mask & 1)
            svUsedList[svUsedCount++] = i + svIdOffset;
        mask = mask >> 1;
    }

    if (svUsedCount == 0 && LOC_GNSS_CONSTELLATION_GPS != sv_meta_p->svType)
        return 0;

    if (svUsedCount == 0)
        fixType = '1'; // no fix
    else if (svUsedCount <= 3)
        fixType = '2'; // 2D fix
    else
        fixType = '3'; // 3D fix

    // Start printing the sentence
    // Format: $--GSA,a,x,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,p.p,h.h,v.v*cc
    // a : Mode  : A : Automatic, allowed to automatically switch 2D/3D
    // x : Fixtype : 1 (no fix), 2 (2D fix), 3 (3D fix)
    // xx : 12 SV ID
    // p.p : Position DOP (Dilution of Precision)
    // h.h : Horizontal DOP
    // v.v : Vertical DOP
    // cc : Checksum value
    length = snprintf(pMarker, lengthRemaining, "$%sGSA,A,%c,", talker, fixType);

    if (length < 0 || length >= lengthRemaining)
    {
        LOC_LOGE("NMEA Error in string formatting");
        return 0;
    }
    pMarker += length;
    lengthRemaining -= length;

    // Add first 12 satellite IDs
    for (uint8_t i = 0; i < 12; i++)
    {
        if (i < svUsedCount)
            length = snprintf(pMarker, lengthRemaining, "%02d,", svUsedList[i]);
        else
            length = snprintf(pMarker, lengthRemaining, ",");

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return 0;
        }
        pMarker += length;
        lengthRemaining -= length;
    }

    // Add the position/horizontal/vertical DOP values
    if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_DOP)
    {   // dop is in locationExtended, (QMI)
        length = snprintf(pMarker, lengthRemaining, "%.1f,%.1f,%.1f",
                locationExtended.pdop,
                locationExtended.hdop,
                locationExtended.vdop);
    }
    else if (sv_cache_info.pdop > 0 && sv_cache_info.hdop > 0 && sv_cache_info.vdop > 0)
    {   // dop was cached from sv report (RPC)
        length = snprintf(pMarker, lengthRemaining, "%.1f,%.1f,%.1f",
                sv_cache_info.pdop,
                sv_cache_info.hdop,
                sv_cache_info.vdop);
    }
    else
    {   // no dop
        length = snprintf(pMarker, lengthRemaining, ",,");
    }

    /* Sentence is ready, add checksum and broadcast */
    length = loc_nmea_put_checksum(sentence, bufSize);
    nmeaArraystr.push_back(sentence);

    return svUsedCount;
}

/*===========================================================================
FUNCTION    loc_nmea_generate_GSV

DESCRIPTION
   Generate NMEA GSV sentences generated based on sv report
   Currently below sentences are generated:
   - $GPGSV: GPS Satellites in View
   - $GNGSV: GLONASS Satellites in View
   - $GAGSV: GALILEO Satellites in View

DEPENDENCIES
   NONE

RETURN VALUE
   NONE

SIDE EFFECTS
   N/A

===========================================================================*/
static void loc_nmea_generate_GSV(const LocGnssSvStatus &svStatus,
                              char* sentence,
                              int bufSize,
                              loc_nmea_sv_meta* sv_meta_p,
                              std::vector<std::string> &nmeaArraystr)
{
    if (!sentence || bufSize <= 0)
    {
        LOC_LOGE("NMEA Error invalid argument.");
        return;
    }

    char* pMarker = sentence;
    int lengthRemaining = bufSize;
    int length = 0;
    int sentenceCount = 0;
    int sentenceNumber = 1;
    int svNumber = 1;

    const char* talker = sv_meta_p->talker;
    uint32_t svIdOffset = sv_meta_p->svIdOffset;
    int svCount = sv_meta_p->svCount;


    if (svCount <= 0)
    {
        // no svs in view, so just send a blank $--GSV sentence
        snprintf(sentence, lengthRemaining, "$%sGSV,1,1,0,", talker);
        length = loc_nmea_put_checksum(sentence, bufSize);
        nmeaArraystr.push_back(sentence);
        return;
    }

    svNumber = 1;
    sentenceNumber = 1;
    sentenceCount = svCount / 4 + (svCount % 4 != 0);

    while (sentenceNumber <= sentenceCount)
    {
        pMarker = sentence;
        lengthRemaining = bufSize;

        length = snprintf(pMarker, lengthRemaining, "$%sGSV,%d,%d,%02d",
                talker, sentenceCount, sentenceNumber, svCount);

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        for (int i=0; (svNumber <= svStatus.num_svs) && (i < 4);  svNumber++)
        {
            if (sv_meta_p->svType == svStatus.gnss_sv_list[svNumber - 1].constellation)
            {
                length = snprintf(pMarker, lengthRemaining,",%02d,%02d,%03d,",
                        svStatus.gnss_sv_list[svNumber - 1].svid + svIdOffset,
                        (int)(0.5 + svStatus.gnss_sv_list[svNumber - 1].elevation), //float to int
                        (int)(0.5 + svStatus.gnss_sv_list[svNumber - 1].azimuth)); //float to int

                if (length < 0 || length >= lengthRemaining)
                {
                    LOC_LOGE("NMEA Error in string formatting");
                    return;
                }
                pMarker += length;
                lengthRemaining -= length;

                if (svStatus.gnss_sv_list[svNumber - 1].c_n0_dbhz > 0)
                {
                    length = snprintf(pMarker, lengthRemaining,"%02d",
                            (int)(0.5 + svStatus.gnss_sv_list[svNumber - 1].c_n0_dbhz)); //float to int

                    if (length < 0 || length >= lengthRemaining)
                    {
                        LOC_LOGE("NMEA Error in string formatting");
                        return;
                    }
                    pMarker += length;
                    lengthRemaining -= length;
                }

                i++;
            }

        }

        length = loc_nmea_put_checksum(sentence, bufSize);
        nmeaArraystr.push_back(sentence);
        sentenceNumber++;

    }  //while
}

/*===========================================================================
FUNCTION    loc_nmea_generate_pos

DESCRIPTION
   Generate NMEA sentences generated based on position report
   Currently below sentences are generated within this function:
   - $GPGSA : GPS DOP and active SVs
   - $GLGSA : GLONASS DOP and active SVs
   - $GAGSA : GALILEO DOP and active SVs
   - $GNGSA : GNSS DOP and active SVs
   - $--VTG : Track made good and ground speed
   - $--RMC : Recommended minimum navigation information
   - $--GGA : Time, position and fix related data

DEPENDENCIES
   NONE

RETURN VALUE
   0

SIDE EFFECTS
   N/A

===========================================================================*/
void loc_nmea_generate_pos(const UlpLocation &location,
                               const GpsLocationExtended &locationExtended,
                               unsigned char generate_nmea,
                               std::vector<std::string> &nmeaArraystr)
{
    ENTRY_LOG();
    time_t utcTime(location.gpsLocation.timestamp/1000);
    tm * pTm = gmtime(&utcTime);
    if (NULL == pTm) {
        LOC_LOGE("gmtime failed");
        return;
    }

    char sentence[NMEA_SENTENCE_MAX_LENGTH] = {0};
    char* pMarker = sentence;
    int lengthRemaining = sizeof(sentence);
    int length = 0;
    int utcYear = pTm->tm_year % 100; // 2 digit year
    int utcMonth = pTm->tm_mon + 1; // tm_mon starts at zero
    int utcDay = pTm->tm_mday;
    int utcHours = pTm->tm_hour;
    int utcMinutes = pTm->tm_min;
    int utcSeconds = pTm->tm_sec;
    int utcMSeconds = (location.gpsLocation.timestamp)%1000;

    if (generate_nmea) {
        char talker[3] = {'G', 'P', '\0'};
        uint32_t svUsedCount = 0;
        uint32_t count = 0;
        loc_nmea_sv_meta sv_meta;
        // -------------------
        // ---$GPGSA/$GNGSA---
        // -------------------

        count = loc_nmea_generate_GSA(locationExtended, sentence, sizeof(sentence),
                loc_nmea_sv_meta_init(sv_meta, LOC_GNSS_CONSTELLATION_GPS, true), nmeaArraystr);
        if (count > 0)
        {
            svUsedCount += count;
            talker[1] = sv_meta.talker[1];
        }

        // -------------------
        // ---$GLGSA/$GNGSA---
        // -------------------

        count = loc_nmea_generate_GSA(locationExtended, sentence, sizeof(sentence),
                loc_nmea_sv_meta_init(sv_meta, LOC_GNSS_CONSTELLATION_GLONASS, true), nmeaArraystr);
        if (count > 0)
        {
            svUsedCount += count;
            talker[1] = sv_meta.talker[1];
        }

        // -------------------
        // ---$GAGSA/$GNGSA---
        // -------------------

        count = loc_nmea_generate_GSA(locationExtended, sentence, sizeof(sentence),
                loc_nmea_sv_meta_init(sv_meta, LOC_GNSS_CONSTELLATION_GALILEO, true), nmeaArraystr);
        if (count > 0)
        {
            svUsedCount += count;
            talker[1] = sv_meta.talker[1];
        }

        // -------------------
        // ------$--VTG-------
        // -------------------

        pMarker = sentence;
        lengthRemaining = sizeof(sentence);

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_BEARING)
        {
            float magTrack = location.gpsLocation.bearing;
            if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_MAG_DEV)
            {
                float magTrack = location.gpsLocation.bearing - locationExtended.magneticDeviation;
                if (magTrack < 0.0)
                    magTrack += 360.0;
                else if (magTrack > 360.0)
                    magTrack -= 360.0;
            }

            length = snprintf(pMarker, lengthRemaining, "$%sVTG,%.1lf,T,%.1lf,M,", talker, location.gpsLocation.bearing, magTrack);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining, "$%sVTG,,T,,M,", talker);
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_SPEED)
        {
            float speedKnots = location.gpsLocation.speed * (3600.0/1852.0);
            float speedKmPerHour = location.gpsLocation.speed * 3.6;

            length = snprintf(pMarker, lengthRemaining, "%.1lf,N,%.1lf,K,", speedKnots, speedKmPerHour);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining, ",N,,K,");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (!(location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_LAT_LONG))
            // N means no fix
            length = snprintf(pMarker, lengthRemaining, "%c", 'N');
        else if (LOC_NAV_MASK_SBAS_CORRECTION_IONO & locationExtended.navSolutionMask)
            // D means differential
            length = snprintf(pMarker, lengthRemaining, "%c", 'D');
        else if (LOC_POS_TECH_MASK_SENSORS == locationExtended.tech_mask)
            // E means estimated (dead reckoning)
            length = snprintf(pMarker, lengthRemaining, "%c", 'E');
        else // A means autonomous
            length = snprintf(pMarker, lengthRemaining, "%c", 'A');

        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        // -------------------
        // ------$--RMC-------
        // -------------------

        pMarker = sentence;
        lengthRemaining = sizeof(sentence);

        length = snprintf(pMarker, lengthRemaining, "$%sRMC,%02d%02d%02d.%02d,A," ,
                          talker, utcHours, utcMinutes, utcSeconds,utcMSeconds/10);

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_LAT_LONG)
        {
            double latitude = location.gpsLocation.latitude;
            double longitude = location.gpsLocation.longitude;
            char latHemisphere;
            char lonHemisphere;
            double latMinutes;
            double lonMinutes;

            if (latitude > 0)
            {
                latHemisphere = 'N';
            }
            else
            {
                latHemisphere = 'S';
                latitude *= -1.0;
            }

            if (longitude < 0)
            {
                lonHemisphere = 'W';
                longitude *= -1.0;
            }
            else
            {
                lonHemisphere = 'E';
            }

            latMinutes = fmod(latitude * 60.0 , 60.0);
            lonMinutes = fmod(longitude * 60.0 , 60.0);

            length = snprintf(pMarker, lengthRemaining, "%02d%09.6lf,%c,%03d%09.6lf,%c,",
                              (uint8_t)floor(latitude), latMinutes, latHemisphere,
                              (uint8_t)floor(longitude),lonMinutes, lonHemisphere);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining,",,,,");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_SPEED)
        {
            float speedKnots = location.gpsLocation.speed * (3600.0/1852.0);
            length = snprintf(pMarker, lengthRemaining, "%.1lf,", speedKnots);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining, ",");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_BEARING)
        {
            length = snprintf(pMarker, lengthRemaining, "%.1lf,", location.gpsLocation.bearing);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining, ",");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        length = snprintf(pMarker, lengthRemaining, "%2.2d%2.2d%2.2d,",
                          utcDay, utcMonth, utcYear);

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_MAG_DEV)
        {
            float magneticVariation = locationExtended.magneticDeviation;
            char direction;
            if (magneticVariation < 0.0)
            {
                direction = 'W';
                magneticVariation *= -1.0;
            }
            else
            {
                direction = 'E';
            }

            length = snprintf(pMarker, lengthRemaining, "%.1lf,%c,",
                              magneticVariation, direction);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining, ",,");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (!(location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_LAT_LONG))
            // N means no fix
            length = snprintf(pMarker, lengthRemaining, "%c", 'N');
        else if (LOC_NAV_MASK_SBAS_CORRECTION_IONO & locationExtended.navSolutionMask)
            // D means differential
            length = snprintf(pMarker, lengthRemaining, "%c", 'D');
        else if (LOC_POS_TECH_MASK_SENSORS == locationExtended.tech_mask)
            // E means estimated (dead reckoning)
            length = snprintf(pMarker, lengthRemaining, "%c", 'E');
        else  // A means autonomous
            length = snprintf(pMarker, lengthRemaining, "%c", 'A');

        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        // -------------------
        // ------$--GGA-------
        // -------------------

        pMarker = sentence;
        lengthRemaining = sizeof(sentence);

        length = snprintf(pMarker, lengthRemaining, "$%sGGA,%02d%02d%02d.%02d," ,
                          talker, utcHours, utcMinutes, utcSeconds, utcMSeconds/10);

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_LAT_LONG)
        {
            double latitude = location.gpsLocation.latitude;
            double longitude = location.gpsLocation.longitude;
            char latHemisphere;
            char lonHemisphere;
            double latMinutes;
            double lonMinutes;

            if (latitude > 0)
            {
                latHemisphere = 'N';
            }
            else
            {
                latHemisphere = 'S';
                latitude *= -1.0;
            }

            if (longitude < 0)
            {
                lonHemisphere = 'W';
                longitude *= -1.0;
            }
            else
            {
                lonHemisphere = 'E';
            }

            latMinutes = fmod(latitude * 60.0 , 60.0);
            lonMinutes = fmod(longitude * 60.0 , 60.0);

            length = snprintf(pMarker, lengthRemaining, "%02d%09.6lf,%c,%03d%09.6lf,%c,",
                              (uint8_t)floor(latitude), latMinutes, latHemisphere,
                              (uint8_t)floor(longitude),lonMinutes, lonHemisphere);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining,",,,,");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        char gpsQuality;
        if (!(location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_LAT_LONG))
            gpsQuality = '0'; // 0 means no fix
        else if (LOC_NAV_MASK_SBAS_CORRECTION_IONO & locationExtended.navSolutionMask)
            gpsQuality = '2'; // 2 means DGPS fix
        else if (LOC_POS_TECH_MASK_SENSORS == locationExtended.tech_mask)
            gpsQuality = '6'; // 6 means estimated (dead reckoning)
        else
            gpsQuality = '1'; // 1 means GPS fix

        // Number of satellites in use, 00-12
        if (svUsedCount > MAX_SATELLITES_IN_USE)
            svUsedCount = MAX_SATELLITES_IN_USE;
        if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_DOP)
        {   // dop is in locationExtended, (QMI)
            length = snprintf(pMarker, lengthRemaining, "%c,%02d,%.1f,",
                              gpsQuality, svUsedCount, locationExtended.hdop);
        }
        else if (sv_cache_info.pdop > 0 && sv_cache_info.hdop > 0 && sv_cache_info.vdop > 0)
        {   // dop was cached from sv report (RPC)
            length = snprintf(pMarker, lengthRemaining, "%c,%02d,%.1f,",
                              gpsQuality, svUsedCount, sv_cache_info.hdop);
        }
        else
        {   // no hdop
            length = snprintf(pMarker, lengthRemaining, "%c,%02d,,",
                              gpsQuality, svUsedCount);
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_ALTITUDE_MEAN_SEA_LEVEL)
        {
            length = snprintf(pMarker, lengthRemaining, "%.1lf,M,",
                              locationExtended.altitudeMeanSeaLevel);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining,",,");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if ((location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_ALTITUDE) &&
            (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_ALTITUDE_MEAN_SEA_LEVEL))
        {
            length = snprintf(pMarker, lengthRemaining, "%.1lf,M,,",
                              location.gpsLocation.altitude - locationExtended.altitudeMeanSeaLevel);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining,",,,");
        }

        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        // clear the cache so they can't be used again
        sv_cache_info.gps_used_mask = 0;
        sv_cache_info.glo_used_mask = 0;
        sv_cache_info.gal_used_mask = 0;
    }
    //Send blank NMEA reports for non-final fixes
    else {
        strlcpy(sentence, "$GPGSA,A,1,,,,,,,,,,,,,,,", sizeof(sentence));
        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        strlcpy(sentence, "$GNGSA,A,1,,,,,,,,,,,,,,,", sizeof(sentence));
        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        strlcpy(sentence, "$GPVTG,,T,,M,,N,,K,N", sizeof(sentence));
        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        strlcpy(sentence, "$GPRMC,,V,,,,,,,,,,N", sizeof(sentence));
        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        strlcpy(sentence, "$GPGGA,,,,,,0,,,,,,,,", sizeof(sentence));
        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);
    }
    // clear the dop cache so they can't be used again
    sv_cache_info.pdop = 0;
    sv_cache_info.hdop = 0;
    sv_cache_info.vdop = 0;

    EXIT_LOG(%d, 0);
}



/*===========================================================================
FUNCTION    loc_nmea_generate_sv

DESCRIPTION
   Generate NMEA sentences generated based on sv report

DEPENDENCIES
   NONE

RETURN VALUE
   0

SIDE EFFECTS
   N/A

===========================================================================*/
void loc_nmea_generate_sv(const LocGnssSvStatus &svStatus,
                              const GpsLocationExtended &locationExtended,
                              std::vector<std::string> &nmeaArraystr)
{
    ENTRY_LOG();

    char sentence[NMEA_SENTENCE_MAX_LENGTH] = {0};
    char* pMarker = sentence;
    int lengthRemaining = sizeof(sentence);
    int length = 0;
    int svCount = svStatus.num_svs;
    int sentenceCount = 0;
    int sentenceNumber = 1;
    int svNumber = 1;

    //Count GPS SVs for saparating GPS from GLONASS and throw others

    sv_cache_info.gps_used_mask = 0;
    sv_cache_info.glo_used_mask = 0;
    sv_cache_info.gal_used_mask = 0;

    sv_cache_info.gps_count = 0;
    sv_cache_info.glo_count = 0;
    sv_cache_info.gal_count = 0;
    for(svNumber=1; svNumber <= svCount; svNumber++) {
        if (LOC_GNSS_CONSTELLATION_GPS == svStatus.gnss_sv_list[svNumber - 1].constellation)
        {
            // cache the used in fix mask, as it will be needed to send $GPGSA
            // during the position report
            if (LOC_GNSS_SV_FLAGS_USED_IN_FIX == (svStatus.gnss_sv_list[svNumber - 1].flags & LOC_GNSS_SV_FLAGS_USED_IN_FIX))
            {
                sv_cache_info.gps_used_mask |= (1 << (svStatus.gnss_sv_list[svNumber - 1].svid - 1));
            }
            sv_cache_info.gps_count++;
        }
        else if (LOC_GNSS_CONSTELLATION_GLONASS == svStatus.gnss_sv_list[svNumber - 1].constellation)
        {
            // cache the used in fix mask, as it will be needed to send $GNGSA
            // during the position report
            if (LOC_GNSS_SV_FLAGS_USED_IN_FIX == (svStatus.gnss_sv_list[svNumber - 1].flags & LOC_GNSS_SV_FLAGS_USED_IN_FIX))
            {
                sv_cache_info.glo_used_mask |= (1 << (svStatus.gnss_sv_list[svNumber - 1].svid - 1));
            }
            sv_cache_info.glo_count++;
        }
        else if (LOC_GNSS_CONSTELLATION_GALILEO == svStatus.gnss_sv_list[svNumber - 1].constellation)
        {
            // cache the used in fix mask, as it will be needed to send $GAGSA
            // during the position report
            if (LOC_GNSS_SV_FLAGS_USED_IN_FIX == (svStatus.gnss_sv_list[svNumber - 1].flags & LOC_GNSS_SV_FLAGS_USED_IN_FIX))
            {
                sv_cache_info.gal_used_mask |= (1 << (svStatus.gnss_sv_list[svNumber - 1].svid - 1));
            }
            sv_cache_info.gal_count++;
        }
    }

    loc_nmea_sv_meta sv_meta;
    // ------------------
    // ------$GPGSV------
    // ------------------

    loc_nmea_generate_GSV(svStatus, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, LOC_GNSS_CONSTELLATION_GPS, false), nmeaArraystr);

    // ------------------
    // ------$GLGSV------
    // ------------------

    loc_nmea_generate_GSV(svStatus, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, LOC_GNSS_CONSTELLATION_GLONASS, false), nmeaArraystr);

    // ------------------
    // ------$GAGSV------
    // ------------------

    loc_nmea_generate_GSV(svStatus, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, LOC_GNSS_CONSTELLATION_GALILEO, false), nmeaArraystr);


    // For RPC, the DOP are sent during sv report, so cache them
    // now to be sent during position report.
    // For QMI, the DOP will be in position report.
    if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_DOP)
    {
        sv_cache_info.pdop = locationExtended.pdop;
        sv_cache_info.hdop = locationExtended.hdop;
        sv_cache_info.vdop = locationExtended.vdop;
    }
    else
    {
        sv_cache_info.pdop = 0;
        sv_cache_info.hdop = 0;
        sv_cache_info.vdop = 0;
    }

    EXIT_LOG(%d, 0);
}
