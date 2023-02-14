/*
 *  Copyright (c) 2018, Sensirion AG <andreas.brauchli@sensirion.com>
 *  Copyright (c) 2015-2016, Johannes Winkelmann <jw@smts.ch>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of the Sensirion AG nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <cinttypes>

#include <Arduino.h>
#include "i2c_esp32.h"

// Forward declaration
class SHTSensorDriver;

/**
 * Official interface for Sensirion SHT Sensors
 */
class SHTSensor
{
public:
  /**
   * Enum of the supported Digital Sensirion SHT Sensors.
   * For analog sensors, see SHT3xAnalogSensor.
   * Using the special AUTO_DETECT sensor causes all i2c sensors to be
   * probed. The first matching sensor will then be used.
   */
  enum SHTSensorType
  {
    /** Automatically detect the sensor type (only i2c sensors listed above) */
    AUTO_DETECT,
    // i2c Sensors:
    /** SHT3x-DIS with ADDR (sensor pin 2) connected to VSS (default) */
    SHT3X,
    /** SHT3x-DIS with ADDR (sensor pin 2) connected to VDD */
    SHT3X_ALT,
    SHTC1,
    SHTW1,
    SHTW2
  };

  /**
   * Accuracy setting of measurement.
   * Not all sensors support changing the sampling accuracy.
   */
  enum SHTAccuracy
  {
    /** Highest repeatability at the cost of slower measurement */
    SHT_ACCURACY_HIGH,
    /** Balanced repeatability and speed of measurement */
    SHT_ACCURACY_MEDIUM,
    /** Fastest measurement but lowest repeatability */
    SHT_ACCURACY_LOW
  };

  /** Value reported by getHumidity() when the sensor is not initialized */
  static const float HUMIDITY_INVALID;
  /** Value reported by getTemperature() when the sensor is not initialized */
  static const float TEMPERATURE_INVALID;
  /**
   * Auto-detectable sensor types.
   * Note that the SHTW1 and SHTW2 share exactly the same driver as the SHTC1
   * and are thus not listed individually.
   */
  static const SHTSensorType AUTO_DETECT_SENSORS[];

  /**
   * Instantiate a new SHTSensor
   * By default, the i2c bus is queried for known SHT Sensors. To address
   * a specific sensor, set the `sensorType'.
   */
  SHTSensor(SHTSensorType sensorType = AUTO_DETECT)
      : mSensorType(sensorType),
        mSensor(NULL),
        mTemperature(SHTSensor::TEMPERATURE_INVALID),
        mHumidity(SHTSensor::HUMIDITY_INVALID)
  {
  }

  virtual ~SHTSensor()
  {
    cleanup();
  }

  /**
   * Initialize the sensor driver
   * To read out the sensor use readSample(), followed by getTemperature() and
   * getHumidity() to retrieve the values from the sample
   */
  bool init();

  /**
   * Read new values from the sensor
   * After the call, use getTemperature() and getHumidity() to retrieve the
   * values
   * Returns true if the sample was read and the values are cached
   */
  bool readSample();

  bool softReset();

  /**
   * Get the relative humidity in percent read from the last sample
   * Use readSample() to trigger a new sensor reading
   */
  float getHumidity() const
  {
    return mHumidity;
  }

  /**
   * Get the humidity in percent read from the last sample
   * Use readSample() to trigger a new sensor reading
   */
  float getTemperature() const
  {
    return mTemperature;
  }

  /**
   * Change the sensor accurancy, if supported by the sensor
   * Returns true if the accuracy was changed
   */
  bool setAccuracy(SHTAccuracy newAccuracy);

  // bool softReset();

private:
  void cleanup();

  SHTSensorType mSensorType;
  SHTSensorDriver *mSensor;
  float mTemperature;
  float mHumidity;
};

/** Abstract class for a digital SHT Sensor driver */
class SHTSensorDriver
{
public:
  virtual ~SHTSensorDriver() = 0;

