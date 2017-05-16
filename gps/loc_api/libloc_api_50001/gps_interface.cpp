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

#define LOG_NDEBUG 0
#define LOG_TAG "GpsInterface"

#include <hardware/gps.h>
#include <gps_extended.h>
#include <gps_converter.h>
#include <loc_eng.h>
#include <loc_target.h>
#include <loc_log.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <LocDualContext.h>
#include <platform_lib_includes.h>
#include <cutils/properties.h>

using namespace loc_core;

// All functions and variables should be static
// except gps_get_hardware_interface() and get_gps_interface()

extern "C" const LocGpsInterface* loc_eng_gps_get_hardware_interface ();
extern "C" const LocGpsInterface* loc_eng_get_gps_interface();

/*===========================================================================
  Functions and variables for sGpsInterface
===========================================================================*/
static const LocGpsInterface* pLocGpsInterface = NULL;

static GpsCallbacks sGpsCallbacks;
static GpsCallbacks* pGpsCallbacks  = NULL;

static void local_gps_location_callback(LocGpsLocation* location);
static void local_gps_sv_status_callback(LocGpsSvStatus* sv_status);
static void local_gps_status_callback(LocGpsStatus* status);
static void local_gps_nmea_callback(LocGpsUtcTime timestamp, const char* nmea, int length);
static void local_gps_set_capabilities(uint32_t capabilities);
static void local_gps_acquire_wakelock();
static void local_gps_release_wakelock();
static pthread_t local_gps_create_thread(const char* name, void (*start)(void *), void* arg);
static void local_gps_request_utc_time();
static void local_gnss_set_system_info(const LocGnssSystemInfo* info);
static void local_gnss_sv_status_callback(LocGnssSvStatus* sv_info);

static LocGpsCallbacks sLocGpsCallbacks = {
    sizeof(LocGpsCallbacks),
    local_gps_location_callback,
    local_gps_status_callback,
    local_gps_sv_status_callback,
    local_gps_nmea_callback,
    local_gps_set_capabilities,
    local_gps_acquire_wakelock,
    local_gps_release_wakelock,
    local_gps_create_thread,
    local_gps_request_utc_time,
    local_gnss_set_system_info,
    local_gnss_sv_status_callback,
};

static int  loc_init(GpsCallbacks* callbacks);
static int  loc_start();
static int  loc_stop();
static void loc_cleanup();
static int  loc_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty);
static int  loc_inject_location(double latitude, double longitude, float accuracy);
static void loc_delete_aiding_data(GpsAidingData f);
static int  loc_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
                                  uint32_t min_interval, uint32_t preferred_accuracy,
                                  uint32_t preferred_time);
static const void* loc_get_extension(const char* name);

static const GpsInterface sGpsInterface = {
   sizeof(GpsInterface),
   loc_init,
   loc_start,
   loc_stop,
   loc_cleanup,
   loc_inject_time,
   loc_inject_location,
   loc_delete_aiding_data,
   loc_set_position_mode,
   loc_get_extension
};

/*===========================================================================
  Functions and variables for sGpsXtraInterface
===========================================================================*/
static const LocGpsXtraInterface* pLocGpsXtraInterface = NULL;

static GpsXtraCallbacks sGpsXtraCallbacks;
static GpsXtraCallbacks* pGpsXtraCallbacks = NULL;

static void local_gps_xtra_download_request();
static pthread_t local_gps_xtra_create_thread(const char* name, void (*start)(void *), void* arg);

static LocGpsXtraCallbacks sLocGpsXtraCallbacks = {
    local_gps_xtra_download_request,
    local_gps_xtra_create_thread
};

static int loc_xtra_init(GpsXtraCallbacks* callbacks);
static int loc_xtra_inject_data(char* data, int length);

static const GpsXtraInterface sGpsXtraInterface = {
    sizeof(GpsXtraInterface),
    loc_xtra_init,
    loc_xtra_inject_data
};

/*===========================================================================
  Functions and variables for sAGpsInterface
===========================================================================*/
static const LocAGpsInterface* pLocAGpsInterface = NULL;

static AGpsCallbacks sAGpsCallbacks;
static AGpsCallbacks* pAGpsCallbacks = NULL;

static void local_agps_status_callback(LocAGpsStatus* status);
static pthread_t local_agps_create_thread(const char* name, void (*start)(void *), void* arg);

static LocAGpsCallbacks sLocAGpsCallbacks = {
    local_agps_status_callback,
    local_agps_create_thread
};

static void loc_agps_init(AGpsCallbacks* callbacks);
static int  loc_agps_open(const char* apn);
static int  loc_agps_closed();
static int  loc_agps_open_failed();
static int  loc_agps_set_server(AGpsType type, const char *hostname, int port);
static int  loc_agps_open_with_apniptype( const char* apn, ApnIpType apnIpType);

static const AGpsInterface sAGpsInterface = {
   sizeof(AGpsInterface),
   loc_agps_init,
   loc_agps_open,
   loc_agps_closed,
   loc_agps_open_failed,
   loc_agps_set_server,
   loc_agps_open_with_apniptype
};

/*===========================================================================
  Functions and variables for SUPL sSuplCertificateInterface
===========================================================================*/
static const LocSuplCertificateInterface* pLocSuplCertificateInterface = NULL;

