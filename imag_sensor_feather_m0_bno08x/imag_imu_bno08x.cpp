/* imag_imu_bno08x.cpp
 * 
 * imagination sensor firmware
 * hillcrest bno08x interface module
 * 
 * 2021-2034 rumori
 */

#include "imag_imu_bno08x.h"
#include "imag_debug.h"

// redefine DBG output macros for this module only
#if ! IMAG_IMU_DEBUG
#undef DBG
#define DBG       ;
#undef DBGLN
#define DBGLN     ;
#undef DBGN
#define DBGN      ;
#undef DBGNLN
#define DBGNLN    ;
#undef DBGHEX
#define DBGHEX    ;
#endif // #if ! IMAG_IMU_DEBUG

namespace imag::imu
{

BNO08x::BNO08x (uint8_t resetPin, uint8_t intPin)
    : bno08x (resetPin),
      initialised (false),
      lastType (DataType::none),
      reliability (0),
      accuracy (-1.0f),
      sourceOfReliability (DataType::none),
      sourceOfAccuracy (DataType::none),
      calibrating (false)
{
    // interrupt pin, only configure for now (unused)
    pinMode (intPin, INPUT_PULLUP);
    queryRates.fill (100);
}


bool BNO08x::init (uint8_t i2cAddr)
{
    initialised = false;

    // init i2c, checks whether chip is present
    if (! bno08x.begin_I2C (i2cAddr))
    {
	DBGLN ("BNO08x: sensor not found");
	return false;
    }

    // make sure reset took place
    if (! bno08x.wasReset())
    {
	DBGLN("BNO08x: reset flag not set after i2c init (involving a hardware reset)");
	return false;
    }
    
    // The following softwareReset() seems to be a critical point where warn start sometimes fails.
    // delay (100);
    
    // seems to prevent hangs of following setDefaultAutoCalibration() at warm start
    if (! bno08x.softwareReset())
    {
        DBGLN("BNO08x: error soft-resetting sensor");
        return false;
    }

    return reinit();
}


bool BNO08x::read()
{
    // restore reports if sensor was reset
    if (bno08x.wasReset() && ! reinit())
    {
        DBGLN ("BNO08x: reinit after reset failed");
        return false;
    }

    // query sensor data, return false if none available
    if (! bno08x.getSensorEvent (&sensorValue))
    {
        // DBGLN ("BNO08x: getSensorEvent() did not yield any data");
        return false;
    }

    // TODO: check for sequence number gap

    // set data type
    lastType = static_cast<DataType> (sensorValue.sensorId);
    
    if (! isSupportedDataType (lastType))
    {
        lastType = DataType::none;
        DBGLN("BNO08x: received unsupported report type");
        return false;
    }

    // store reliability
    if (lastType == sourceOfReliability)
    {
        reliability = sensorValue.status & 0x03;
        DBG("BNO08x: received reliability "); DBGNLN(reliability);
    }

    // store accuracy
    if (lastType == sourceOfAccuracy)
    {
        if (lastType == DataType::rotation ||
            lastType == DataType::rotationGeo ||
            lastType == DataType::rotationArvr)
            accuracy = sensorValue.un.rotationVector.accuracy;
        else
            accuracy = -1.0f;
        DBG("BNO08x: received accuracy "); DBGNLN(accuracy * 180.0 / PI);
    }

    return true;
}


bool BNO08x::getLastData (Quaternion& rotation)
{
    if (! isAnyRotationDataType (lastType))
        return false;

    rotation.w = sensorValue.un.rotationVector.real;
    rotation.x = sensorValue.un.rotationVector.i;
    rotation.y = sensorValue.un.rotationVector.j;
    rotation.z = sensorValue.un.rotationVector.k;

    return true;
}


bool BNO08x::setDataTypesToQuery (const std::initializer_list<DataType>& dataTypes)
{
    auto res = true;

    typesToQuery.clear();
  
    for (auto type : dataTypes)
    {
        if (isSupportedDataType (type))
            typesToQuery.push_back (type);
        else
            res = false;
    }

    // try to update supported types even if also unsupported were requested
    return updateDataTypesToQuery() && res;
}


bool BNO08x::setReorientation (const Quaternion& newReorientation)
{
    reorientation = newReorientation;
    return updateReorientation();
}


bool BNO08x::beginCalibration()
{
    auto res = disableAllSensors();

    // enable dynamic calibration according to BNO08x Sensor Calibration Procedure document
    res &= bno08x.setSensorsPerformingDynamicCalibration (SH2_CAL_ACCEL | SH2_CAL_GYRO | SH2_CAL_MAG);

    // enable only sensors according to BNO08x Sensor Calibration Procedure document
    if (! (bno08x.enableReport (SH2_GAME_ROTATION_VECTOR) &&            // game rotation
           bno08x.enableReport (SH2_MAGNETIC_FIELD_CALIBRATED, 20000))) // magnetic field @ 50Hz
    {
        res = false;
        DBGLN("BNO08x: error enabling sensor reports for calibration");
    }

    // get reliability from magnetic field
    sourceOfReliability = DataType::mag;

    if (res)
        calibrating = true;
  
    return res;
}


bool BNO08x::endCalibration()
{
    calibrating = false;

    // set back to default mode
    return reinit();
}


bool BNO08x::saveCalibration()
{
    if (! bno08x.saveDynamicCalibrationData())
    {
        DBGLN("BNO08x: error saving dynamic calibration data");
        return false;
    }

    delay (250);

    return reinit();
}


bool BNO08x::clearCalibration()
{
    if (! bno08x.clearDynamicCalibrationData())
    {
        DBGLN("BNO08x: error clearing dynamic calibration data");
        return false;
    }

    // this will have caused a reset, so reinit.
    delay (250);

    return reinit();
}


void BNO08x::printCalibrationReliability()
{
#if IMAG_IMU_DEBUG
    DBG("BNO08x: current calibration reliability: ");
    switch (reliability)
    {
    case 0:
        DBGLN("Unreliable");
        break;

    case 1:
        DBGLN("Low");
        break;
    
    case 2:
        DBGLN("Medium");
        break;

    case 3:
        DBGLN("High");
        break;
    }
#endif // IMAG_IMU_DEBUG
}


bool BNO08x::printSensorsPerformingDynamicCalibration()
{
#if IMAG_IMU_DEBUG
    uint8_t sensors;

    if (! bno08x.getSensorsPerformingDynamicCalibration(sensors))
    {
        DBGLN("BNO08x: querying sensor dynamic calibration state failed");
        return false;
    }

    DBGLN("BNO08x: sensors performing dynamic calibration:");
    DBG("BNO08x: Accelerometer: "); DBGLN(sensors & SH2_CAL_ACCEL  ? "on" : "off");
    DBG("BNO08x: Gyroscope    : "); DBGLN(sensors & SH2_CAL_GYRO   ? "on" : "off");
    DBG("BNO08x: Magnetometer : "); DBGLN(sensors & SH2_CAL_MAG    ? "on" : "off");
    DBG("BNO08x: Planar       : "); DBGLN(sensors & SH2_CAL_PLANAR ? "on" : "off");
#endif // IMAG_IMU_DEBUG

    return true;
}


bool BNO08x::reinit()
{
    initialised = false;

    // set sensors which do auto-calibration
    if (! setDefaultAutoCalibration())
    {
        DBGLN("BNO08x: error setting default auto calibration sensors");
        return false;
    }

    // enable reports
    if (! updateDataTypesToQuery())
    {
        DBGLN("BNO08x: could not set reports");
        return false;
    }

    // set reorientation
    if (! updateReorientation())
    {
        DBGLN("BNO08x: error setting reorientation to sensor");
        return false;
    }

    return initialised = true;
}


bool BNO08x::updateDataTypesToQuery (const std::vector<DataType>& newTypesToQuery)
{
    auto res = true;

    // disable all sensors
    res = disableAllSensors();

    // enable requested sensors
    for (auto type : newTypesToQuery)
    {
        const auto typeInt = static_cast<int> (type);
        
        if (! bno08x.enableReport (typeInt, 1000000L / static_cast<uint32_t> (queryRates[typeInt])))
        {
            DBG("BNO08x: error while enabling sensor for data type "); DBGLN(typeInt);
            res = false;
        }
    }

    // get reliability and accuracy from first type in query list
    sourceOfReliability = newTypesToQuery.size() > 0 ? newTypesToQuery[0] : DataType::none;
    sourceOfAccuracy = sourceOfReliability;

    return res;
}


bool BNO08x::disableAllSensors()
{
    auto res = true;

    // disable all sensors
    for (size_t type = 0; type < static_cast<size_t> (DataType::totalNum); ++type)
    {
        if (! bno08x.enableReport (type, 0))
        {
            DBG("BNO08x: error while disabling sensor for data type "); DBGLN(type);
            res = false;
        }
    }

    return res;
}


bool BNO08x::setTare (bool tareFull)
{
    sh2_TareBasis_t basis = SH2_TARE_BASIS_ROTATION_VECTOR; // default
    uint8_t axes = SH2_TARE_Z;

    if (typesToQuery.size() > 0)
    {
        if (typesToQuery[0] == DataType::rotationGame)
            basis = SH2_TARE_BASIS_GAMING_ROTATION_VECTOR;
        else if (typesToQuery[0] == DataType::rotationGeo)
            basis = SH2_TARE_BASIS_GEOMAGNETIC_ROTATION_VECTOR;
    }

    if (tareFull)
        axes = SH2_TARE_X | SH2_TARE_Y | SH2_TARE_Z;

    if (! bno08x.setTare (axes, basis))
    {
        DBGLN("BNO08x: error setting tare");
        return false;
    }

    return true;
}


bool BNO08x::setDefaultAutoCalibration()
{
    // according to recommendation in BNO08x Sensor Calibration Procedure document
    return bno08x.setSensorsPerformingDynamicCalibration (SH2_CAL_ACCEL);
}


bool BNO08x::updateReorientation()
{
    auto sh2Quat = quatToSh2Quat (reorientation);
    
    return sh2_setReorientation (&sh2Quat) == SH2_OK;
}

} // namespace imag
