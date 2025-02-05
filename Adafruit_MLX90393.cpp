/******************************************************************************
  This is a library for the MLX90393 magnetometer.

  Designed specifically to work with the MLX90393 breakout from Adafruit:

  ----> https://www.adafruit.com/products/4022

  These sensors use I2C to communicate, 2 pins are required to interface.

  Adafruit invests time and resources providing this open source code, please
  support Adafruit and open-source hardware by purchasing products from
  Adafruit!

  Written by Kevin Townsend/ktown for Adafruit Industries.

  MIT license, all text above must be included in any redistribution
 *****************************************************************************/
#include "Adafruit_MLX90393.h"

/**
 * Instantiates a new Adafruit_MLX90393 class instance
 */
Adafruit_MLX90393::Adafruit_MLX90393(void) {}

/*!
 *    @brief  Sets up the hardware and initializes I2C
 *    @param  i2c_addr
 *            The I2C address to be used.
 *    @param  wire
 *            The Wire object to be used for I2C connections.
 *    @return True if initialization was successful, otherwise false.
 */
bool Adafruit_MLX90393::begin_I2C(uint8_t i2c_addr, TwoWire *wire) {
  _i2c_address = i2c_addr;
  _i2c = wire;
  wire->begin();
  return _init();
}

bool Adafruit_MLX90393::_init(void) {
  if (!exitMode())
    return false;

  if (!reset())
    return false;

  /* Set gain and sensor config. */
  if (!setGain(MLX90393_GAIN_1X)) {
    return false;
  }

  /* Set resolution. */
  if (!setResolution(MLX90393_X, MLX90393_RES_16))
    return false;
  if (!setResolution(MLX90393_Y, MLX90393_RES_16))
    return false;
  if (!setResolution(MLX90393_Z, MLX90393_RES_16))
    return false;

  /* Set oversampling. */
  if (!setOversampling(MLX90393_OSR_3))
    return false;

  /* Set digital filtering. */
  if (!setFilter(MLX90393_FILTER_7))
    return false;

  /* set INT pin to output interrupt */
  if (!setTrigInt(false)) {
    return false;
  }

  return true;
}

/**
 * Perform a mode exit
 * @return True if the operation succeeded, otherwise false.
 */
bool Adafruit_MLX90393::exitMode(void) {
  uint8_t tx[1] = {MLX90393_REG_EX};

  /* Perform the transaction. */
  const uint8_t status = transceive(tx, sizeof(tx), NULL, 0, 0);
  return (status & (MLX90393_STATUS_BURSTMODE | MLX90393_STATUS_SMMODE |
                    MLX90393_STATUS_WOC | MLX90393_STATUS_ERROR)) == 0;
}

/**
 * Perform a soft reset
 * @return True if the operation succeeded, otherwise false.
 */
bool Adafruit_MLX90393::reset(void) {
  uint8_t tx[1] = {MLX90393_REG_RT};

  /* Perform the transaction. */
  if (transceive(tx, sizeof(tx), NULL, 0, 5) != MLX90393_STATUS_RESET) {
    return false;
  }
  return true;
}

/**
 * Sets the sensor gain to the specified level.
 * @param gain  The gain level to set.
 * @return True if the operation succeeded, otherwise false.
 */
bool Adafruit_MLX90393::setGain(mlx90393_gain_t gain) {
  _gain = gain;

  uint16_t data = 0;
  readRegister(MLX90393_CONF1, &data);

  // mask off gain bits
  data &= ~0x0070;
  // set gain bits
  data |= gain << MLX90393_GAIN_SHIFT;

  return writeRegister(MLX90393_CONF1, data);
}

/**
 * Gets the current sensor gain.
 *
 * @return An enum containing the current gain level.
 */
mlx90393_gain_t Adafruit_MLX90393::getGain(void) {
  uint16_t data = 0;
  readRegister(MLX90393_CONF1, &data);

  // mask off gain bits
  data &= 0x0070;

  return (mlx90393_gain_t)(data >> 4);
}