static int loc_agps_install_certificates(const DerEncodedCertificate* certificates,
                                         size_t length);
static int loc_agps_revoke_certificates(const Sha1CertificateFingerprint* fingerprints,
                                        size_t length);

static const SuplCertificateInterface sSuplCertificateInterface =
{
    sizeof(SuplCertificateInterface),
    loc_agps_install_certificates,
    loc_agps_revoke_certificates
};

/*===========================================================================
  Functions and variables for sGpsNiInterface
===========================================================================*/
static const LocGpsNiInterface* pLocGpsNiInterface = NULL;

static GpsNiCallbacks sGpsNiCallbacks;
static GpsNiCallbacks* pGpsNiCallbacks = NULL;

static void local_gps_ni_notify_callback(LocGpsNiNotification *notification);
static pthread_t local_gps_ni_create_thread(const char* name, void (*start)(void *), void* arg);

static LocGpsNiCallbacks sLocGpsNiCallbacks = {
    local_gps_ni_notify_callback,
    local_gps_ni_create_thread
};

static void loc_ni_init(GpsNiCallbacks *callbacks);
static void loc_ni_respond(int notif_id, GpsUserResponseType user_response);

static const GpsNiInterface sGpsNiInterface =
{
   sizeof(GpsNiInterface),
   loc_ni_init,
   loc_ni_respond,
};

/*===========================================================================
  Functions and variables for sAGpsRilInterface
===========================================================================*/
static const LocAGpsRilInterface* pLocAGpsRilInterface = NULL;

static void loc_agps_ril_init(AGpsRilCallbacks* callbacks);
static void loc_agps_ril_set_ref_location(const AGpsRefLocation *agps_reflocation, size_t sz_struct);
static void loc_agps_ril_set_set_id(AGpsSetIDType type, const char* setid);
static void loc_agps_ril_ni_message(uint8_t *msg, size_t len);
static void loc_agps_ril_update_network_state(int connected, int type, int roaming, const char* extra_info);
static void loc_agps_ril_update_network_availability(int avaiable, const char* apn);

static const AGpsRilInterface sAGpsRilInterface =
{
   sizeof(AGpsRilInterface),
   loc_agps_ril_init,
   loc_agps_ril_set_ref_location,
   loc_agps_ril_set_set_id,
   loc_agps_ril_ni_message,
   loc_agps_ril_update_network_state,
   loc_agps_ril_update_network_availability
};

/*===========================================================================
  Functions and variables for sGpsGeofencingInterface
===========================================================================*/
static const LocGpsGeofencingInterface* pLocGpsGeofencingInterface = NULL;

static GpsGeofenceCallbacks sGpsGeofenceCallbacks;
static GpsGeofenceCallbacks* pGpsGeofenceCallbacks  = NULL;

static void local_gps_geofence_transition_callback(int32_t geofence_id,  LocGpsLocation* location,
        int32_t transition, LocGpsUtcTime timestamp);
static void local_gps_geofence_status_callback(int32_t status, LocGpsLocation* last_location);
static void local_gps_geofence_add_callback(int32_t geofence_id, int32_t status);
static void local_gps_geofence_remove_callback(int32_t geofence_id, int32_t status);
static void local_gps_geofence_pause_callback(int32_t geofence_id, int32_t status);
static void local_gps_geofence_resume_callback(int32_t geofence_id, int32_t status);
static pthread_t local_geofence_create_thread(const char* name, void (*start)(void *), void* arg);

static LocGpsGeofenceCallbacks sLocGpsGeofenceCallbacks =
{
    local_gps_geofence_transition_callback,
    local_gps_geofence_status_callback,
    local_gps_geofence_add_callback,
    local_gps_geofence_remove_callback,
    local_gps_geofence_pause_callback,
    local_gps_geofence_resume_callback,
    local_geofence_create_thread
};

static void loc_geofence_init(GpsGeofenceCallbacks* callbacks);
static void loc_add_geofence_area(int32_t geofence_id, double latitude, double longitude,
    double radius_meters, int last_transition, int monitor_transitions,
    int notification_responsiveness_ms, int unknown_timer_ms);
static void loc_pause_geofence(int32_t geofence_id);
static void loc_resume_geofence(int32_t geofence_id, int monitor_transitions);
static void loc_remove_geofence_area(int32_t geofence_id);

static const GpsGeofencingInterface sGpsGeofencingInterface =
{
    sizeof(GpsGeofencingInterface),
    loc_geofence_init,
    loc_add_geofence_area,
    loc_pause_geofence,
    loc_resume_geofence,
    loc_remove_geofence_area
};

/*===========================================================================
  Functions and variables for sGpsMeasurementInterface
===========================================================================*/
static const LocGpsMeasurementInterface* pLocGpsMeasurementInterface = NULL;

static GpsMeasurementCallbacks sGpsMeasurementCallbacks;
static GpsMeasurementCallbacks* pGpsMeasurementCallbacks = NULL;

static void local_gps_measurement_callback(LocGpsData* data);
static void local_gnss_measurement_callback(LocGnssData* data);

