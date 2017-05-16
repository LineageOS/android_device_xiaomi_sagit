/* Copyright (c) 2016-2017, The Linux Foundation. All rights reserved.
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
#ifndef __SYSTEM_STATUS__
#define __SYSTEM_STATUS__

#include <stdint.h>
#include <vector>

namespace loc_core
{

/******************************************************************************
 SystemStatus report data structure
******************************************************************************/
class SystemStatusItemBase
{
public:
    timespec mUtcTime;     // UTC timestamp when this info was last updated
    timespec mUtcReported; // UTC timestamp when this info was reported

    SystemStatusItemBase() {
        timeval tv;
        gettimeofday(&tv, NULL);
        mUtcTime.tv_sec  = tv.tv_sec;
        mUtcTime.tv_nsec = tv.tv_usec *1000ULL;
        mUtcReported = mUtcTime;
    };
    virtual ~SystemStatusItemBase() { };
    virtual void dump(void) { };
};

class SystemStatusPQWM1;
class SystemStatusTimeAndClock : public SystemStatusItemBase
{
public:
    uint16_t mGpsWeek;
    uint32_t mGpsTowMs;
    uint8_t  mTimeValid;
    uint8_t  mTimeSource;
    int32_t  mTimeUnc;
    int32_t  mClockFreqBias;
    int32_t  mClockFreqBiasUnc;
    SystemStatusTimeAndClock(const SystemStatusPQWM1& nmea);
    bool equals(SystemStatusTimeAndClock& peer);
    void dump(void);
};

class SystemStatusXoState : public SystemStatusItemBase
{
public:
    uint8_t  mXoState;
    SystemStatusXoState(const SystemStatusPQWM1& nmea);
    bool equals(SystemStatusXoState& peer);
    void dump(void);
};

class SystemStatusRfAndParams : public SystemStatusItemBase
{
public:
    int32_t  mPgaGain;
    uint32_t mGpsBpAmpI;
    uint32_t mGpsBpAmpQ;
    uint32_t mAdcI;
    uint32_t mAdcQ;
    uint32_t mJammerGps;
    uint32_t mJammerGlo;
    uint32_t mJammerBds;
    uint32_t mJammerGal;
    SystemStatusRfAndParams(const SystemStatusPQWM1& nmea);
    bool equals(SystemStatusRfAndParams& peer);
    void dump(void);
};

class SystemStatusErrRecovery : public SystemStatusItemBase
{
public:
    uint32_t mRecErrorRecovery;
    SystemStatusErrRecovery(const SystemStatusPQWM1& nmea);
    bool equals(SystemStatusErrRecovery& peer);
    void dump(void);
};

class SystemStatusPQWP1;
class SystemStatusInjectedPosition : public SystemStatusItemBase
{
public:
    uint8_t  mEpiValidity;
    float    mEpiLat;
    float    mEpiLon;
    float    mEpiAlt;
    float    mEpiHepe;
    float    mEpiAltUnc;
    uint8_t  mEpiSrc;
    SystemStatusInjectedPosition(const SystemStatusPQWP1& nmea);
    bool equals(SystemStatusInjectedPosition& peer);
    void dump(void);
};

class SystemStatusPQWP2;
class SystemStatusBestPosition : public SystemStatusItemBase
{
public:
    float    mBestLat;
    float    mBestLon;
    float    mBestAlt;
    float    mBestHepe;
    float    mBestAltUnc;
    SystemStatusBestPosition(const SystemStatusPQWP2& nmea);
    bool equals(SystemStatusBestPosition& peer);
    void dump(void);
};

class SystemStatusPQWP3;
class SystemStatusXtra : public SystemStatusItemBase
{
public:
    uint8_t   mXtraValidMask;
    uint32_t  mGpsXtraAge;
    uint32_t  mGloXtraAge;
    uint32_t  mBdsXtraAge;
    uint32_t  mGalXtraAge;
    uint32_t  mQzssXtraAge;
    uint32_t  mGpsXtraValid;
    uint32_t  mGloXtraValid;
    uint64_t  mBdsXtraValid;
    uint64_t  mGalXtraValid;
    uint8_t   mQzssXtraValid;
    SystemStatusXtra(const SystemStatusPQWP3& nmea);
    bool equals(SystemStatusXtra& peer);
    void dump(void);
};

class SystemStatusPQWP4;
class SystemStatusEphemeris : public SystemStatusItemBase
{
public:
    uint32_t  mGpsEpheValid;
    uint32_t  mGloEpheValid;
    uint64_t  mBdsEpheValid;
    uint64_t  mGalEpheValid;
    uint8_t   mQzssEpheValid;
    SystemStatusEphemeris(const SystemStatusPQWP4& nmea);
    bool equals(SystemStatusEphemeris& peer);
    void dump(void);
};