/**
 * Sets the sensor resolution to the specified level.
 * @param axis  The axis to set.
 * @param resolution  The resolution level to set.
 * @return True if the operation succeeded, otherwise false.
 */
bool Adafruit_MLX90393::setResolution(enum mlx90393_axis axis,
                                      enum mlx90393_resolution resolution) {
  uint16_t data = 0;
  readRegister(MLX90393_CONF3, &data);

  switch (axis) {
  case MLX90393_X:
    _res_x = resolution;
    data &= ~0x0060;
    data |= resolution << 5;
    break;
  case MLX90393_Y:
    _res_y = resolution;
    data &= ~0x0180;
    data |= resolution << 7;
    break;
  case MLX90393_Z:
    _res_z = resolution;
    data &= ~0x0600;
    data |= resolution << 9;
    break;
  default:
    assert(false);
    return false;  // Not implemented yet.
  }

  return writeRegister(MLX90393_CONF3, data);
}

/**
 * Gets the current sensor resolution.
 * @param axis  The axis to get.
 * @return An enum containing the current resolution.
 */
enum mlx90393_resolution
Adafruit_MLX90393::getResolution(enum mlx90393_axis axis) {
  switch (axis) {
  case MLX90393_X:
    return _res_x;
  case MLX90393_Y:
    return _res_y;
  case MLX90393_Z:
    return _res_z;
  case MLX90393_T:
    assert(false);
    return _res_x;
  }
  return _res_x;  // I guess?
}

/**
 * Sets the digital filter.
 * @param filter The digital filter setting.
 * @return True if the operation succeeded, otherwise false.
 */
bool Adafruit_MLX90393::setFilter(enum mlx90393_filter filter) {
  _dig_filt = filter;

  uint16_t data = 0;
  readRegister(MLX90393_CONF3, &data);

  data &= ~0x1C;
  data |= filter << 2;

  return writeRegister(MLX90393_CONF3, data);
}

/**
 * Gets the current digital filter setting.
 * @return An enum containing the current digital filter setting.
 */
enum mlx90393_filter Adafruit_MLX90393::getFilter(void) { return _dig_filt; }

/**
 * Sets the oversampling.
 * @param oversampling The oversampling value to use.
 * @return True if the operation succeeded, otherwise false.
 */
bool Adafruit_MLX90393::setOversampling(
    enum mlx90393_oversampling oversampling) {
  _osr = oversampling;

  uint16_t data = 0;
  readRegister(MLX90393_CONF3, &data);

  data &= ~0x03;
  data |= oversampling;

  return writeRegister(MLX90393_CONF3, data);
}

/**
 * Gets the current oversampling setting.
 * @return An enum containing the current oversampling setting.
 */
enum mlx90393_oversampling Adafruit_MLX90393::getOversampling(void) {
  return _osr;
}

/**
 * Sets the TRIG_INT pin to the specified function.
 *
 * @param state  'true/1' sets the pin to INT, 'false/0' to TRIG.
 *
 * @return True if the operation succeeded, otherwise false.
 */
bool Adafruit_MLX90393::setTrigInt(bool state) {
  uint16_t data = 0;
  readRegister(MLX90393_CONF2, &data);

  // mask off trigint bit
  data &= ~0x8000;

  // set trigint bit if desired
  if (state) {
    /* Set the INT, highest bit */
    data |= 0x8000;
  }

  return writeRegister(MLX90393_CONF2, data);
}

bool Adafruit_MLX90393::startBurstMode(uint8_t axes) {
  uint8_t tx[1] = {
      static_cast<uint8_t>(MLX90393_REG_SB | axes),
  };
  // Note that transceive "helpfully" shifts the status right by two bits.
  // To allow looking at the status directly, we need to shift it back.
  uint8_t status = transceive(tx, sizeof(tx), NULL, 0, 0);
  if (!(status & MLX90393_STATUS_BURSTMODE)) {
    return false;
  }
  if (status & 0b0001000) {
    return false;
  }
  return true;
}

