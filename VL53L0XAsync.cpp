// Most of the functionality of this library is based on the VL53L0X API
// provided by ST (STSW-IMG005), and some of the explanatory comments are quoted
// or paraphrased from the API source code, API user manual (UM2039), and the
// VL53L0X datasheet.

#include <Wire.h>
#include <math.h>

#include "Debug.hpp"
#include "Application.hpp"

#include "VL53L0XAsync.hpp"


// Defines /////////////////////////////////////////////////////////////////////

// The Arduino two-wire interface uses a 7-bit number for the address,
// and sets the last bit correctly based on reads and writes
#define ADDRESS_DEFAULT 0b0101001

// Decode VCSEL (vertical cavity surface emitting laser) pulse period in PCLKs
// from register value
// based on VL53L0X_decode_vcsel_period()
#define decodeVcselPeriod(reg_val)      (((reg_val) + 1) << 1)

// Encode VCSEL pulse period register value from period in PCLKs
// based on VL53L0X_encode_vcsel_period()
#define encodeVcselPeriod(period_pclks) (((period_pclks) >> 1) - 1)

// Calculate macro period in *nanoseconds* from VCSEL period in PCLKs
// based on VL53L0X_calc_macro_period_ps()
// PLL_period_ps = 1655; macro_period_vclks = 2304
#define calcMacroPeriod(vcsel_period_pclks) ((((uint32_t)2304 * (vcsel_period_pclks) * 1655) + 500) / 1000)


#define startTimer(slot)                                  \
    debugAssert(!mTimer.running());                       \
    EventObjectConnect(&mTimer, expired, this, slot);     \
    mTimer.setTimeout(10);                                \
    mTimer.start();                                       \
                                                          \
    mExpires = 0;                                         


#define stopTimer(slot)                                   \
    EventObjectDisconnect(&mTimer, expired, this, slot);  \
    mTimer.stop();                                        \


const unsigned char VL53L0XAsync::DefaultAddress = 0b0101001;


bool VL53L0XAsync::sDrivingXshut = false;


bool VL53L0XAsync::tickTimer()
{
    return io_timeout > 0 && ++mExpires >= io_timeout / mTimer.timeout();
}


void VL53L0XAsync::shutdown()
{
    did_timeout = true;

    if (mXshutPin != 0) {
        pinMode(mXshutPin, OUTPUT);
        digitalWrite(mXshutPin, LOW);
    }
}


void VL53L0XAsync::start()
{
    debugAssert((initFinished()->emitting() || 
                initFinished()->lastEmitted() != -1) &&
            !did_timeout);

    startContinuous();
    debugAssert(!mTimer.running());
    EventObjectConnect(&mTimer, expired, this, onRangeReadyTimerExpired);
    mTimer.setTimeout(measurement_timing_budget_us / 1000 + 1);
    mTimer.start();
}


float VL53L0XAsync::range() const
{
    return mRange > maximum() ? INFINITY : mRange;
}


float VL53L0XAsync::delta() const
{
    return 20;
}


float VL53L0XAsync::maximum() const
{
    return 1500;
}


void VL53L0XAsync::reinit()
{
    did_timeout = false;
    startTimer(onStarted);
}


void VL53L0XAsync::onRangeReadyTimerExpired()
{
    if ((readReg(RESULT_INTERRUPT_STATUS) & 0x07) == 0 ||
            readReg(I2C_SLAVE_DEVICE_ADDRESS) != address) {

        EventObjectDisconnect(&mTimer, expired, this,
                onRangeReadyTimerExpired);
        mTimer.stop();
        shutdown();

        rangeError()->post();

        return;
    }

    mRange = readReg16Bit(RESULT_RANGE_STATUS + 10);

    writeReg(SYSTEM_INTERRUPT_CLEAR, 0x01);

    rangeReady()->post();
}


