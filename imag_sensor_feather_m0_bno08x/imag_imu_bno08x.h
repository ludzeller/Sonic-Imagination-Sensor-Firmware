/* Imag_imu_bno08x.h
 * 
 * imagination sensor firmware
 * hillcrest bno08x interface module
 * 
 * 2021-2024 rumori
 */

#pragma once

#include "Adafruit_BNO08x_ext.h"

#include <Arduino_Helpers.h>
#include <AH/Math/Quaternion.hpp>

#include <array>
#include <vector>
#include <algorithm>

namespace imag::imu
{
// data type aliases that can be queried from sensor
enum class DataType
{
    none                = -1,
    accel               = SH2_ACCELEROMETER,
    gyro                = SH2_GYROSCOPE_CALIBRATED,
    mag                 = SH2_MAGNETIC_FIELD_CALIBRATED,
    rotation            = SH2_ROTATION_VECTOR,
    rotationGame        = SH2_GAME_ROTATION_VECTOR,
    rotationGeo         = SH2_GEOMAGNETIC_ROTATION_VECTOR,
    rotationArvr        = SH2_ARVR_STABILIZED_RV,
    rotationGameArvr    = SH2_ARVR_STABILIZED_GRV,
    detectTap           = SH2_TAP_DETECTOR,
    detectShake         = SH2_SHAKE_DETECTOR,
    detectStability     = SH2_STABILITY_DETECTOR,
    detectStep          = SH2_STEP_DETECTOR,
    countStep           = SH2_STEP_COUNTER,
    classStability      = SH2_STABILITY_CLASSIFIER,
    classActivity       = SH2_PERSONAL_ACTIVITY_CLASSIFIER,
    significantMotion   = SH2_SIGNIFICANT_MOTION,

    totalNum            = SH2_MAX_SENSOR_ID + 1
};

// convenience method for datatype classes
constexpr bool isAnyRotationDataType (DataType dataType)
{
    return dataType == DataType::rotation ||
        dataType == DataType::rotationGame ||
        dataType == DataType::rotationGeo ||
        dataType == DataType::rotationArvr ||
        dataType == DataType::rotationGameArvr;
};

// check whether a specific data type is supported by this implementation
constexpr bool isSupportedDataType (DataType dataType)
{
    // rotation and single sensor types (for calibration) so far
    return isAnyRotationDataType (dataType) ||
        dataType == DataType::accel ||
        dataType == DataType::gyro ||
        dataType == DataType::mag;
}


class BNO08x
{
public:
    // constructor
    BNO08x (uint8_t resetPin, uint8_t intPin);
  
    // check sensor presence and initialise
    bool init (uint8_t i2cAddr);

    // query data from sensor if available and set type member
    bool read();

    // get type of previously queried data
    DataType getLastDataType() const { return lastType; }

    // return previously received data: Quaternion overload
    bool getLastData (Quaternion& rotation);

    // set data types to query from sensor
    bool setDataTypesToQuery (const std::initializer_list<DataType>& dataTypes);

    // get data types currently queried by sensor
    const std::vector<DataType>& getDataTypesToQuery() const { return typesToQuery; }

    // tare methods
    bool setTareFull() { return setTare (true); }
    bool setTareHeading() { return setTare(); }
    bool resetTare() { return updateReorientation(); }

    bool setTareTilt() { return false; }
    bool saveTare() { return false; }

    // reorientation methods
    bool setReorientation (const Quaternion& newReorientation);
    const Quaternion& getReorientation() const { return reorientation; }

    // sensor calibration methods, not supported by default
    bool beginCalibration();
    bool endCalibration();
    bool saveCalibration();
    bool isCalibrating() const { return calibrating; }

    // set sensor data rate
    bool setDataRate (int rate) { return false; } // not supported by default

    // get current sensor/fusion reliability, 0.0..1.0
    float getCurrentReliability() const { return reliability / 3.0f; }

    // get current sensor/fusion accuracy, radians, negative if invalid
    float getCurrentAccuracy() const { return accuracy; }

    bool isInitialised() const { return initialised; }

    // debug printers
    void printCalibrationReliability();
    bool printSensorsPerformingDynamicCalibration();

private:
    bool reinit();
    bool updateDataTypesToQuery() { return updateDataTypesToQuery (typesToQuery); }
    bool updateDataTypesToQuery (const std::vector<DataType>& newTypesToQuery);
    bool disableAllSensors();
    bool setTare (bool tareFull = false);

    // set default auto calibration mode
    bool setDefaultAutoCalibration();

    // send current reorientation to sensor
    bool updateReorientation();

    // quaternion translation helpers
    static sh2_Quaternion_t quatToSh2Quat (const Quaternion& quat) { return { quat.x, quat.y, quat.z, quat.w }; }
    static Quaternion sh2QuatToQuat (const sh2_Quaternion_t& sh2Quat)
    {
        return { float (sh2Quat.w), float (sh2Quat.x), float (sh2Quat.y), float (sh2Quat.z) };
    }

    // bno08x interface object
    Adafruit_BNO08x_ext bno08x;

    // initialised flag
    bool initialised;

    // sensor value last read
    sh2_SensorValue_t sensorValue;

    // type of last queried data
    DataType lastType;

    // reorientation quaternion
    Quaternion reorientation;

    // data types to query
    std::vector<DataType> typesToQuery;

    // reliability sent with last matching sensor data
    int reliability;

    // accuracy sent with last sensor data or negative if invalid
    float accuracy;

    // sensor report type from which to set reliability member
    DataType sourceOfReliability;

    // sensor report type from which to set accuracy member (if supported)
    DataType sourceOfAccuracy;

    // query rates in Hz per data type
    /* one 16-bit number per available sensor return type
       wastes a few bytes as only a few types are used, but, well...
    */
    std::array<uint16_t, static_cast<size_t> (DataType::totalNum)> queryRates;

    // sensor value sequence
    /* one 8-bit number per available sensor return type
       wastes a few bytes as only a few types are used, but, well...
    */
    std::array<uint8_t, static_cast<size_t> (DataType::totalNum)> sequenceNumbers;

    // calibration mode flag
    bool calibrating;
}; // class BNO08x

} // namespace imag::imu