bool Adafruit_MLX90393::setBurstRate(int delay_ms) {
  const int delay_20ms = std::clamp(delay_ms / 20, 0, 0b111111);

  uint16_t data = 0;
  if (!readRegister(MLX90393_CONF2, &data)) {
    return false;
  }

  data &= ~0b111111;
  data |= delay_20ms;

  return writeRegister(MLX90393_CONF2, data);
}

/**
 * Begin a single measurement on all axes
 *
 * @return True on command success
 */
bool Adafruit_MLX90393::startSingleMeasurement(void) {
  uint8_t tx[1] = {MLX90393_REG_SM | MLX90393_AXIS_ALL};

  /* Set the device to single measurement mode */
  uint8_t stat = transceive(tx, sizeof(tx), NULL, 0, 0);
  if (!(stat & MLX90393_STATUS_ERROR) || (stat == MLX90393_STATUS_SMMODE)) {
    return true;
  }
  return false;
}

/**
 * Reads data from data register & returns the results.
 *
 * @param x     Pointer to where the 'x' value should be stored.
 * @param y     Pointer to where the 'y' value should be stored.
 * @param z     Pointer to where the 'z' value should be stored.
 *
 * @return True on command success
 */
bool Adafruit_MLX90393::readMeasurement(float *x, float *y, float *z) {
  uint8_t tx[1] = {MLX90393_REG_RM | MLX90393_AXIS_ALL};
  uint8_t rx[6] = {0};

  /* Read a single data sample. */
  if (transceive(tx, sizeof(tx), rx, sizeof(rx), 0) & MLX90393_STATUS_ERROR) {
    return false;
  }

  int16_t xi, yi, zi;

  /* Convert data to uT and float. */
  xi = (rx[0] << 8) | rx[1];
  yi = (rx[2] << 8) | rx[3];
  zi = (rx[4] << 8) | rx[5];

  if (_res_x == MLX90393_RES_18)
    xi -= 0x8000;
  if (_res_x == MLX90393_RES_19)
    xi -= 0x4000;
  if (_res_y == MLX90393_RES_18)
    yi -= 0x8000;
  if (_res_y == MLX90393_RES_19)
    yi -= 0x4000;
  if (_res_z == MLX90393_RES_18)
    zi -= 0x8000;
  if (_res_z == MLX90393_RES_19)
    zi -= 0x4000;

  *x = (float)xi * mlx90393_lsb_lookup[0][_gain][_res_x][0];
  *y = (float)yi * mlx90393_lsb_lookup[0][_gain][_res_y][0];
  *z = (float)zi * mlx90393_lsb_lookup[0][_gain][_res_z][1];

  return true;
}

bool Adafruit_MLX90393::readMeasurement(uint8_t axes, std::span<float> result) {
  if (axes & MLX90393_T) {
    return false;
  }
  const int naxes = std::popcount(axes);
  if (naxes > result.size()) {
    return false;
  }

  std::array<uint8_t, 1> tx = {static_cast<uint8_t>(MLX90393_REG_RM | axes)};
  std::array<uint8_t, 8> rx;
  const int rxlen = 2 * naxes;
  if (transceive(tx.data(), 1, rx.data(), rxlen, 0) & MLX90393_STATUS_ERROR) {
    return false;
  }

  int result_index = 0;
  std::span<const uint8_t> rx_span(rx);
  for (auto axis : {MLX90393_X, MLX90393_Y, MLX90393_Z}) {
    if ((axes & axis) == 0) {
      continue;
    }
    const int16_t raw = (rx_span[0] << 8) | rx_span[1];
    result[result_index++] = measurementToFloat(axis, raw);
    rx_span = rx_span.subspan(2);
  }
  return true;
}

mlx90393_resolution Adafruit_MLX90393::resFromAxis(mlx90393_axis_t axis) const {
  switch (axis) {
    case MLX90393_X:
      return _res_x;
    case MLX90393_Y:
      return _res_y;
    case MLX90393_Z:
      return _res_z;
    default:
      assert(false);
      return _res_x;
  }
}