void VL53L0XAsync::onSpadInfoTimerExpired()
{
    if (readReg(0x83) == 0) {
        if (tickTimer()) {
            stopTimer(onSpadInfoTimerExpired);
            shutdown();
            initFailed()->post();
        }

        return;
    }

    stopTimer(onSpadInfoTimerExpired);

  uint8_t tmp;

  writeReg(0x83, 0x01);
  tmp = readReg(0x92);

  uint8_t spad_count = tmp & 0x7f;
  bool spad_type_is_aperture = (tmp >> 7) & 0x01;

  writeReg(0x81, 0x00);
  writeReg(0xFF, 0x06);
  writeReg(0x83, readReg(0x83)  & ~0x04);
  writeReg(0xFF, 0x01);
  writeReg(0x00, 0x01);

  writeReg(0xFF, 0x00);
  writeReg(0x80, 0x00);

  // VL53L0X_StaticInit() begin

  // The SPAD map (RefGoodSpadMap) is read by VL53L0X_get_info_from_device() in
  // the API, but the same data seems to be more easily readable from
  // GLOBAL_CONFIG_SPAD_ENABLES_REF_0 through _6, so read it from there
  uint8_t ref_spad_map[6];
  readMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

  // -- VL53L0X_set_reference_spads() begin (assume NVM values are valid)

  writeReg(0xFF, 0x01);
  writeReg(DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
  writeReg(DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
  writeReg(0xFF, 0x00);
  writeReg(GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);

  uint8_t first_spad_to_enable = spad_type_is_aperture ? 12 : 0; // 12 is the first aperture spad
  uint8_t spads_enabled = 0;

  for (uint8_t i = 0; i < 48; i++)
  {
    if (i < first_spad_to_enable || spads_enabled == spad_count)
    {
      // This bit is lower than the first one that should be enabled, or
      // (reference_spad_count) bits have already been enabled, so zero this bit
      ref_spad_map[i / 8] &= ~(1 << (i % 8));
    }
    else if ((ref_spad_map[i / 8] >> (i % 8)) & 0x1)
    {
      spads_enabled++;
    }
  }

  writeMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

  // -- VL53L0X_set_reference_spads() end

  // -- VL53L0X_load_tuning_settings() begin
  // DefaultTuningSettings from vl53l0x_tuning.h

  writeReg(0xFF, 0x01);
  writeReg(0x00, 0x00);

  writeReg(0xFF, 0x00);
  writeReg(0x09, 0x00);
  writeReg(0x10, 0x00);
  writeReg(0x11, 0x00);

  writeReg(0x24, 0x01);
  writeReg(0x25, 0xFF);
  writeReg(0x75, 0x00);

  writeReg(0xFF, 0x01);
  writeReg(0x4E, 0x2C);
  writeReg(0x48, 0x00);
  writeReg(0x30, 0x20);

  writeReg(0xFF, 0x00);
  writeReg(0x30, 0x09);
  writeReg(0x54, 0x00);
  writeReg(0x31, 0x04);
  writeReg(0x32, 0x03);
  writeReg(0x40, 0x83);
  writeReg(0x46, 0x25);
  writeReg(0x60, 0x00);
  writeReg(0x27, 0x00);
  writeReg(0x50, 0x06);
  writeReg(0x51, 0x00);
  writeReg(0x52, 0x96);
  writeReg(0x56, 0x08);


  writeReg(0x57, 0x30);

  writeReg(0x61, 0x00);
  writeReg(0x62, 0x00);
  writeReg(0x64, 0x00);
  writeReg(0x65, 0x00);
  writeReg(0x66, 0xA0);

  writeReg(0xFF, 0x01);
  writeReg(0x22, 0x32);
  writeReg(0x47, 0x14);
  writeReg(0x49, 0xFF);
  writeReg(0x4A, 0x00);

  writeReg(0xFF, 0x00);
  writeReg(0x7A, 0x0A);
  writeReg(0x7B, 0x00);
  writeReg(0x78, 0x21);

  writeReg(0xFF, 0x01);
  writeReg(0x23, 0x34);
  writeReg(0x42, 0x00);
  writeReg(0x44, 0xFF);
  writeReg(0x45, 0x26);
  writeReg(0x46, 0x05);
  writeReg(0x40, 0x40);
  writeReg(0x0E, 0x06);
  writeReg(0x20, 0x1A);
  writeReg(0x43, 0x40);

  writeReg(0xFF, 0x00);
  writeReg(0x34, 0x03);
  writeReg(0x35, 0x44);

  writeReg(0xFF, 0x01);
  writeReg(0x31, 0x04);
  writeReg(0x4B, 0x09);
  writeReg(0x4C, 0x05);
  writeReg(0x4D, 0x04);

  writeReg(0xFF, 0x00);
  writeReg(0x44, 0x00);
  writeReg(0x45, 0x20);
  writeReg(0x47, 0x08);
  writeReg(0x48, 0x28);
  writeReg(0x67, 0x00);
  writeReg(0x70, 0x04);
  writeReg(0x71, 0x01);
  writeReg(0x72, 0xFE);
  writeReg(0x76, 0x00);
  writeReg(0x77, 0x00);

  writeReg(0xFF, 0x01);
  writeReg(0x0D, 0x01);

  writeReg(0xFF, 0x00);
  writeReg(0x80, 0x01);
  writeReg(0x01, 0xF8);

  writeReg(0xFF, 0x01);
  writeReg(0x8E, 0x01);
  writeReg(0x00, 0x01);
  writeReg(0xFF, 0x00);
  writeReg(0x80, 0x00);

  // -- VL53L0X_load_tuning_settings() end

  // "Set interrupt config to new sample ready"
  // -- VL53L0X_SetGpioConfig() begin

  writeReg(SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
  writeReg(GPIO_HV_MUX_ACTIVE_HIGH, readReg(GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10); // active low
  writeReg(SYSTEM_INTERRUPT_CLEAR, 0x01);

  // -- VL53L0X_SetGpioConfig() end

  measurement_timing_budget_us = getMeasurementTimingBudget();

  // "Disable MSRC and TCC by default"
  // MSRC = Minimum Signal Rate Check
  // TCC = Target CentreCheck
  // -- VL53L0X_SetSequenceStepEnable() begin

  writeReg(SYSTEM_SEQUENCE_CONFIG, 0xE8);

  // -- VL53L0X_SetSequenceStepEnable() end

  // "Recalculate timing budget"
  setMeasurementTimingBudget(measurement_timing_budget_us);

  // VL53L0X_StaticInit() end

  // VL53L0X_PerformRefCalibration() begin (VL53L0X_perform_ref_calibration())

  // -- VL53L0X_perform_vhv_calibration() begin

    singleRefCalibration()->disconnect(this, nullptr);
    EventObjectOnce(this, singleRefCalibration, this, onVhvCalibration);

    writeReg(SYSTEM_SEQUENCE_CONFIG, 0x01);
    performSingleRefCalibration(0x40);

  // -- VL53L0X_perform_vhv_calibration() end
}


void VL53L0XAsync::onVhvCalibration()
{
    SequenceStepEnables enables;
    SequenceStepTimeouts timeouts;

    getSequenceStepEnables(&enables);
    getSequenceStepTimeouts(&enables, &timeouts);

    /* Set VcselPeriodPreRange */

    switch (mVcselPeriodPreRange) {


    case 12:

        writeReg(PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x18);

        break;


    case 14:

        writeReg(PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x30);

        break;


    case 16:

        writeReg(PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x40);

        break;


    case 18:

        writeReg(PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x50);

        break;


    }

    writeReg(PRE_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);
    writeReg(PRE_RANGE_CONFIG_VCSEL_PERIOD,
            encodeVcselPeriod(mVcselPeriodPreRange));

    // update timeouts

    // set_sequence_step_timeout() begin
    // (SequenceStepId == VL53L0X_SEQUENCESTEP_PRE_RANGE)

    uint16_t new_pre_range_timeout_mclks =
      timeoutMicrosecondsToMclks(timeouts.pre_range_us, mVcselPeriodPreRange);

    writeReg16Bit(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI,
      encodeTimeout(
          timeoutMicrosecondsToMclks(timeouts.pre_range_us,
              mVcselPeriodPreRange)));

    // set_sequence_step_timeout() end

    // set_sequence_step_timeout() begin
    // (SequenceStepId == VL53L0X_SEQUENCESTEP_MSRC)

    uint16_t new_msrc_timeout_mclks =
      timeoutMicrosecondsToMclks(timeouts.msrc_dss_tcc_us,
              mVcselPeriodPreRange);

    writeReg(MSRC_CONFIG_TIMEOUT_MACROP,
      (new_msrc_timeout_mclks > 256) ? 255 : (new_msrc_timeout_mclks - 1));

    // set_sequence_step_timeout() end

    /* Set VcselPeriodFinalRange */

    switch (mVcselPeriodFinalRange) {


    case 8:

        writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x10);
        writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
        writeReg(GLOBAL_CONFIG_VCSEL_WIDTH, 0x02);
        writeReg(ALGO_PHASECAL_CONFIG_TIMEOUT, 0x0C);
        writeReg(0xFF, 0x01);
        writeReg(ALGO_PHASECAL_LIM, 0x30);
        writeReg(0xFF, 0x00);

        break;


    case 10:

        writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x28);
        writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
        writeReg(GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
        writeReg(ALGO_PHASECAL_CONFIG_TIMEOUT, 0x09);
        writeReg(0xFF, 0x01);
        writeReg(ALGO_PHASECAL_LIM, 0x20);
        writeReg(0xFF, 0x00);

        break;


    case 12:

        writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x38);
        writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
        writeReg(GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
        writeReg(ALGO_PHASECAL_CONFIG_TIMEOUT, 0x08);
        writeReg(0xFF, 0x01);
        writeReg(ALGO_PHASECAL_LIM, 0x20);
        writeReg(0xFF, 0x00);

        break;


    case 14:

        writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x48);
        writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
        writeReg(GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
        writeReg(ALGO_PHASECAL_CONFIG_TIMEOUT, 0x07);
        writeReg(0xFF, 0x01);
        writeReg(ALGO_PHASECAL_LIM, 0x20);
        writeReg(0xFF, 0x00);

        break;


    }

    // apply new VCSEL period
    writeReg(FINAL_RANGE_CONFIG_VCSEL_PERIOD,
            encodeVcselPeriod(mVcselPeriodFinalRange));

    // update timeouts

    // set_sequence_step_timeout() begin
    // (SequenceStepId == VL53L0X_SEQUENCESTEP_FINAL_RANGE)

    // "For the final range timeout, the pre-range timeout
    //  must be added. To do this both final and pre-range
    //  timeouts must be expressed in macro periods MClks
    //  because they have different vcsel periods."

    uint16_t new_final_range_timeout_mclks =
      timeoutMicrosecondsToMclks(timeouts.final_range_us,
              mVcselPeriodFinalRange);

    if (enables.pre_range)
    {
      new_final_range_timeout_mclks += timeouts.pre_range_mclks;
    }

    writeReg16Bit(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI,
      encodeTimeout(new_final_range_timeout_mclks));

    // set_sequence_step_timeout end

    setMeasurementTimingBudget(measurement_timing_budget_us);


  // -- VL53L0X_perform_phase_calibration() begin

    singleRefCalibration()->disconnect(this, nullptr);
    EventObjectOnce(this, singleRefCalibration, this, onPhaseCalibration);
    writeReg(SYSTEM_SEQUENCE_CONFIG, 0x02);
    performSingleRefCalibration(0x00);

  // -- VL53L0X_perform_phase_calibration() end
}


void VL53L0XAsync::onPhaseCalibration()
{
  // "restore the previous Sequence Config"
  writeReg(SYSTEM_SEQUENCE_CONFIG, 0xE8);

  // VL53L0X_PerformRefCalibration() end

    initFinished()->post();

}


// Constructors ////////////////////////////////////////////////////////////////

VL53L0XAsync::VL53L0XAsync(unsigned char xshutPin, unsigned char address)
  : RangeSensor(),
    address(address)
  , io_timeout(100) // no timeout
  , did_timeout(false),
    mTimer(10),
    mRange(INFINITY),
    mXshutPin(xshutPin),
    mVcselPeriodPreRange(14),
    mVcselPeriodFinalRange(10)
{
    EventObjectConnect(Application::instance(), started, this, onStarted);

    if (xshutPin != 0) {
        pinMode(xshutPin, OUTPUT);
        digitalWrite(xshutPin, LOW);
    }
}

// Public Methods //////////////////////////////////////////////////////////////


// Initialize sensor using sequence based on VL53L0X_DataInit(),
// VL53L0X_StaticInit(), and VL53L0X_PerformRefCalibration().
// This function does not perform reference SPAD calibration
// (VL53L0X_PerformRefSpadManagement()), since the API user manual says that it
// is performed by ST on the bare modules; it seems like that should work well
// enough unless a cover glass is added.
// If io_2v8 (optional) is true or not given, the sensor is configured for 2V8
// mode.
void VL53L0XAsync::onStarted()
{
    stopTimer(onStarted);

    if (mXshutPin != 0) {
        if (sDrivingXshut) {
            startTimer(onStarted);

            return;
        }

        sDrivingXshut = true;

        pinMode(mXshutPin, INPUT);
        startTimer(onStartedTimerExpired);
    } else {
        onStartedTimerExpired();
    }
}


void VL53L0XAsync::onStartedTimerExpired()
{
    if (mTimer.running()) {
        sDrivingXshut = false;
    }

    stopTimer(onStartedTimerExpired);

    if (address != DefaultAddress) {
        unsigned char tmpAddress = address;

        address = DefaultAddress;

        writeReg(I2C_SLAVE_DEVICE_ADDRESS, tmpAddress & 0x7F);
        address = tmpAddress;
    }
    
  // VL53L0X_DataInit() begin

  // sensor uses 1V8 mode for I/O by default; switch to 2V8 mode if necessary
    const bool io_2v8 = true;

  if (io_2v8)
  {
    writeReg(VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV,
      readReg(VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV) | 0x01); // set bit 0
  }

  // "Set I2C standard mode"
  writeReg(0x88, 0x00);

  writeReg(0x80, 0x01);
  writeReg(0xFF, 0x01);
  writeReg(0x00, 0x00);
  stop_variable = readReg(0x91);
  writeReg(0x00, 0x01);
  writeReg(0xFF, 0x00);
  writeReg(0x80, 0x00);

  // disable SIGNAL_RATE_MSRC (bit 1) and SIGNAL_RATE_PRE_RANGE (bit 4) limit checks
  writeReg(MSRC_CONFIG_CONTROL, readReg(MSRC_CONFIG_CONTROL) | 0x12);

  // set final range signal rate limit to 0.25 MCPS (million counts per second)
  setSignalRateLimit(0.25);

  writeReg(SYSTEM_SEQUENCE_CONFIG, 0xFF);

  // VL53L0X_DataInit() end

    getSpadInfo();
}

// Write an 8-bit register
void VL53L0XAsync::writeReg(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
  //last_status = Wire.endTransmission();
}

// Write a 16-bit register
void VL53L0XAsync::writeReg16Bit(uint8_t reg, uint16_t value)
{
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write((value >> 8) & 0xFF); // value high byte
  Wire.write( value       & 0xFF); // value low byte
  Wire.endTransmission();
  //last_status = Wire.endTransmission();
}

// Write a 32-bit register
void VL53L0XAsync::writeReg32Bit(uint8_t reg, uint32_t value)
{
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write((unsigned char) (value >> 24) & 0xFF); // value highest byte
  Wire.write((unsigned char) (value >> 16) & 0xFF);
  Wire.write((unsigned char) (value >>  8) & 0xFF);
  Wire.write((unsigned char)  value        & 0xFF); // value lowest byte
  Wire.endTransmission();
  //last_status = Wire.endTransmission();
}

// Read an 8-bit register
uint8_t VL53L0XAsync::readReg(uint8_t reg)
{
  uint8_t value;

  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();
  //last_status = Wire.endTransmission();

  Wire.requestFrom(address, (uint8_t)1);
  value = Wire.read();

  return value;
}

// Read a 16-bit register
uint16_t VL53L0XAsync::readReg16Bit(uint8_t reg)
{
  uint16_t value;

  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();
  //last_status = Wire.endTransmission();

  Wire.requestFrom(address, (uint8_t)2);
  value  = (uint16_t)Wire.read() << 8; // value high byte
  value |=           Wire.read();      // value low byte

  return value;
}

// Read a 32-bit register
uint32_t VL53L0XAsync::readReg32Bit(uint8_t reg)
{
  uint32_t value;

  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();
  //last_status = Wire.endTransmission();

  Wire.requestFrom(address, (uint8_t)4);
  value  = (uint32_t)Wire.read() << 24; // value highest byte
  value |= (uint32_t)Wire.read() << 16;
  value |= (uint16_t)Wire.read() <<  8;
  value |=           Wire.read();       // value lowest byte

  return value;
}

// Write an arbitrary number of bytes from the given array to the sensor,
// starting at the given register
void VL53L0XAsync::writeMulti(uint8_t reg, uint8_t const * src, uint8_t count)
{
  Wire.beginTransmission(address);
  Wire.write(reg);

  while (count-- > 0)
  {
    Wire.write(*(src++));
  }

  Wire.endTransmission();
  //last_status = Wire.endTransmission();
}

// Read an arbitrary number of bytes from the sensor, starting at the given
// register, into the given array
void VL53L0XAsync::readMulti(uint8_t reg, uint8_t * dst, uint8_t count)
{
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();
  //last_status = Wire.endTransmission();

  Wire.requestFrom(address, count);

  while (count-- > 0)
  {
    *(dst++) = Wire.read();
  }
}

// Set the return signal rate limit check value in units of MCPS (mega counts
// per second). "This represents the amplitude of the signal reflected from the
// target and detected by the device"; setting this limit presumably determines
// the minimum measurement necessary for the sensor to report a valid reading.
// Setting a lower limit increases the potential range of the sensor but also
// seems to increase the likelihood of getting an inaccurate reading because of
// unwanted reflections from objects other than the intended target.
// Defaults to 0.25 MCPS as initialized by the ST API and this library.
void VL53L0XAsync::setSignalRateLimit(float limit_Mcps)
{
  debugAssert(limit_Mcps >= 0 && limit_Mcps <= 511.99);

  // Q9.7 fixed point format (9 integer bits, 7 fractional bits)
  writeReg16Bit(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, limit_Mcps * (1 << 7));
}

// Get the return signal rate limit check value in MCPS
float VL53L0XAsync::getSignalRateLimit(void)
{
  return (float)readReg16Bit(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT) / (1 << 7);
}

// Set the measurement timing budget in microseconds, which is the time allowed
// for one measurement; the ST API and this library take care of splitting the
// timing budget among the sub-steps in the ranging sequence. A longer timing
// budget allows for more accurate measurements. Increasing the budget by a
// factor of N decreases the range measurement standard deviation by a factor of
// sqrt(N). Defaults to about 33 milliseconds; the minimum is 20 ms.
// based on VL53L0X_set_measurement_timing_budget_micro_seconds()
bool VL53L0XAsync::setMeasurementTimingBudget(uint32_t budget_us)
{
  SequenceStepEnables enables;
  SequenceStepTimeouts timeouts;

  uint16_t const StartOverhead      = 1320; // note that this is different than the value in get_
  uint16_t const EndOverhead        = 960;
  uint16_t const MsrcOverhead       = 660;
  uint16_t const TccOverhead        = 590;
  uint16_t const DssOverhead        = 690;
  uint16_t const PreRangeOverhead   = 660;
  uint16_t const FinalRangeOverhead = 550;

  uint32_t const MinTimingBudget = 20000;


    if ((!initFinished()->emitting() && initFinished()->lastEmitted() == -1) ||
            did_timeout) {

        return false;
    }

  if (budget_us < MinTimingBudget) { return false; }

  uint32_t used_budget_us = StartOverhead + EndOverhead;

  getSequenceStepEnables(&enables);
  getSequenceStepTimeouts(&enables, &timeouts);

  if (enables.tcc)
  {
    used_budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
  }

  if (enables.dss)
  {
    used_budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
  }
  else if (enables.msrc)
  {
    used_budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
  }

  if (enables.pre_range)
  {
    used_budget_us += (timeouts.pre_range_us + PreRangeOverhead);
  }

  if (enables.final_range)
  {
    used_budget_us += FinalRangeOverhead;

    // "Note that the final range timeout is determined by the timing
    // budget and the sum of all other timeouts within the sequence.
    // If there is no room for the final range timeout, then an error
    // will be set. Otherwise the remaining time will be applied to
    // the final range."

    if (used_budget_us > budget_us)
    {
      // "Requested timeout too big."
      return false;
    }

    uint32_t final_range_timeout_us = budget_us - used_budget_us;

    // set_sequence_step_timeout() begin
    // (SequenceStepId == VL53L0X_SEQUENCESTEP_FINAL_RANGE)

    // "For the final range timeout, the pre-range timeout
    //  must be added. To do this both final and pre-range
    //  timeouts must be expressed in macro periods MClks
    //  because they have different vcsel periods."

    uint16_t final_range_timeout_mclks =
      timeoutMicrosecondsToMclks(final_range_timeout_us,
                                 timeouts.final_range_vcsel_period_pclks);

    if (enables.pre_range)
    {
      final_range_timeout_mclks += timeouts.pre_range_mclks;
    }

    writeReg16Bit(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI,
      encodeTimeout(final_range_timeout_mclks));

    // set_sequence_step_timeout() end

    measurement_timing_budget_us = budget_us; // store for internal reuse
  }
  return true;
}

// Get the measurement timing budget in microseconds
// based on VL53L0X_get_measurement_timing_budget_micro_seconds()
// in us
uint32_t VL53L0XAsync::getMeasurementTimingBudget(void)
{
  SequenceStepEnables enables;
  SequenceStepTimeouts timeouts;

  uint16_t const StartOverhead     = 1910; // note that this is different than the value in set_
  uint16_t const EndOverhead        = 960;
  uint16_t const MsrcOverhead       = 660;
  uint16_t const TccOverhead        = 590;
  uint16_t const DssOverhead        = 690;
  uint16_t const PreRangeOverhead   = 660;
  uint16_t const FinalRangeOverhead = 550;

  // "Start and end overhead times always present"
  uint32_t budget_us = StartOverhead + EndOverhead;

  getSequenceStepEnables(&enables);
  getSequenceStepTimeouts(&enables, &timeouts);

  if (enables.tcc)
  {
    budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
  }

  if (enables.dss)
  {
    budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
  }
  else if (enables.msrc)
  {
    budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
  }

  if (enables.pre_range)
  {
    budget_us += (timeouts.pre_range_us + PreRangeOverhead);
  }

  if (enables.final_range)
  {
    budget_us += (timeouts.final_range_us + FinalRangeOverhead);
  }

  measurement_timing_budget_us = budget_us; // store for internal reuse
  return budget_us;
}

// Get the VCSEL pulse period in PCLKs for the given period type.
// based on VL53L0X_get_vcsel_pulse_period()
uint8_t VL53L0XAsync::getVcselPulsePeriod(vcselPeriodType type)
{
  if (type == VcselPeriodPreRange)
  {
    return decodeVcselPeriod(readReg(PRE_RANGE_CONFIG_VCSEL_PERIOD));
  }
  else if (type == VcselPeriodFinalRange)
  {
    return decodeVcselPeriod(readReg(FINAL_RANGE_CONFIG_VCSEL_PERIOD));
  }
  else { return 255; }
}

// Start continuous ranging measurements. If period_ms (optional) is 0 or not
// given, continuous back-to-back mode is used (the sensor takes measurements as
// often as possible); otherwise, continuous timed mode is used, with the given
// inter-measurement period in milliseconds determining how often the sensor
// takes a measurement.
// based on VL53L0X_StartMeasurement()
void VL53L0XAsync::startContinuous(uint32_t period_ms)
{
  writeReg(0x80, 0x01);
  writeReg(0xFF, 0x01);
  writeReg(0x00, 0x00);
  writeReg(0x91, stop_variable);
  writeReg(0x00, 0x01);
  writeReg(0xFF, 0x00);
  writeReg(0x80, 0x00);

  if (period_ms != 0)
  {
    // continuous timed mode

    // VL53L0X_SetInterMeasurementPeriodMilliSeconds() begin

    uint16_t osc_calibrate_val = readReg16Bit(OSC_CALIBRATE_VAL);

    if (osc_calibrate_val != 0)
    {
      period_ms *= osc_calibrate_val;
    }

    writeReg32Bit(SYSTEM_INTERMEASUREMENT_PERIOD, period_ms);

    // VL53L0X_SetInterMeasurementPeriodMilliSeconds() end

    writeReg(SYSRANGE_START, 0x04); // VL53L0X_REG_SYSRANGE_MODE_TIMED
  }
  else
  {
    // continuous back-to-back mode
    writeReg(SYSRANGE_START, 0x02); // VL53L0X_REG_SYSRANGE_MODE_BACKTOBACK
  }
}

// Private Methods /////////////////////////////////////////////////////////////

// Get reference SPAD (single photon avalanche diode) count and type
// based on VL53L0X_get_info_from_device(),
// but only gets reference SPAD count and type
void VL53L0XAsync::getSpadInfo()
{
  writeReg(0x80, 0x01);
  writeReg(0xFF, 0x01);
  writeReg(0x00, 0x00);

  writeReg(0xFF, 0x06);
  writeReg(0x83, readReg(0x83) | 0x04);
  writeReg(0xFF, 0x07);
  writeReg(0x81, 0x01);

  writeReg(0x80, 0x01);

  writeReg(0x94, 0x6b);
  writeReg(0x83, 0x00);

  startTimer(onSpadInfoTimerExpired);
}

// Get sequence step enables
// based on VL53L0X_GetSequenceStepEnables()
void VL53L0XAsync::getSequenceStepEnables(SequenceStepEnables * enables)
{
  uint8_t sequence_config = readReg(SYSTEM_SEQUENCE_CONFIG);

  enables->tcc          = (sequence_config >> 4) & 0x1;
  enables->dss          = (sequence_config >> 3) & 0x1;
  enables->msrc         = (sequence_config >> 2) & 0x1;
  enables->pre_range    = (sequence_config >> 6) & 0x1;
  enables->final_range  = (sequence_config >> 7) & 0x1;
}

// Get sequence step timeouts
// based on get_sequence_step_timeout(),
// but gets all timeouts instead of just the requested one, and also stores
// intermediate values
void VL53L0XAsync::getSequenceStepTimeouts(SequenceStepEnables const * enables, SequenceStepTimeouts * timeouts)
{
  timeouts->pre_range_vcsel_period_pclks = getVcselPulsePeriod(VcselPeriodPreRange);

  timeouts->msrc_dss_tcc_mclks = readReg(MSRC_CONFIG_TIMEOUT_MACROP) + 1;
  timeouts->msrc_dss_tcc_us =
    timeoutMclksToMicroseconds(timeouts->msrc_dss_tcc_mclks,
                               timeouts->pre_range_vcsel_period_pclks);

  timeouts->pre_range_mclks =
    decodeTimeout(readReg16Bit(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
  timeouts->pre_range_us =
    timeoutMclksToMicroseconds(timeouts->pre_range_mclks,
                               timeouts->pre_range_vcsel_period_pclks);

  timeouts->final_range_vcsel_period_pclks = getVcselPulsePeriod(VcselPeriodFinalRange);

  timeouts->final_range_mclks =
    decodeTimeout(readReg16Bit(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI));

  if (enables->pre_range)
  {
    timeouts->final_range_mclks -= timeouts->pre_range_mclks;
  }

  timeouts->final_range_us =
    timeoutMclksToMicroseconds(timeouts->final_range_mclks,
                               timeouts->final_range_vcsel_period_pclks);
}

// Decode sequence step timeout in MCLKs from register value
// based on VL53L0X_decode_timeout()
// Note: the original function returned a uint32_t, but the return value is
// always stored in a uint16_t.
uint16_t VL53L0XAsync::decodeTimeout(uint16_t reg_val)
{
  // format: "(LSByte * 2^MSByte) + 1"
  return (uint16_t)((reg_val & 0x00FF) <<
         (uint16_t)((reg_val & 0xFF00) >> 8)) + 1;
}

// Encode sequence step timeout register value from timeout in MCLKs
// based on VL53L0X_encode_timeout()
// Note: the original function took a uint16_t, but the argument passed to it
// is always a uint16_t.
uint16_t VL53L0XAsync::encodeTimeout(uint16_t timeout_mclks)
{
  // format: "(LSByte * 2^MSByte) + 1"

  uint32_t ls_byte = 0;
  uint16_t ms_byte = 0;

  if (timeout_mclks > 0)
  {
    ls_byte = timeout_mclks - 1;

    while ((ls_byte & 0xFFFFFF00) > 0)
    {
      ls_byte >>= 1;
      ms_byte++;
    }

    return (ms_byte << 8) | (ls_byte & 0xFF);
  }
  else { return 0; }
}

// Convert sequence step timeout from MCLKs to microseconds with given VCSEL period in PCLKs
// based on VL53L0X_calc_timeout_us()
uint32_t VL53L0XAsync::timeoutMclksToMicroseconds(uint16_t timeout_period_mclks, uint8_t vcsel_period_pclks)
{
  uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);

  return ((timeout_period_mclks * macro_period_ns) + (macro_period_ns / 2)) / 1000;
}

// Convert sequence step timeout from microseconds to MCLKs with given VCSEL period in PCLKs
// based on VL53L0X_calc_timeout_mclks()
uint32_t VL53L0XAsync::timeoutMicrosecondsToMclks(uint32_t timeout_period_us, uint8_t vcsel_period_pclks)
{
  uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);

  return (((timeout_period_us * 1000) + (macro_period_ns / 2)) / macro_period_ns);
}


// based on VL53L0X_perform_single_ref_calibration()
void VL53L0XAsync::performSingleRefCalibration(uint8_t vhv_init_byte)
{
  writeReg(SYSRANGE_START, 0x01 | vhv_init_byte); // VL53L0X_REG_SYSRANGE_MODE_START_STOP

  startTimer(onPerformSingleRefCalibrationTimerExpired);
}


void VL53L0XAsync::onPerformSingleRefCalibrationTimerExpired()
{
    if ((readReg(RESULT_INTERRUPT_STATUS) & 0x07) == 0 ||
            readReg(I2C_SLAVE_DEVICE_ADDRESS) != address) {

        if (tickTimer()) {
            stopTimer(onPerformSingleRefCalibrationTimerExpired);
            shutdown();
            initFailed()->post();
        }

        return;
    }

    stopTimer(onPerformSingleRefCalibrationTimerExpired);
    writeReg(SYSTEM_INTERRUPT_CLEAR, 0x01);

    writeReg(SYSRANGE_START, 0x00);

    singleRefCalibration()->post();
}


void VL53L0XAsync::setVcselPeriodPreRange(unsigned char value)
{
    debugAssert(value == 12 || value == 14 || value == 16 || value == 18);

    mVcselPeriodPreRange = value;
}


void VL53L0XAsync::setVcselPeriodFinalRange(unsigned char value)
{
    debugAssert(value == 8 || value == 10 || value == 12 || value == 14);

    mVcselPeriodFinalRange = value;
}