  /**
   * Set the sensor accuracy.
   * Returns false if the sensor does not support changing the accuracy
   */
  virtual bool setAccuracy(SHTSensor::SHTAccuracy newAccuracy)
  {
    return false;
  }

  /**
   * Soft reset the sensor
   * Returns false if the sensor does not support soft reset
   */
  virtual bool softReset()
  {
    return false;
  }

  /** Returns true if the next sample was read and the values are cached */
  virtual bool readSample();

  /**
   * Get the relative humidity in percent read from the last sample
   * Use readSample() to trigger a new sensor reading
   */
  float getHumidity() const
  {
    return mHumidity;
  }

  /**
   * Get the humidity in percent read from the last sample
   * Use readSample() to trigger a new sensor reading
   */
  float getTemperature() const
  {
    return mTemperature;
  }

  float mTemperature;
  float mHumidity;
};

/** Base class for i2c SHT Sensor drivers */
class SHTI2cSensor : public SHTSensorDriver
{
public:
  /** Size of i2c commands to send */
  static const uint8_t CMD_SIZE;

  /** Size of i2c replies to expect */
  static const uint8_t EXPECTED_DATA_SIZE;

  /**
   * Constructor for i2c SHT Sensors
   * Takes the `i2cAddress' to read, the `i2cCommand' issues when sampling
   * the sensor and the values `a', `b', `c' to convert the fixed-point
   * temperature value received by the sensor to a floating point value using
   * the formula: temperature = a + b * (rawTemperature / c)
   * and the values `x' and `y' to convert the fixed-point humidity value
   * received by the sensor to a floating point value using the formula:
   * humidity = x * (rawHumidity / y)
   * duration is the duration in milliseconds of one measurement
   */
  SHTI2cSensor(uint8_t i2cAddress, uint16_t i2cCommand, uint8_t duration,
               float a, float b, float c,
               float x, float y)
      : mI2cAddress(i2cAddress), mI2cCommand(i2cCommand), mDuration(duration),
        mA(a), mB(b), mC(c), mX(x), mY(y)
  {
  }

  virtual ~SHTI2cSensor()
  {
  }

  virtual bool readSample();
  // virtual bool softReset();

  uint8_t mI2cAddress;
  uint16_t mI2cCommand;
  uint8_t mDuration;
  float mA;
  float mB;
  float mC;
  float mX;
  float mY;

private:
  static uint8_t crc8(const uint8_t *data, uint8_t len);
  static bool readFromI2c(uint8_t i2cAddress,
                          const uint8_t *i2cCommand,
                          uint8_t commandLength, uint8_t *data,
                          uint8_t dataLength, uint8_t duration);
};

class SHT3xAnalogSensor
{
public:
  /**
   * Instantiate a new Sensirion SHT3x Analog sensor driver instance.
   * The required paramters are `humidityPin` and `temperaturePin`
   * An optional `readResolutionBits' can be set since the Arduino/Genuino Zero
   * support 12bit precision analog readings. By default, 10 bit precision is
   * used.
   *
   * Example usage:
   * SHT3xAnalogSensor sht3xAnalog(HUMIDITY_PIN, TEMPERATURE_PIN);
   * float humidity = sht.readHumidity();
   * float temperature = sht.readTemperature();
   */
  SHT3xAnalogSensor(uint8_t humidityPin, uint8_t temperaturePin,
                    uint8_t readResolutionBits = 10)
      : mHumidityAdcPin(humidityPin), mTemperatureAdcPin(temperaturePin),
        mReadResolutionBits(readResolutionBits)
  {
  }

  virtual ~SHT3xAnalogSensor()
  {
  }

  float readHumidity();
  float readTemperature();

  uint8_t mHumidityAdcPin;
  uint8_t mTemperatureAdcPin;
  uint8_t mReadResolutionBits;
};
