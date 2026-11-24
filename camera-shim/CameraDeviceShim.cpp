#include "CameraDeviceShim.h"
#include "CameraDeviceSessionShim.h"
#include "CameraProviderShim.h"

#include <aidl/android/hardware/camera/common/TorchModeStatus.h>

#include <system/camera_metadata.h>

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android-base/file.h>

#include <cerrno>
#include <cinttypes>
#include <cstring>

namespace aidl::android::hardware::camera::provider::shim {


using ::aidl::android::hardware::camera::device::CameraMetadata;
using ::aidl::android::hardware::camera::common::CameraResourceCost;

CameraDeviceShim::CameraDeviceShim(int cameraId, camera_module_t* module)
    : mCameraId(cameraId), mModule(module), mStaticMetadata(nullptr), mTorchOn(false) {}

void CameraDeviceShim::setProvider(CameraProviderShim* provider) {
    std::lock_guard<std::mutex> lock(mLock);
    mProvider = provider;
}

CameraDeviceShim::~CameraDeviceShim() {
    std::lock_guard<std::mutex> lock(mLock);
    if (mStaticMetadata != nullptr) {
        free_camera_metadata(mStaticMetadata);
    }
}

static void addInt32(camera_metadata_t* m, uint32_t tag, const int32_t* v, size_t count) {
    add_camera_metadata_entry(m, tag, v, count);
}

static void addInt64(camera_metadata_t* m, uint32_t tag, const int64_t* v, size_t count) {
    add_camera_metadata_entry(m, tag, v, count);
}

static void addFloat(camera_metadata_t* m, uint32_t tag, const float* v, size_t count) {
    add_camera_metadata_entry(m, tag, v, count);
}

static void addByte(camera_metadata_t* m, uint32_t tag, const uint8_t* v, size_t count) {
    add_camera_metadata_entry(m, tag, v, count);
}

static void addRational(camera_metadata_t* m, uint32_t tag, const camera_metadata_rational_t* v,
                        size_t count) {
    add_camera_metadata_entry(m, tag, v, count);
}

// Builds the static camera characteristics for one mako camera by querying the
// HAL 1.0 module (get_camera_info) and, where needed, the device parameters.
camera_metadata_t* CameraDeviceShim::buildStaticCharacteristicsLocked() {
    if (mStaticMetadata != nullptr) {
        return mStaticMetadata;
    }

    // The legacy HAL initializes its internal camera handles during
    // get_number_of_cameras(); get_camera_info() returns -1 without it.
    if (mModule->get_number_of_cameras() <= 0) {
        ALOGE("shim: get_number_of_cameras failed");
        return nullptr;
    }
    struct camera_info info;
    memset(&info, 0, sizeof(info));
    int gci = mModule->get_camera_info(mCameraId, &info);
    if (gci != 0) {
        ALOGE("shim: get_camera_info(%d) failed: %d", mCameraId, gci);
        return nullptr;
    }
    ALOGI("shim: camera %d info facing=%d orientation=%d", mCameraId, info.facing,
          info.orientation);

    const size_t kEntryCap = 256;
    const size_t kDataCap = 8192;
    camera_metadata_t* m = allocate_camera_metadata(kEntryCap, kDataCap);
    if (m == nullptr) {
        ALOGE("shim: allocate_camera_metadata failed");
        return nullptr;
    }

    const int32_t facing =
            (info.facing == CAMERA_FACING_FRONT) ? ANDROID_LENS_FACING_FRONT
                                                : ANDROID_LENS_FACING_BACK;
    addByte(m, ANDROID_LENS_FACING, reinterpret_cast<const uint8_t*>(&facing), 1);

    const int32_t orientation = info.orientation;
    addInt32(m, ANDROID_SENSOR_ORIENTATION, &orientation, 1);

    // Sensor / array geometry (mako: Sony IMX111 8MP rear, IMX119 1.3MP front)
    const int32_t pixelArray[] = {mCameraId == 0 ? 3280 : 1392,
                                  mCameraId == 0 ? 2464 : 1048};
    const int32_t activeArray[] = {0, 0, mCameraId == 0 ? 3264 : 1344,
                                   mCameraId == 0 ? 2448 : 1024};
    addInt32(m, ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE, pixelArray, 2);
    addInt32(m, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, activeArray, 4);
    // Capture timestamps come from CLOCK_MONOTONIC, not elapsed realtime.
    const uint8_t timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_UNKNOWN;
    addByte(m, ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE, &timestampSource, 1);
    const float physicalSize[] = {4.6f, 3.45f};
    addFloat(m, ANDROID_SENSOR_INFO_PHYSICAL_SIZE, physicalSize, 2);
    const uint8_t aeLockAvailable = 1;
    addByte(m, ANDROID_CONTROL_AE_LOCK_AVAILABLE, &aeLockAvailable, 1);
    const uint8_t awbLockAvailable = 1;
    addByte(m, ANDROID_CONTROL_AWB_LOCK_AVAILABLE, &awbLockAvailable, 1);
    const int32_t distortionModes[] = {ANDROID_DISTORTION_CORRECTION_MODE_OFF};
    addByte(m, ANDROID_DISTORTION_CORRECTION_AVAILABLE_MODES,
            reinterpret_cast<const uint8_t*>(distortionModes), 1);
    const int32_t videoStabModes[] = {ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF};
    addByte(m, ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
            reinterpret_cast<const uint8_t*>(videoStabModes), 1);
    const int32_t focusCalibration =
            ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    addByte(m, ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION,
            reinterpret_cast<const uint8_t*>(&focusCalibration), 1);
    addInt32(m, ANDROID_SENSOR_ORIENTATION, &orientation, 1);
    const int64_t exposureRange[] = {10000, 33333333};
    addInt64(m, ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE, exposureRange, 2);
    const int32_t sensitivityRange[] = {100, 1600};
    addInt32(m, ANDROID_SENSOR_INFO_SENSITIVITY_RANGE, sensitivityRange, 2);
    const int32_t colorFilter = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    addByte(m, ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
            reinterpret_cast<const uint8_t*>(&colorFilter), 1);
    const int32_t testPattern = ANDROID_SENSOR_TEST_PATTERN_MODE_OFF;
    addInt32(m, ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES, &testPattern, 1);

    // Lens
    const float focalLength = 4.6f;
    addFloat(m, ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS, &focalLength, 1);
    const float apertures[] = {2.4f};
    addFloat(m, ANDROID_LENS_INFO_AVAILABLE_APERTURES, apertures, 1);
    const int32_t opticalStab[] = {ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF};
    addByte(m, ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
            reinterpret_cast<const uint8_t*>(opticalStab), 1);
    const float minFocusDistance[] = {10.0f};
    addFloat(m, ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE, minFocusDistance, 1);
    const int32_t hyperfocal = 0;
    addFloat(m, ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE,
             reinterpret_cast<const float*>(&hyperfocal), 1);
    const int32_t shadingMapModes[] = {ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF};
    addByte(m, ANDROID_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES,
            reinterpret_cast<const uint8_t*>(shadingMapModes), 1);
    const uint8_t shadingApplied = 0;
    addByte(m, ANDROID_SENSOR_INFO_LENS_SHADING_APPLIED, &shadingApplied, 1);

    // Scaler: stream configurations for preview + picture sizes.
    // mako bring-up: the legacy HAL delivers preview through fixed 640x480
    // window buffers; every advertised size is CPU-converted per frame in this
    // process.  Sizes larger than 640x480 require upscaling (~2 Mpx/frame at
    // 1080p) which starves the request pipeline (black preview, watchdog
    // storms, recordings never start).  Advertise only downscale-or-crop sizes.
    const int32_t previewSizes[][2] = {
            {640, 480},  {480, 640},  {384, 640},  {360, 640},  {352, 288},
            {320, 480},  {288, 480},  {240, 400},  {240, 320},  {176, 288},
            {192, 320},  {144, 176},  {128, 160},
    };
    const int32_t pictureSizes[][2] = {
            {3264, 2448}, {3264, 1836}, {3200, 2400}, {2592, 1944}, {2592, 1456},
            {2048, 1536}, {2048, 1152}, {1600, 1200}, {1440, 1080}, {1280, 960},
            {1280, 720},  {1024, 768},  {960, 720},   {800, 600},   {720, 480},
            {640, 480},   {640, 368},   {576, 432},   {480, 320},   {384, 288},
            {352, 288},   {320, 240},   {240, 160},   {176, 144},
    };

    const int32_t kStreamFormatPreview = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
    const int32_t kStreamFormatYuv = HAL_PIXEL_FORMAT_YCbCr_420_888;
    const int32_t kStreamFormatJpeg = HAL_PIXEL_FORMAT_BLOB;

    std::vector<int32_t> streamConfigs;
    std::vector<int64_t> minFrameDurations;
    std::vector<int64_t> stallDurations;
    for (const auto& s : previewSizes) {
        streamConfigs.insert(streamConfigs.end(),
                             {kStreamFormatPreview, s[0], s[1],
                              ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT});
        streamConfigs.insert(streamConfigs.end(),
                             {kStreamFormatYuv, s[0], s[1],
                              ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT});
        minFrameDurations.insert(minFrameDurations.end(),
                                 {kStreamFormatPreview, s[0], s[1], 33333333LL});
        minFrameDurations.insert(minFrameDurations.end(),
                                 {kStreamFormatYuv, s[0], s[1], 33333333LL});
        stallDurations.insert(stallDurations.end(), {kStreamFormatPreview, s[0], s[1], 0});
    }
    for (const auto& s : pictureSizes) {
        // IMX119 accepts still captures up to 1280x960 through the legacy HAL.
        if (info.facing == CAMERA_FACING_FRONT && (s[0] > 1280 || s[1] > 960)) {
            continue;
        }
        streamConfigs.insert(streamConfigs.end(),
                             {kStreamFormatJpeg, s[0], s[1],
                              ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT});
        streamConfigs.insert(streamConfigs.end(),
                             {kStreamFormatYuv, s[0], s[1],
                              ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT});
        minFrameDurations.insert(minFrameDurations.end(),
                                 {kStreamFormatJpeg, s[0], s[1], 500000000LL});
        stallDurations.insert(stallDurations.end(), {kStreamFormatJpeg, s[0], s[1], 1000000000});
    }
    addInt32(m, ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, streamConfigs.data(),
             streamConfigs.size());
    addInt64(m, ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS, minFrameDurations.data(),
             minFrameDurations.size());
    addInt64(m, ANDROID_SCALER_AVAILABLE_STALL_DURATIONS, stallDurations.data(),
             stallDurations.size());

    // The shim does not apply crop-region requests. Expose only unity zoom.
    const float maxDigitalZoom = 1.0f;
    addFloat(m, ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM, &maxDigitalZoom, 1);
    const int32_t cropType = ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY;
    addByte(m, ANDROID_SCALER_CROPPING_TYPE, reinterpret_cast<const uint8_t*>(&cropType), 1);

    // Control (AE/AF/AWB)
    const int32_t aeModes[] = {ANDROID_CONTROL_AE_MODE_OFF, ANDROID_CONTROL_AE_MODE_ON};
    addByte(m, ANDROID_CONTROL_AE_AVAILABLE_MODES, reinterpret_cast<const uint8_t*>(aeModes), 2);
    const int32_t fpsRanges[][2] = {{15, 30}, {15, 24}, {10, 30},
                                    {30, 30}, {24, 24}, {20, 20},
                                    {15, 15}, {7, 7}};
    addInt32(m, ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES, &fpsRanges[0][0], 16);
    const int32_t antibandingModes[] = {ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF,
                                        ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ,
                                        ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ,
                                        ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO};
    addByte(m, ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
            reinterpret_cast<const uint8_t*>(antibandingModes), 4);
    const int32_t afModes[] = {ANDROID_CONTROL_AF_MODE_OFF,
                               ANDROID_CONTROL_AF_MODE_AUTO,
                               ANDROID_CONTROL_AF_MODE_MACRO,
                               ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO,
                               ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE};
    addByte(m, ANDROID_CONTROL_AF_AVAILABLE_MODES, reinterpret_cast<const uint8_t*>(afModes), 5);
    const int32_t awbModes[] = {ANDROID_CONTROL_AWB_MODE_OFF, ANDROID_CONTROL_AWB_MODE_AUTO,
                                ANDROID_CONTROL_AWB_MODE_INCANDESCENT,
                                ANDROID_CONTROL_AWB_MODE_FLUORESCENT,
                                ANDROID_CONTROL_AWB_MODE_WARM_FLUORESCENT,
                                ANDROID_CONTROL_AWB_MODE_DAYLIGHT,
                                ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT,
                                ANDROID_CONTROL_AWB_MODE_TWILIGHT, ANDROID_CONTROL_AWB_MODE_SHADE};
    addByte(m, ANDROID_CONTROL_AWB_AVAILABLE_MODES, reinterpret_cast<const uint8_t*>(awbModes),
            9);
    const int32_t sceneModes[] = {ANDROID_CONTROL_SCENE_MODE_DISABLED,
                                  ANDROID_CONTROL_SCENE_MODE_BEACH,
                                  ANDROID_CONTROL_SCENE_MODE_CANDLELIGHT,
                                  ANDROID_CONTROL_SCENE_MODE_FIREWORKS,
                                  ANDROID_CONTROL_SCENE_MODE_LANDSCAPE,
                                  ANDROID_CONTROL_SCENE_MODE_NIGHT,
                                  ANDROID_CONTROL_SCENE_MODE_NIGHT_PORTRAIT,
                                  ANDROID_CONTROL_SCENE_MODE_SNOW,
                                  ANDROID_CONTROL_SCENE_MODE_SPORTS,
                                  ANDROID_CONTROL_SCENE_MODE_STEADYPHOTO,
                                  ANDROID_CONTROL_SCENE_MODE_SUNSET,
                                  ANDROID_CONTROL_SCENE_MODE_THEATRE};
    addByte(m, ANDROID_CONTROL_AVAILABLE_SCENE_MODES,
            reinterpret_cast<const uint8_t*>(sceneModes), 13);
    const int32_t effectModes[] = {ANDROID_CONTROL_EFFECT_MODE_OFF};
    addByte(m, ANDROID_CONTROL_AVAILABLE_EFFECTS, reinterpret_cast<const uint8_t*>(effectModes),
            1);
    const int32_t regions[] = {0, 0, 0};
    addInt32(m, ANDROID_CONTROL_MAX_REGIONS, regions, 3);
    const int32_t aeCompensationRange[] = {-2000, 2000};
    addInt32(m, ANDROID_CONTROL_AE_COMPENSATION_RANGE, aeCompensationRange, 2);
    const camera_metadata_rational_t aeCompensationStep[] = {{1, 2}};
    addRational(m, ANDROID_CONTROL_AE_COMPENSATION_STEP, aeCompensationStep, 1);

    // Flash (rear camera only)
    const uint8_t flashAvailable = (mCameraId == 0) ? ANDROID_FLASH_INFO_AVAILABLE_TRUE
                                                    : ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    addByte(m, ANDROID_FLASH_INFO_AVAILABLE, &flashAvailable, 1);

    // JPEG
    const int32_t thumbnailSizes[][2] = {{512, 288}, {480, 320}, {320, 240}, {240, 180}, {0, 0}};
    addInt32(m, ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES, &thumbnailSizes[0][0], 10);
    const int32_t jpegMaxSize = 12000000;
    addInt32(m, ANDROID_JPEG_MAX_SIZE, &jpegMaxSize, 1);

    // Request pipeline
    const int32_t maxNumOutputStreams[] = {0, 4, 1};  // raw, processed, jpeg
    addInt32(m, ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS, maxNumOutputStreams, 3);
    const int32_t pipelineDepth = 2;
    addByte(m, ANDROID_REQUEST_PIPELINE_MAX_DEPTH,
            reinterpret_cast<const uint8_t*>(&pipelineDepth), 1);
    const int32_t partialResults = 1;
    addInt32(m, ANDROID_REQUEST_PARTIAL_RESULT_COUNT, &partialResults, 1);
    const int32_t maxInputStreams = 0;
    addInt32(m, ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS, &maxInputStreams, 1);
    const int32_t maxNumOutputRaw = 0;
    addInt32(m, ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS, &maxNumOutputRaw, 1);

    // Sync
    const int32_t maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL;
    addInt32(m, ANDROID_SYNC_MAX_LATENCY, &maxLatency, 1);

    // Statistics
    const int32_t faceDetectModes[] = {ANDROID_STATISTICS_FACE_DETECT_MODE_OFF,
                                       ANDROID_STATISTICS_FACE_DETECT_MODE_SIMPLE};
    addByte(m, ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
            reinterpret_cast<const uint8_t*>(faceDetectModes), 2);
    const int32_t maxFaces = 2;
    addInt32(m, ANDROID_STATISTICS_INFO_MAX_FACE_COUNT, &maxFaces, 1);
    const int32_t hotPixelModes[] = {ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE_OFF};
    addByte(m, ANDROID_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES,
            reinterpret_cast<const uint8_t*>(hotPixelModes), 1);
    const int32_t aberrationModes[] = {ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF};
    addByte(m, ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES,
            reinterpret_cast<const uint8_t*>(aberrationModes), 1);

    // Capabilities: legacy HAL 1.0 device, backward compatible with camera1.
    const uint8_t capabilities[] = {
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE};
    addByte(m, ANDROID_REQUEST_AVAILABLE_CAPABILITIES, capabilities, 1);

    // Info
    const int32_t hwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY;
    addByte(m, ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL,
            reinterpret_cast<const uint8_t*>(&hwLevel), 1);
    const int64_t version = 1;
    addInt64(m, ANDROID_INFO_VERSION, &version, 1);

    mStaticMetadata = m;
    return m;
}

ndk::ScopedAStatus CameraDeviceShim::getCameraCharacteristics(CameraMetadata* _aidl_return) {
    std::lock_guard<std::mutex> lock(mLock);
    camera_metadata_t* m = buildStaticCharacteristicsLocked();
    if (m == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    size_t size = get_camera_metadata_size(m);
    _aidl_return->metadata.resize(size);
    memcpy(_aidl_return->metadata.data(), m, size);
    size_t validateSize = size;
    int vres = validate_camera_metadata_structure(m, &validateSize);
    ALOGI("shim: camera %d characteristics size=%zu entries=%zu data=%zu validate=%d",
          mCameraId, size, get_camera_metadata_entry_count(m),
          get_camera_metadata_data_count(m), vres);
    camera_metadata_ro_entry_t scEntry;
    if (find_camera_metadata_ro_entry(m, ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                      &scEntry) == 0) {
        ALOGI("shim: camera %d stream configs count=%zu first=[%d,%d,%d,%d]", mCameraId,
              scEntry.count, scEntry.data.i32[0], scEntry.data.i32[1], scEntry.data.i32[2],
              scEntry.data.i32[3]);
    } else {
        ALOGI("shim: camera %d stream configs MISSING", mCameraId);
    }
    camera_metadata_ro_entry_t aaEntry;
    if (find_camera_metadata_ro_entry(m, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &aaEntry) == 0) {
        ALOGI("shim: camera %d active array count=%zu [%d,%d,%d,%d]", mCameraId, aaEntry.count,
              aaEntry.data.i32[0], aaEntry.data.i32[1], aaEntry.data.i32[2], aaEntry.data.i32[3]);
    }
    camera_metadata_ro_entry_t fpsEntry;
    if (find_camera_metadata_ro_entry(m, ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
                                      &fpsEntry) == 0) {
        ALOGI("shim: camera %d fps ranges count=%zu first=[%d,%d]", mCameraId, fpsEntry.count,
              fpsEntry.data.i32[0], fpsEntry.data.i32[1]);
    }
    camera_metadata_ro_entry_t durEntry;
    if (find_camera_metadata_ro_entry(m, ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS,
                                      &durEntry) == 0) {
        ALOGI("shim: camera %d durations count=%zu first=[%lld,%lld,%lld,%lld]", mCameraId,
              durEntry.count, (long long)durEntry.data.i64[0], (long long)durEntry.data.i64[1],
              (long long)durEntry.data.i64[2], (long long)durEntry.data.i64[3]);
    } else {
        ALOGI("shim: camera %d durations MISSING", mCameraId);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceShim::getPhysicalCameraCharacteristics(
    const std::string& /*physicalCameraId*/, CameraMetadata* _aidl_return) {
    _aidl_return->metadata.clear();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceShim::getResourceCost(CameraResourceCost* _aidl_return) {
    _aidl_return->resourceCost = 100;
    _aidl_return->conflictingDevices.clear();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceShim::isStreamCombinationSupported(
    const StreamConfiguration& /*streams*/, bool* _aidl_return) {
    *_aidl_return = true;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceShim::open(
    const std::shared_ptr<ICameraDeviceCallback>& callback,
    std::shared_ptr<ICameraDeviceSession>* _aidl_return) {
    std::lock_guard<std::mutex> lock(mLock);
    *_aidl_return =
            ::ndk::SharedRefBase::make<CameraDeviceSessionShim>(mModule, mCameraId, callback);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceShim::openInjectionSession(
    const std::shared_ptr<ICameraDeviceCallback>& /*callback*/,
    std::shared_ptr<::aidl::android::hardware::camera::device::ICameraInjectionSession>*
        /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus CameraDeviceShim::setTorchMode(bool on) {
    if (mCameraId != 0) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    CameraProviderShim* provider;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if (!::android::base::WriteStringToFile(on ? "255" : "0",
                "/sys/class/leds/spotlight/brightness")) {
            ALOGE("shim: setTorchMode(%d): spotlight write failed: %s",
                  on, strerror(errno));
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
        mTorchOn = on;
        provider = mProvider;
    }
    if (provider != nullptr) {
        using ::aidl::android::hardware::camera::common::TorchModeStatus;
        provider->notifyTorchModeChanged(
                "device@1.0/internal/0",
                static_cast<int32_t>(on ? TorchModeStatus::AVAILABLE_ON
                                        : TorchModeStatus::AVAILABLE_OFF));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceShim::turnOnTorchWithStrengthLevel(int32_t torchStrength) {
    return setTorchMode(torchStrength > 0);
}

ndk::ScopedAStatus CameraDeviceShim::getTorchStrengthLevel(int32_t* _aidl_return) {
    std::lock_guard<std::mutex> lock(mLock);
    *_aidl_return = mTorchOn ? 1 : 0;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceShim::constructDefaultRequestSettings(
    RequestTemplate type, CameraMetadata* _aidl_return) {
    (void)type;
    camera_metadata_t* m = allocate_camera_metadata(16, 256);
    const int32_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
    addByte(m, ANDROID_CONTROL_AE_MODE, reinterpret_cast<const uint8_t*>(&aeMode), 1);
    const int32_t afMode = ANDROID_CONTROL_AF_MODE_AUTO;
    addByte(m, ANDROID_CONTROL_AF_MODE, reinterpret_cast<const uint8_t*>(&afMode), 1);
    const int32_t awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
    addByte(m, ANDROID_CONTROL_AWB_MODE, reinterpret_cast<const uint8_t*>(&awbMode), 1);
    const int32_t antibanding = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
    addByte(m, ANDROID_CONTROL_AE_ANTIBANDING_MODE,
            reinterpret_cast<const uint8_t*>(&antibanding), 1);
    const int32_t fpsRange[] = {30000, 30000};
    addInt32(m, ANDROID_CONTROL_AE_TARGET_FPS_RANGE, fpsRange, 2);
    const int32_t aeCompensation = 0;
    addInt32(m, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &aeCompensation, 1);
    const int32_t flashMode = ANDROID_FLASH_MODE_OFF;
    addByte(m, ANDROID_FLASH_MODE, reinterpret_cast<const uint8_t*>(&flashMode), 1);
    const int32_t effect = ANDROID_CONTROL_EFFECT_MODE_OFF;
    addByte(m, ANDROID_CONTROL_EFFECT_MODE, reinterpret_cast<const uint8_t*>(&effect), 1);
    const int32_t sceneMode = ANDROID_CONTROL_SCENE_MODE_DISABLED;
    addByte(m, ANDROID_CONTROL_SCENE_MODE, reinterpret_cast<const uint8_t*>(&sceneMode), 1);
    const int32_t videoStab = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF;
    addByte(m, ANDROID_CONTROL_VIDEO_STABILIZATION_MODE,
            reinterpret_cast<const uint8_t*>(&videoStab), 1);
    const int32_t colorCorrection = ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF;
    addByte(m, ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
            reinterpret_cast<const uint8_t*>(&colorCorrection), 1);
    const int32_t jpegQuality = 90;
    addByte(m, ANDROID_JPEG_QUALITY, reinterpret_cast<const uint8_t*>(&jpegQuality), 1);
    const int32_t jpegRotation = 0;
    addInt32(m, ANDROID_JPEG_ORIENTATION, &jpegRotation, 1);
    const int32_t thumbnailSize[] = {512, 288};
    addInt32(m, ANDROID_JPEG_THUMBNAIL_SIZE, thumbnailSize, 2);
    size_t size = get_camera_metadata_size(m);
    _aidl_return->metadata.resize(size);
    memcpy(_aidl_return->metadata.data(), m, size);
    ALOGI("shim: defaults type=%d size=%zu", static_cast<int>(type), size);
    free_camera_metadata(m);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceShim::isStreamCombinationWithSettingsSupported(
    const StreamConfiguration& /*streams*/, bool* _aidl_return) {
    *_aidl_return = true;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceShim::getSessionCharacteristics(
    const StreamConfiguration& /*sessionConfig*/, CameraMetadata* _aidl_return) {
    return getCameraCharacteristics(_aidl_return);
}

}  // namespace aidl::android::hardware::camera::provider::shim