float Adafruit_MLX90393::measurementToFloat(mlx90393_axis_t axis,
                                            int16_t raw) const {
  const mlx90393_resolution res = resFromAxis(axis);
  if (res == MLX90393_RES_18) {
    raw -= 0x8000;
  } else if (res == MLX90393_RES_19) {
    raw -= 0x4000;
  }
  const bool is_z = axis == MLX90393_Z;
  return (float)raw * mlx90393_lsb_lookup[0][_gain][res][is_z];
}

/**
 * Performs a single X/Y/Z conversion and returns the results.
 *
 * @param x     Pointer to where the 'x' value should be stored.
 * @param y     Pointer to where the 'y' value should be stored.
 * @param z     Pointer to where the 'z' value should be stored.
 *
 * @return True if the operation succeeded, otherwise false.
 */
bool Adafruit_MLX90393::readData(float *x, float *y, float *z) {
  if (!startSingleMeasurement())
    return false;
  // See MLX90393 Getting Started Guide for fancy formula
  // tconv = f(OSR, DIG_FILT, OSR2, ZYXT)
  // For now, using Table 18 from datasheet
  // Without +10ms delay measurement doesn't always seem to work
  delay(mlx90393_tconv[_dig_filt][_osr] + 10);
  return readMeasurement(x, y, z);
}

bool Adafruit_MLX90393::readData(uint8_t axes, std::span<float> result) {
  if (!startSingleMeasurement()) {
    return false;
  }
  delay(mlx90393_tconv[_dig_filt][_osr] + 10);
  return readMeasurement(axes, result);
}

bool Adafruit_MLX90393::writeRegister(uint8_t reg, uint16_t data) {
  uint8_t tx[4] = {
      MLX90393_REG_WR,
      (uint8_t)(data >> 8),   // high byte
      (uint8_t)(data & 0xFF), // low byte
      (uint8_t)(reg << 2)};   // the register itself, shift up by 2 bits!

  /* Perform the transaction. */
  return !(transceive(tx, sizeof(tx), NULL, 0, 0) & MLX90393_STATUS_ERROR);
}

bool Adafruit_MLX90393::readRegister(uint8_t reg, uint16_t *data) {
  uint8_t tx[2] = {
      MLX90393_REG_RR,
      (uint8_t)(reg << 2)}; // the register itself, shift up by 2 bits!

  uint8_t rx[2];

  /* Perform the transaction. */
  if (transceive(tx, sizeof(tx), rx, sizeof(rx), 0) & MLX90393_STATUS_ERROR) {
    return false;
  }

  *data = ((uint16_t)rx[0] << 8) | rx[1];

  return true;
}

/**
 * Performs a full read/write transaction with the sensor.
 *
 * @param txbuf     Pointer the the buffer containing the data to write.
 * @param txlen     The number of bytes to write.
 * @param rxbuf     Pointer to an appropriately large buffer where data read
 *                  back will be written.
 * @param rxlen     The number of bytes to read back (not including the
 *                  mandatory status byte that is always returned).
 *
 * @return The status byte from the IC.
 */
uint8_t Adafruit_MLX90393::transceive(uint8_t *txbuf, uint8_t txlen,
                                      uint8_t *rxbuf, uint8_t rxlen,
                                      uint8_t interdelay) {
  uint8_t status = 0;
  uint8_t i;
  uint8_t rxbuf2[rxlen + 2];

  /* Write stage */

  _i2c->beginTransmission(_i2c_address);
  _i2c->write(txbuf, txlen);
  if (_i2c->endTransmission() != 0) {
    return MLX90393_STATUS_ERROR;
  }
  delay(interdelay);

  /* Read status byte plus any others */
  uint8_t rxlen1 = rxlen + 1;
  if (_i2c->requestFrom(_i2c_address, rxlen1) != rxlen1 ||
      _i2c->readBytes(rxbuf2, rxlen1) != rxlen1) {
    return MLX90393_STATUS_ERROR;
  }

  status = rxbuf2[0];
  for (i = 0; i < rxlen; i++) {
    rxbuf[i] = rxbuf2[i + 1];
  }

  /* Mask out bytes available part of the status response. */
  return status & ~0b11;
}