static LocGpsMeasurementCallbacks sLocGpsMeasurementCallbacks =
{
    sizeof(LocGpsMeasurementCallbacks),
    local_gps_measurement_callback,
    local_gnss_measurement_callback
};

static int loc_gps_measurement_init(GpsMeasurementCallbacks* callbacks);
static void loc_gps_measurement_close();

static const GpsMeasurementInterface sGpsMeasurementInterface =
{
    sizeof(GpsMeasurementInterface),
    loc_gps_measurement_init,
    loc_gps_measurement_close
};

/*===========================================================================
  Functions and variables for sGnssConfigurationInterface
===========================================================================*/
static const LocGnssConfigurationInterface* pLocGnssConfigurationInterface = NULL;

static void loc_configuration_update(const char* config_data, int32_t length);

static const GnssConfigurationInterface sGnssConfigurationInterface =
{
    sizeof(GnssConfigurationInterface),
    loc_configuration_update
};


/*===========================================================================
FUNCTION    gps_get_hardware_interface

DESCRIPTION
   Returns the GPS hardware interaface based on LOC API
   if GPS is enabled.

DEPENDENCIES
   None

RETURN VALUE
   0: success

SIDE EFFECTS
   N/A

===========================================================================*/
extern "C" const GpsInterface* gps_get_hardware_interface ()
{
    ENTRY_LOG_CALLFLOW();
    const GpsInterface* retVal;
    pLocGpsInterface = loc_eng_gps_get_hardware_interface();
    if (NULL != pLocGpsInterface) {
        retVal = &sGpsInterface;
    } else {
        retVal = NULL;
    }
    EXIT_LOG(%p, retVal);
    return retVal;
}

// for gps.c
extern "C" const GpsInterface* get_gps_interface()
{
    pLocGpsInterface = loc_eng_get_gps_interface();
    return &sGpsInterface;
}