class SystemStatusPQWP5;
class SystemStatusSvHealth : public SystemStatusItemBase
{
public:
    uint32_t  mGpsUnknownMask;
    uint32_t  mGloUnknownMask;
    uint64_t  mBdsUnknownMask;
    uint64_t  mGalUnknownMask;
    uint8_t   mQzssUnknownMask;
    uint32_t  mGpsGoodMask;
    uint32_t  mGloGoodMask;
    uint64_t  mBdsGoodMask;
    uint64_t  mGalGoodMask;
    uint8_t   mQzssGoodMask;
    uint32_t  mGpsBadMask;
    uint32_t  mGloBadMask;
    uint64_t  mBdsBadMask;
    uint64_t  mGalBadMask;
    uint8_t   mQzssBadMask;
    SystemStatusSvHealth(const SystemStatusPQWP5& nmea);
    bool equals(SystemStatusSvHealth& peer);
    void dump(void);
};

class SystemStatusPQWP6;
class SystemStatusPdr : public SystemStatusItemBase
{
public:
    uint32_t  mFixInfoMask;
    SystemStatusPdr(const SystemStatusPQWP6& nmea);
    bool equals(SystemStatusPdr& peer);
    void dump(void);
};

class SystemStatusPQWS1;
class SystemStatusPositionFailure : public SystemStatusItemBase
{
public:
    uint32_t  mFixInfoMask;
    uint32_t  mHepeLimit;
    SystemStatusPositionFailure(const SystemStatusPQWS1& nmea);
    bool equals(SystemStatusPositionFailure& peer);
    void dump(void);
};

/******************************************************************************
 SystemStatusReports
******************************************************************************/
class SystemStatusReports
{
public:
    std::vector<SystemStatusTimeAndClock>     mTimeAndClock;
    std::vector<SystemStatusXoState>          mXoState;
    std::vector<SystemStatusRfAndParams>      mRfAndParams;
    std::vector<SystemStatusErrRecovery>      mErrRecovery;

    std::vector<SystemStatusInjectedPosition> mInjectedPosition;
    std::vector<SystemStatusBestPosition>     mBestPosition;
    std::vector<SystemStatusXtra>             mXtra;
    std::vector<SystemStatusEphemeris>        mEphemeris;
    std::vector<SystemStatusSvHealth>         mSvHealth;
    std::vector<SystemStatusPdr>              mPdr;
    std::vector<SystemStatusPositionFailure>  mPositionFailure;
};

/******************************************************************************
 SystemStatus
******************************************************************************/
class SystemStatus
{
    static pthread_mutex_t mMutexSystemStatus;

    static const uint32_t                     maxTimeAndClock = 5;
    static const uint32_t                     maxXoState = 5;
    static const uint32_t                     maxRfAndParams = 5;
    static const uint32_t                     maxErrRecovery = 5;

    static const uint32_t                     maxInjectedPosition = 5;
    static const uint32_t                     maxBestPosition = 5;
    static const uint32_t                     maxXtra = 5;
    static const uint32_t                     maxEphemeris = 5;
    static const uint32_t                     maxSvHealth = 5;
    static const uint32_t                     maxPdr = 5;
    static const uint32_t                     maxPositionFailure = 5;

    SystemStatusReports mCache;

    bool setTimeAndCLock(const SystemStatusPQWM1& nmea);
    bool setXoState(const SystemStatusPQWM1& nmea);
    bool setRfAndParams(const SystemStatusPQWM1& nmea);
    bool setErrRecovery(const SystemStatusPQWM1& nmea);

    bool setInjectedPosition(const SystemStatusPQWP1& nmea);
    bool setBestPosition(const SystemStatusPQWP2& nmea);
    bool setXtra(const SystemStatusPQWP3& nmea);
    bool setEphemeris(const SystemStatusPQWP4& nmea);
    bool setSvHealth(const SystemStatusPQWP5& nmea);
    bool setPdr(const SystemStatusPQWP6& nmea);
    bool setPositionFailure(const SystemStatusPQWS1& nmea);

public:
    SystemStatus();
    ~SystemStatus() { }

    bool setNmeaString(const char *data, uint32_t len);
    bool getReport(SystemStatusReports& reports, bool isLatestonly = false) const;
};

} // namespace loc_core

#endif //__SYSTEM_STATUS__