/*===========================================================================
  Implementation of functions for sGpsInterface
===========================================================================*/
static void local_gps_location_callback(LocGpsLocation* location)
{
    ENTRY_LOG();
    if (NULL != pGpsCallbacks && NULL != pGpsCallbacks->location_cb) {
        GpsLocation loc;
        GpsLocation* p_loc = NULL;
        if (NULL != location) {
            Q2A_GpsLocation(*location, loc);
            p_loc = &loc;
        }
        // We are not sure whether NULL pointer is a legal
        // parameter to the original callback functions or not.
        // So we should forward NULL pointer at this layer.
        pGpsCallbacks->location_cb(p_loc);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gps_sv_status_callback(LocGpsSvStatus* sv_status)
{
    ENTRY_LOG();
    if (NULL != pGpsCallbacks && NULL != pGpsCallbacks->sv_status_cb) {
        CALLBACK_LOG_CALLFLOW("sv_status_cb -", %d, sv_status->num_svs);
        GpsSvStatus st;
        GpsSvStatus* p_st = NULL;
        if (NULL != sv_status) {
            Q2A_GpsSvStatus(*sv_status, st);
            p_st = &st;
        }
        pGpsCallbacks->sv_status_cb(p_st);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gps_status_callback(LocGpsStatus* status)
{
    ENTRY_LOG();
    if (NULL != pGpsCallbacks && NULL != pGpsCallbacks->status_cb) {
        GpsStatus st;
        GpsStatus* p_st = NULL;
        if (NULL != status) {
            Q2A_GpsStatus(*status, st);
            p_st = &st;
        }
        pGpsCallbacks->status_cb(p_st);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gps_nmea_callback(LocGpsUtcTime timestamp, const char* nmea, int length)
{
    ENTRY_LOG();
    if (NULL != pGpsCallbacks && NULL != pGpsCallbacks->nmea_cb) {
        pGpsCallbacks->nmea_cb(A2Q_GpsUtcTime(timestamp), nmea, length);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gps_set_capabilities(uint32_t capabilities)
{
    ENTRY_LOG();
    if (NULL != pGpsCallbacks && NULL != pGpsCallbacks->set_capabilities_cb) {
        pGpsCallbacks->set_capabilities_cb(capabilities);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gps_acquire_wakelock()
{
    ENTRY_LOG();
    if (NULL != pGpsCallbacks && NULL != pGpsCallbacks->acquire_wakelock_cb) {
        pGpsCallbacks->acquire_wakelock_cb();
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gps_release_wakelock()
{
    ENTRY_LOG();
    if (NULL != pGpsCallbacks && NULL != pGpsCallbacks->release_wakelock_cb) {
        pGpsCallbacks->release_wakelock_cb();
    }
    EXIT_LOG(%s, VOID_RET);
}

static pthread_t local_gps_create_thread(const char* name, void (*start)(void *), void* arg)
{
    ENTRY_LOG();
    pthread_t retVal = -1;
    if (NULL != pGpsCallbacks && NULL != pGpsCallbacks->create_thread_cb) {
        retVal = pGpsCallbacks->create_thread_cb(name, start, arg);
    }
    EXIT_LOG(%d, (int)retVal);
    return retVal;
}

static void local_gps_request_utc_time()
{
    ENTRY_LOG();
    if (NULL != pGpsCallbacks && NULL != pGpsCallbacks->request_utc_time_cb) {
        pGpsCallbacks->request_utc_time_cb();
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gnss_set_system_info(const LocGnssSystemInfo* info)
{
    ENTRY_LOG();
    if (NULL != pGpsCallbacks && NULL != pGpsCallbacks->set_system_info_cb) {
        GnssSystemInfo sysinfo;
        GnssSystemInfo* p_sysinfo = NULL;
        if (NULL != info) {
            Q2A_GnssSystemInfo(*info, sysinfo);
            p_sysinfo = &sysinfo;
        }
        pGpsCallbacks->set_system_info_cb(p_sysinfo);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gnss_sv_status_callback(LocGnssSvStatus* sv_info)
{
    ENTRY_LOG();
    if (NULL != pGpsCallbacks && NULL != pGpsCallbacks->gnss_sv_status_cb) {
        GnssSvStatus info;
        GnssSvStatus* p_info = NULL;
        if (NULL != sv_info) {
            Q2A_GnssSvStatus(*sv_info, info);
            p_info = &info;
        }
        pGpsCallbacks->gnss_sv_status_cb(p_info);
    }
    EXIT_LOG(%s, VOID_RET);
}

static int loc_init(GpsCallbacks* callbacks)
{
    ENTRY_LOG();

    int retVal = -1;
    LocGpsCallbacks* cbs = NULL;
    if (NULL != callbacks) {
        memcpy(&sGpsCallbacks, callbacks, sizeof(GpsCallbacks));
        pGpsCallbacks = &sGpsCallbacks;
        cbs = &sLocGpsCallbacks;
    }

    if(NULL != pLocGpsInterface && NULL != pLocGpsInterface->init) {
        retVal = pLocGpsInterface->init(cbs);
    }

    EXIT_LOG(%d, retVal);
    return retVal;
}

static int loc_start()
{
    ENTRY_LOG();
    int retVal = -1;
    if (NULL != pLocGpsInterface && NULL != pLocGpsInterface->start) {
        retVal = pLocGpsInterface->start();
    }
    EXIT_LOG(%d, retVal);
    return retVal;
}

static int loc_stop()
{
    ENTRY_LOG();
    int retVal = -1;
    if (NULL != pLocGpsInterface && NULL != pLocGpsInterface->stop) {
        retVal = pLocGpsInterface->stop();
    }
    EXIT_LOG(%d, retVal);
    return retVal;
}

static void loc_cleanup()
{
    ENTRY_LOG();
    if (NULL != pLocGpsInterface && NULL != pLocGpsInterface->cleanup) {
        pLocGpsInterface->cleanup();
    }
    EXIT_LOG(%s, VOID_RET);
}

static int loc_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
    ENTRY_LOG();
    int retVal = -1;
    if (NULL != pLocGpsInterface && NULL != pLocGpsInterface->inject_time) {
        retVal = pLocGpsInterface->inject_time(A2Q_GpsUtcTime(time),
                timeReference, uncertainty);
    }
    EXIT_LOG(%d, retVal);
    return retVal;
}

static int loc_inject_location(double latitude, double longitude, float accuracy)
{
    ENTRY_LOG();
    int retVal = -1;
    if (NULL != pLocGpsInterface && NULL != pLocGpsInterface->inject_location) {
        retVal = pLocGpsInterface->inject_location(latitude, longitude, accuracy);
    }
    EXIT_LOG(%d, retVal);
    return retVal;
}

static void loc_delete_aiding_data(GpsAidingData f)
{
    ENTRY_LOG();

    if (NULL != pLocGpsInterface && NULL != pLocGpsInterface->delete_aiding_data) {
        pLocGpsInterface->delete_aiding_data(A2Q_GpsAidingData(f));
    }

    EXIT_LOG(%s, VOID_RET);
}

static int  loc_set_position_mode(GpsPositionMode mode,
                                  GpsPositionRecurrence recurrence,
                                  uint32_t min_interval,
                                  uint32_t preferred_accuracy,
                                  uint32_t preferred_time)
{
    ENTRY_LOG();
    int retVal = -1;
    if (NULL != pLocGpsInterface && NULL != pLocGpsInterface->set_position_mode) {
        retVal = pLocGpsInterface->set_position_mode(A2Q_GpsPositionMode(mode),
                A2Q_GpsPositionRecurrence(recurrence),
                min_interval, preferred_accuracy, preferred_time);
    }
    EXIT_LOG(%d, retVal);
    return retVal;
}

const void* loc_get_extension(const char* name)
{
    ENTRY_LOG();
    const void* retVal = NULL;

    LOC_LOGD("%s:%d] For Interface = %s\n",__func__, __LINE__, name);
    if (NULL != pLocGpsInterface && NULL != pLocGpsInterface->get_extension) {
        if (strcmp(name, GPS_XTRA_INTERFACE) == 0)
        {
            pLocGpsXtraInterface = (const LocGpsXtraInterface*)pLocGpsInterface->
                get_extension(LOC_GPS_XTRA_INTERFACE);
            if (NULL != pLocGpsXtraInterface) {
                retVal = &sGpsXtraInterface;
            }
        }
        else if (strcmp(name, AGPS_INTERFACE) == 0)
        {
            pLocAGpsInterface = (const LocAGpsInterface*)pLocGpsInterface->
                get_extension(LOC_AGPS_INTERFACE);
            if (NULL != pLocAGpsInterface) {
                retVal = &sAGpsInterface;
            }
        }
        else if (strcmp(name, SUPL_CERTIFICATE_INTERFACE) == 0)
        {
            pLocSuplCertificateInterface = (const LocSuplCertificateInterface*)pLocGpsInterface->
                get_extension(LOC_SUPL_CERTIFICATE_INTERFACE);
            if (NULL != pLocAGpsInterface) {
                retVal = &sSuplCertificateInterface;
            }
        }
        else if (strcmp(name, GPS_NI_INTERFACE) == 0)
        {
            pLocGpsNiInterface = (const LocGpsNiInterface*)pLocGpsInterface->
                get_extension(LOC_GPS_NI_INTERFACE);
            if (NULL != pLocGpsNiInterface) {
                retVal = &sGpsNiInterface;
            }
        }
        else if (strcmp(name, AGPS_RIL_INTERFACE) == 0)
        {
            pLocAGpsRilInterface = (const LocAGpsRilInterface*)pLocGpsInterface->
                get_extension(LOC_AGPS_RIL_INTERFACE);
            if (NULL != pLocAGpsRilInterface) {
                retVal = &sAGpsRilInterface;
            }
        }
        else if (strcmp(name, GPS_GEOFENCING_INTERFACE) == 0)
        {
            pLocGpsGeofencingInterface = (const LocGpsGeofencingInterface*)pLocGpsInterface->
                get_extension(LOC_GPS_GEOFENCING_INTERFACE);
            if (NULL != pLocGpsGeofencingInterface) {
                retVal = &sGpsGeofencingInterface;
            }
        }
        else if (strcmp(name, GPS_MEASUREMENT_INTERFACE) == 0)
        {
            pLocGpsMeasurementInterface = (const LocGpsMeasurementInterface*)pLocGpsInterface->
                get_extension(LOC_GPS_MEASUREMENT_INTERFACE);
            if (NULL != pLocGpsMeasurementInterface) {
                retVal = &sGpsMeasurementInterface;
            }
        }
        else if (strcmp(name, GNSS_CONFIGURATION_INTERFACE) == 0)
        {
            pLocGnssConfigurationInterface = (const LocGnssConfigurationInterface*)pLocGpsInterface->
                get_extension(LOC_GNSS_CONFIGURATION_INTERFACE);
            if (NULL != pLocGnssConfigurationInterface) {
                retVal = &sGnssConfigurationInterface;
            }
        }
        else
        {
            LOC_LOGE ("get_extension: Invalid interface passed in\n");
        }
    }
    EXIT_LOG(%p, retVal);
    return retVal;
}

/*===========================================================================
  Implementation of functions for sGpsXtraInterface
===========================================================================*/
static void local_gps_xtra_download_request()
{
    ENTRY_LOG();
    if (NULL != pGpsXtraCallbacks && NULL != pGpsXtraCallbacks->download_request_cb) {
        pGpsXtraCallbacks->download_request_cb();
    }
    EXIT_LOG(%s, VOID_RET);
}

static pthread_t local_gps_xtra_create_thread(const char* name, void (*start)(void *), void* arg)
{
    ENTRY_LOG();
    pthread_t retVal = -1;
    if (NULL != pGpsXtraCallbacks && NULL != pGpsXtraCallbacks->create_thread_cb) {
        retVal = pGpsXtraCallbacks->create_thread_cb(name, start, arg);
    }
    EXIT_LOG(%d, (int)retVal);
    return retVal;
}

static int loc_xtra_init(GpsXtraCallbacks* callbacks)
{
    ENTRY_LOG();

    int retVal = -1;
    LocGpsXtraCallbacks* cbs = NULL;
    if (NULL != callbacks)
    {
        memcpy(&sGpsXtraCallbacks, callbacks, sizeof(GpsXtraCallbacks));
        pGpsXtraCallbacks = &sGpsXtraCallbacks;
        cbs = &sLocGpsXtraCallbacks;
    }

    if (NULL != pLocGpsXtraInterface && NULL != pLocGpsXtraInterface->init) {
        retVal = pLocGpsXtraInterface->init(cbs);
    }

    EXIT_LOG(%d, retVal);
    return retVal;
}


static int loc_xtra_inject_data(char* data, int length)
{
    ENTRY_LOG();
    int retVal = -1;
    if (NULL != pLocGpsXtraInterface && NULL != pLocGpsXtraInterface->inject_xtra_data) {
        retVal = pLocGpsXtraInterface->inject_xtra_data(data, length);
    }
    EXIT_LOG(%d, retVal);
    return retVal;
}

/*===========================================================================
  Implementation of functions for sAGpsInterface
===========================================================================*/
static void local_agps_status_callback(LocAGpsStatus* status)
{
    ENTRY_LOG();
    if (NULL != pAGpsCallbacks && NULL != pAGpsCallbacks->status_cb) {
        AGpsStatus st;
        AGpsStatus* p_st = NULL;
        if (NULL != status)
        {
            Q2A_AGpsStatus(*status, st);
            p_st = &st;
        }
        pAGpsCallbacks->status_cb(p_st);
    }
    EXIT_LOG(%s, VOID_RET);
}

static pthread_t local_agps_create_thread(const char* name, void (*start)(void *), void* arg)
{
    ENTRY_LOG();
    pthread_t retVal = -1;
    if (NULL != pAGpsCallbacks && NULL != pAGpsCallbacks->create_thread_cb) {
        retVal = pAGpsCallbacks->create_thread_cb(name, start, arg);
    }
    EXIT_LOG(%d, (int)retVal);
    return retVal;
}

static void loc_agps_init(AGpsCallbacks* callbacks)
{
    ENTRY_LOG();

    LocAGpsCallbacks* cbs = NULL;
    if (NULL != callbacks)
    {
        memcpy(&sAGpsCallbacks, callbacks, sizeof(AGpsCallbacks));
        pAGpsCallbacks = &sAGpsCallbacks;
        cbs = &sLocAGpsCallbacks;
    }

    if (NULL != pLocAGpsInterface && NULL != pLocAGpsInterface->init) {
        pLocAGpsInterface->init(cbs);
    }

    EXIT_LOG(%s, VOID_RET);
}

static int loc_agps_open(const char* apn)
{
    ENTRY_LOG();

    int retVal = -1;
    if (NULL != pLocAGpsInterface && NULL != pLocAGpsInterface->data_conn_open) {
        retVal = pLocAGpsInterface->data_conn_open(apn);
    }

    EXIT_LOG(%d, retVal);
    return retVal;
}

static int loc_agps_closed()
{
    ENTRY_LOG();

    int retVal = -1;
    if (NULL != pLocAGpsInterface && NULL != pLocAGpsInterface->data_conn_closed) {
        retVal = pLocAGpsInterface->data_conn_closed();
    }

    EXIT_LOG(%d, retVal);
    return retVal;
}

int loc_agps_open_failed()
{
    ENTRY_LOG();

    int retVal = -1;
    if (NULL != pLocAGpsInterface && NULL != pLocAGpsInterface->data_conn_failed) {
        retVal = pLocAGpsInterface->data_conn_failed();
    }

    EXIT_LOG(%d, retVal);
    return retVal;
}

static int loc_agps_set_server(AGpsType type, const char* hostname, int port)
{
    ENTRY_LOG();

    int retVal = -1;
    if (NULL != pLocAGpsInterface && NULL != pLocAGpsInterface->set_server) {
        retVal = pLocAGpsInterface->set_server(A2Q_AGpsType(type), hostname, port);
    }

    EXIT_LOG(%d, retVal);
    return retVal;
}

static int  loc_agps_open_with_apniptype(const char* apn, ApnIpType apnIpType)
{
    ENTRY_LOG();

    int retVal = -1;
    if (NULL != pLocAGpsInterface && NULL != pLocAGpsInterface->data_conn_open_with_apn_ip_type) {
        retVal = pLocAGpsInterface->data_conn_open_with_apn_ip_type(apn, A2Q_ApnIpType(apnIpType));
    }

    EXIT_LOG(%d, retVal);
    return retVal;
}

/*===========================================================================
  Implementation of functions for sSuplCertificateInterface
===========================================================================*/
static int loc_agps_install_certificates(const DerEncodedCertificate* certificates,
                                         size_t length)
{
    ENTRY_LOG();

    int retVal = -1;
    LocDerEncodedCertificate cert;
    LocDerEncodedCertificate* p_cert = NULL;
    if (NULL != certificates) {
        A2Q_DerEncodedCertificate(*certificates, cert);
        p_cert = &cert;
    }

    if (NULL != pLocSuplCertificateInterface &&
            NULL != pLocSuplCertificateInterface->install_certificates) {
        retVal = pLocSuplCertificateInterface->install_certificates(p_cert, length);
    }

    EXIT_LOG(%d, retVal);
    return retVal;
}

static int loc_agps_revoke_certificates(const Sha1CertificateFingerprint* fingerprints,
                                        size_t length)
{
    ENTRY_LOG();
    LOC_LOGE("%s:%d] agps_revoke_certificates not supported", __func__, __LINE__);
    int retVal = AGPS_CERTIFICATE_ERROR_GENERIC;
    EXIT_LOG(%d, retVal);
    return retVal;
}

/*===========================================================================
  Implementation of functions for sGpsNiInterface
===========================================================================*/
static void local_gps_ni_notify_callback(LocGpsNiNotification *notification)
{
    ENTRY_LOG();
    if (NULL != pGpsNiCallbacks && NULL != pGpsNiCallbacks->notify_cb) {
        GpsNiNotification notify;
        GpsNiNotification* p_notify = NULL;
        if (NULL != notification) {
            Q2A_GpsNiNotification(*notification, notify);
            p_notify = &notify;
        }
        pGpsNiCallbacks->notify_cb(p_notify);
    }
    EXIT_LOG(%s, VOID_RET);
}

static pthread_t local_gps_ni_create_thread(const char* name, void (*start)(void *), void* arg)
{
    ENTRY_LOG();
    pthread_t retVal = -1;
    if (NULL != pGpsNiCallbacks && NULL != pGpsNiCallbacks->create_thread_cb) {
        retVal = pGpsNiCallbacks->create_thread_cb(name, start, arg);
    }
    EXIT_LOG(%d, (int)retVal);
    return retVal;
}

void loc_ni_init(GpsNiCallbacks *callbacks)
{
    ENTRY_LOG();

    LocGpsNiCallbacks* cbs = NULL;
    if (NULL != callbacks) {
        memcpy(&sGpsNiCallbacks, callbacks, sizeof(GpsNiCallbacks));
        pGpsNiCallbacks = &sGpsNiCallbacks;
        cbs = &sLocGpsNiCallbacks;
    }

    if(NULL != pLocGpsNiInterface && NULL != pLocGpsNiInterface->init) {
        pLocGpsNiInterface->init(cbs);
    }

    EXIT_LOG(%s, VOID_RET);
}

void loc_ni_respond(int notif_id, GpsUserResponseType user_response)
{
    ENTRY_LOG();

    if(NULL != pLocGpsNiInterface && NULL != pLocGpsNiInterface->respond) {
        pLocGpsNiInterface->respond(notif_id, A2Q_GpsUserResponseType(user_response));
    }

    EXIT_LOG(%s, VOID_RET);
}

/*===========================================================================
  Implementation of functions for sAGpsRilInterface
===========================================================================*/
// Below stub functions are members of sAGpsRilInterface
static void loc_agps_ril_init( AGpsRilCallbacks* callbacks ) {}
static void loc_agps_ril_set_ref_location(const AGpsRefLocation *agps_reflocation, size_t sz_struct) {}
static void loc_agps_ril_set_set_id(AGpsSetIDType type, const char* setid) {}
static void loc_agps_ril_ni_message(uint8_t *msg, size_t len) {}
static void loc_agps_ril_update_network_state(int connected, int type, int roaming, const char* extra_info) {}

static void loc_agps_ril_update_network_availability(int available, const char* apn)
{
    ENTRY_LOG();

    if (NULL != pLocAGpsRilInterface &&
            NULL != pLocAGpsRilInterface->update_network_availability) {
        pLocAGpsRilInterface->update_network_availability(available, apn);
    }

    EXIT_LOG(%s, VOID_RET);
}

/*===========================================================================
  Implementation of functions for sGpsGeofencingInterface
===========================================================================*/
static void local_gps_geofence_transition_callback(int32_t geofence_id,  LocGpsLocation* location,
        int32_t transition, LocGpsUtcTime timestamp)
{
    ENTRY_LOG();
    if (NULL != pGpsGeofenceCallbacks && NULL != pGpsGeofenceCallbacks->geofence_transition_callback) {
        GpsLocation loc;
        GpsLocation* p_loc = NULL;
        if (NULL != location)
        {
            Q2A_GpsLocation(*location, loc);
            p_loc = &loc;
        }
        pGpsGeofenceCallbacks->geofence_transition_callback(geofence_id, p_loc, transition, Q2A_GpsUtcTime(timestamp));
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gps_geofence_status_callback(int32_t status, LocGpsLocation* last_location)
{
    ENTRY_LOG();
    if (NULL != pGpsGeofenceCallbacks && NULL != pGpsGeofenceCallbacks->geofence_status_callback) {
        GpsLocation loc;
        GpsLocation* p_loc = NULL;
        if (NULL != last_location)
        {
            Q2A_GpsLocation(*last_location, loc);
            p_loc = &loc;
        }
        pGpsGeofenceCallbacks->geofence_status_callback(status, p_loc);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gps_geofence_add_callback(int32_t geofence_id, int32_t status)
{
    ENTRY_LOG();
    if (NULL != pGpsGeofenceCallbacks && NULL != pGpsGeofenceCallbacks->geofence_add_callback) {
        pGpsGeofenceCallbacks->geofence_add_callback(geofence_id, status);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gps_geofence_remove_callback(int32_t geofence_id, int32_t status)
{
    ENTRY_LOG();
    if (NULL != pGpsGeofenceCallbacks && NULL != pGpsGeofenceCallbacks->geofence_remove_callback) {
        pGpsGeofenceCallbacks->geofence_remove_callback(geofence_id, status);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gps_geofence_pause_callback(int32_t geofence_id, int32_t status)
{
    ENTRY_LOG();
    if (NULL != pGpsGeofenceCallbacks && NULL != pGpsGeofenceCallbacks->geofence_pause_callback) {
        pGpsGeofenceCallbacks->geofence_pause_callback(geofence_id, status);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gps_geofence_resume_callback(int32_t geofence_id, int32_t status)
{
    ENTRY_LOG();
    if (NULL != pGpsGeofenceCallbacks && NULL != pGpsGeofenceCallbacks->geofence_resume_callback) {
        pGpsGeofenceCallbacks->geofence_resume_callback(geofence_id, status);
    }
    EXIT_LOG(%s, VOID_RET);
}

static pthread_t local_geofence_create_thread(const char* name, void (*start)(void *), void* arg)
{
    ENTRY_LOG();
    pthread_t retVal = -1;
    if (NULL != pGpsGeofenceCallbacks && NULL != pGpsGeofenceCallbacks->create_thread_cb) {
        retVal = pGpsGeofenceCallbacks->create_thread_cb(name, start, arg);
    }
    EXIT_LOG(%d, (int)retVal);
    return retVal;
}

static void loc_geofence_init(GpsGeofenceCallbacks* callbacks)
{
    ENTRY_LOG();
    if (NULL != pLocGpsGeofencingInterface && NULL != pLocGpsGeofencingInterface->init) {
        LocGpsGeofenceCallbacks* cbs = NULL;
        if (NULL != callbacks) {
            memcpy(&sGpsGeofenceCallbacks, callbacks, sizeof(GpsGeofenceCallbacks));
            pGpsGeofenceCallbacks = &sGpsGeofenceCallbacks;
            cbs = &sLocGpsGeofenceCallbacks;
        }
        pLocGpsGeofencingInterface->init(cbs);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void loc_add_geofence_area(int32_t geofence_id, double latitude, double longitude,
    double radius_meters, int last_transition, int monitor_transitions,
    int notification_responsiveness_ms, int unknown_timer_ms)
{
    ENTRY_LOG();
    if (NULL != pLocGpsGeofencingInterface && NULL != pLocGpsGeofencingInterface->add_geofence_area) {
        pLocGpsGeofencingInterface->add_geofence_area(geofence_id, latitude, longitude,
                radius_meters, last_transition, monitor_transitions,
                notification_responsiveness_ms, unknown_timer_ms);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void loc_pause_geofence(int32_t geofence_id)
{
    ENTRY_LOG();
    if (NULL != pLocGpsGeofencingInterface && NULL != pLocGpsGeofencingInterface->pause_geofence) {
        pLocGpsGeofencingInterface->pause_geofence(geofence_id);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void loc_resume_geofence(int32_t geofence_id, int monitor_transitions)
{
    ENTRY_LOG();
    if (NULL != pLocGpsGeofencingInterface && NULL != pLocGpsGeofencingInterface->resume_geofence) {
        pLocGpsGeofencingInterface->resume_geofence(geofence_id, monitor_transitions);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void loc_remove_geofence_area(int32_t geofence_id)
{
    ENTRY_LOG();
    if (NULL != pLocGpsGeofencingInterface && NULL != pLocGpsGeofencingInterface->remove_geofence_area) {
        pLocGpsGeofencingInterface->remove_geofence_area(geofence_id);
    }
    EXIT_LOG(%s, VOID_RET);
}

/*===========================================================================
  Implementation of functions for sGpsMeasurementInterface
===========================================================================*/
static void local_gps_measurement_callback(LocGpsData* data)
{
    ENTRY_LOG();
    if (NULL != pGpsMeasurementCallbacks && NULL != pGpsMeasurementCallbacks->measurement_callback) {
        GpsData gps_data;
        GpsData* p_gps_data = NULL;
        if (NULL != data)
        {
            Q2A_GpsData(*data, gps_data);
            p_gps_data = &gps_data;
        }
        pGpsMeasurementCallbacks->measurement_callback(p_gps_data);
    }
    EXIT_LOG(%s, VOID_RET);
}

static void local_gnss_measurement_callback(LocGnssData* data)
{
    ENTRY_LOG();
    if (NULL != pGpsMeasurementCallbacks && NULL != pGpsMeasurementCallbacks->gnss_measurement_callback) {
        GnssData gnss_data;
        GnssData* p_gnss_data = NULL;
        if (NULL != data)
        {
            Q2A_GnssData(*data, gnss_data);
            p_gnss_data = &gnss_data;
        }
        pGpsMeasurementCallbacks->gnss_measurement_callback(p_gnss_data);
    }
    EXIT_LOG(%s, VOID_RET);
}

static int loc_gps_measurement_init(GpsMeasurementCallbacks* callbacks)
{
    ENTRY_LOG();

    int retVal = -1;
    LocGpsMeasurementCallbacks* cbs = NULL;
    if (NULL != callbacks)
    {
        memcpy(&sGpsMeasurementCallbacks, callbacks, sizeof(GpsMeasurementCallbacks));
        pGpsMeasurementCallbacks = &sGpsMeasurementCallbacks;
        cbs = &sLocGpsMeasurementCallbacks;

        if (NULL != pLocGpsMeasurementInterface && NULL != pLocGpsMeasurementInterface->init) {
            retVal = pLocGpsMeasurementInterface->init(cbs);
        }
    }

    EXIT_LOG(%d, retVal);
    return retVal;
}

static void loc_gps_measurement_close()
{
    ENTRY_LOG();

    if (NULL != pLocGpsMeasurementInterface && NULL != pLocGpsMeasurementInterface->close) {
        pLocGpsMeasurementInterface->close();
    }

    EXIT_LOG(%s, VOID_RET);
}

/*===========================================================================
  Implementation of functions for sGnssConfigurationInterface
===========================================================================*/
static void loc_configuration_update(const char* config_data, int32_t length)
{
    ENTRY_LOG();

    if (NULL != pLocGnssConfigurationInterface &&
            NULL != pLocGnssConfigurationInterface->configuration_update) {
        pLocGnssConfigurationInterface->configuration_update(config_data, length);
    }

    EXIT_LOG(%s, VOID_RET);
}

