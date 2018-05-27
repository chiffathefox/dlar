// Most of the functionality of this library is based on the VL53L0X API
// provided by ST (STSW-IMG005), and some of the explanatory comments are quoted
// or paraphrased from the API source code, API user manual (UM2039), and the
// VL53L0X datasheet.

#include <math.h>

#include "TWI.hpp"
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
    mTimer.expired()->disconnect(this, nullptr);          \
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

    if (startContinuous()) {
        debugAssert(!mTimer.running());
        EventObjectConnect(&mTimer, expired, this, onRangeReadyTimerExpired);
        mTimer.setTimeout(measurement_timing_budget_us / 1000 + 1);
        mTimer.start();
    } else {
        shutdown();
        rangeError()->post();
    }
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


void VL53L0XAsync::onRangeError()
{
    EventObjectDisconnect(&mTimer, expired, this, onRangeReadyTimerExpired);
    mTimer.stop();
    shutdown();
    rangeError()->post();
}


void VL53L0XAsync::onRangeReadyTimerExpired()
{
    ValueContainer<unsigned char> reg = readReg(RESULT_INTERRUPT_STATUS);

    if (!reg.ok() || (reg.value() & 0x07) == 0) {
        onRangeError();

        return;
    }

    reg = readReg(I2C_SLAVE_DEVICE_ADDRESS);

    if (!reg.ok() || reg.value() != address) {
        onRangeError();

        return;
    }

    ValueContainer<uint16_t> reg16 = readReg16Bit(RESULT_RANGE_STATUS + 10);

    if (!reg16.ok()) {
        onRangeError();

        return;
    }

    mRange = reg16.value();

    if (writeReg(SYSTEM_INTERRUPT_CLEAR, 0x01)) {
        rangeReady()->post();
    } else {
        onRangeError();
    }
}


void VL53L0XAsync::onSpadInfoTimerExpired()
{
    bool rc = true;
    ValueContainer<unsigned char> reg = readReg(0x83);

    if (!reg.ok()) {
        stopTimer(onSpadInfoTimerExpired);
        shutdown();
        initFailed()->post();

        return;
    }

    if (reg.value() == 0) {
        if (tickTimer()) {
            stopTimer(onSpadInfoTimerExpired);
            shutdown();
            initFailed()->post();
        }

        return;
    }

    stopTimer(onSpadInfoTimerExpired);

    rc &= writeReg(0x83, 0x01);
    reg = readReg(0x92);

    if (!reg.ok()) {
        shutdown();
        initFailed()->post();

        return;
    }

    uint8_t spad_count = reg.value() & 0x7f;
    bool spad_type_is_aperture = (reg.value() >> 7) & 0x01;

    rc &= writeReg(0x81, 0x00);
    rc &= writeReg(0xFF, 0x06);

    reg = readReg(0x83);

    if (!reg.ok()) {
        shutdown();
        initFailed()->post();

        return;
    }

    rc &= writeReg(0x83, reg.value()  & ~0x04);
    rc &= writeReg(0xFF, 0x01);
    rc &= writeReg(0x00, 0x01);

    rc &= writeReg(0xFF, 0x00);
    rc &= writeReg(0x80, 0x00);

    // VL53L0X_StaticInit() begin

    // The SPAD map (RefGoodSpadMap) is read by VL53L0X_get_info_from_device() in
    // the API, but the same data seems to be more easily readable from
    // GLOBAL_CONFIG_SPAD_ENABLES_REF_0 through _6, so read it from there
    uint8_t ref_spad_map[6];
    rc &= readMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

    // -- VL53L0X_set_reference_spads() begin (assume NVM values are valid)

    rc &= writeReg(0xFF, 0x01);
    rc &= writeReg(DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
    rc &= writeReg(DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
    rc &= writeReg(0xFF, 0x00);
    rc &= writeReg(GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);

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

    rc &= writeMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

    // -- VL53L0X_set_reference_spads() end

    // -- VL53L0X_load_tuning_settings() begin
    // DefaultTuningSettings from vl53l0x_tuning.h

    rc &= writeReg(0xFF, 0x01);
    rc &= writeReg(0x00, 0x00);

    rc &= writeReg(0xFF, 0x00);
    rc &= writeReg(0x09, 0x00);
    rc &= writeReg(0x10, 0x00);
    rc &= writeReg(0x11, 0x00);

    rc &= writeReg(0x24, 0x01);
    rc &= writeReg(0x25, 0xFF);
    rc &= writeReg(0x75, 0x00);

    rc &= writeReg(0xFF, 0x01);
    rc &= writeReg(0x4E, 0x2C);
    rc &= writeReg(0x48, 0x00);
    rc &= writeReg(0x30, 0x20);

    rc &= writeReg(0xFF, 0x00);
    rc &= writeReg(0x30, 0x09);
    rc &= writeReg(0x54, 0x00);
    rc &= writeReg(0x31, 0x04);
    rc &= writeReg(0x32, 0x03);
    rc &= writeReg(0x40, 0x83);
    rc &= writeReg(0x46, 0x25);
    rc &= writeReg(0x60, 0x00);
    rc &= writeReg(0x27, 0x00);
    rc &= writeReg(0x50, 0x06);
    rc &= writeReg(0x51, 0x00);
    rc &= writeReg(0x52, 0x96);
    rc &= writeReg(0x56, 0x08);


    rc &= writeReg(0x57, 0x30);

    rc &= writeReg(0x61, 0x00);
    rc &= writeReg(0x62, 0x00);
    rc &= writeReg(0x64, 0x00);
    rc &= writeReg(0x65, 0x00);
    rc &= writeReg(0x66, 0xA0);

    rc &= writeReg(0xFF, 0x01);
    rc &= writeReg(0x22, 0x32);
    rc &= writeReg(0x47, 0x14);
    rc &= writeReg(0x49, 0xFF);
    rc &= writeReg(0x4A, 0x00);

    rc &= writeReg(0xFF, 0x00);
    rc &= writeReg(0x7A, 0x0A);
    rc &= writeReg(0x7B, 0x00);
    rc &= writeReg(0x78, 0x21);

    rc &= writeReg(0xFF, 0x01);
    rc &= writeReg(0x23, 0x34);
    rc &= writeReg(0x42, 0x00);
    rc &= writeReg(0x44, 0xFF);
    rc &= writeReg(0x45, 0x26);
    rc &= writeReg(0x46, 0x05);
    rc &= writeReg(0x40, 0x40);
    rc &= writeReg(0x0E, 0x06);
    rc &= writeReg(0x20, 0x1A);
    rc &= writeReg(0x43, 0x40);

    rc &= writeReg(0xFF, 0x00);
    rc &= writeReg(0x34, 0x03);
    rc &= writeReg(0x35, 0x44);

    rc &= writeReg(0xFF, 0x01);
    rc &= writeReg(0x31, 0x04);
    rc &= writeReg(0x4B, 0x09);
    rc &= writeReg(0x4C, 0x05);
    rc &= writeReg(0x4D, 0x04);

    rc &= writeReg(0xFF, 0x00);
    rc &= writeReg(0x44, 0x00);
    rc &= writeReg(0x45, 0x20);
    rc &= writeReg(0x47, 0x08);
    rc &= writeReg(0x48, 0x28);
    rc &= writeReg(0x67, 0x00);
    rc &= writeReg(0x70, 0x04);
    rc &= writeReg(0x71, 0x01);
    rc &= writeReg(0x72, 0xFE);
    rc &= writeReg(0x76, 0x00);
    rc &= writeReg(0x77, 0x00);

    rc &= writeReg(0xFF, 0x01);
    rc &= writeReg(0x0D, 0x01);

    rc &= writeReg(0xFF, 0x00);
    rc &= writeReg(0x80, 0x01);
    rc &= writeReg(0x01, 0xF8);

    rc &= writeReg(0xFF, 0x01);
    rc &= writeReg(0x8E, 0x01);
    rc &= writeReg(0x00, 0x01);
    rc &= writeReg(0xFF, 0x00);
    rc &= writeReg(0x80, 0x00);

    // -- VL53L0X_load_tuning_settings() end

    // "Set interrupt config to new sample ready"
    // -- VL53L0X_SetGpioConfig() begin

    rc &= writeReg(SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);

    reg = readReg(GPIO_HV_MUX_ACTIVE_HIGH);

    if (!reg.ok()) {
        shutdown();
        initFailed()->post();

        return;
    }

    rc &= writeReg(GPIO_HV_MUX_ACTIVE_HIGH, reg.value() & ~0x10); // active low
    rc &= writeReg(SYSTEM_INTERRUPT_CLEAR, 0x01);

    // -- VL53L0X_SetGpioConfig() end

    ValueContainer<uint32_t> budget = getMeasurementTimingBudget();

    if (!budget.ok()) {
        shutdown();
        initFailed()->post();

        return;
    }

    measurement_timing_budget_us = budget.value();

    // "Disable MSRC and TCC by default"
    // MSRC = Minimum Signal Rate Check
    // TCC = Target CentreCheck
    // -- VL53L0X_SetSequenceStepEnable() begin

    rc &= writeReg(SYSTEM_SEQUENCE_CONFIG, 0xE8);

    // -- VL53L0X_SetSequenceStepEnable() end

    // "Recalculate timing budget"
    rc &= setMeasurementTimingBudget(measurement_timing_budget_us);

    // VL53L0X_StaticInit() end

    // VL53L0X_PerformRefCalibration() begin (VL53L0X_perform_ref_calibration())

    // -- VL53L0X_perform_vhv_calibration() begin


    rc &= writeReg(SYSTEM_SEQUENCE_CONFIG, 0x01);

    if (rc) {
        singleRefCalibration()->disconnect(this, nullptr);
        EventObjectOnce(this, singleRefCalibration, this, onVhvCalibration);
        performSingleRefCalibration(0x40);
    } else {
        shutdown();
        initFailed()->post();
    }

    // -- VL53L0X_perform_vhv_calibration() end
}


void VL53L0XAsync::onVhvCalibration()
{
    bool rc = true;
    SequenceStepEnables enables;
    SequenceStepTimeouts timeouts;

    if (!getSequenceStepEnables(&enables) ||
            !getSequenceStepTimeouts(&enables, &timeouts)) {

        shutdown();
        initFailed()->post();

        return;
    }

    /* Set VcselPeriodPreRange */

    switch (mVcselPeriodPreRange) {


    case 12:

        rc &= writeReg(PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x18);

        break;


    case 14:

        rc &= writeReg(PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x30);

        break;


    case 16:

        rc &= writeReg(PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x40);

        break;


    case 18:

        rc &= writeReg(PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x50);

        break;


    }

    rc &= writeReg(PRE_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);
    rc &= writeReg(PRE_RANGE_CONFIG_VCSEL_PERIOD,
            encodeVcselPeriod(mVcselPeriodPreRange));

    // update timeouts

    // set_sequence_step_timeout() begin
    // (SequenceStepId == VL53L0X_SEQUENCESTEP_PRE_RANGE)

    uint16_t new_pre_range_timeout_mclks =
      timeoutMicrosecondsToMclks(timeouts.pre_range_us, mVcselPeriodPreRange);

    rc &= writeReg16Bit(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI,
      encodeTimeout(
          timeoutMicrosecondsToMclks(timeouts.pre_range_us,
              mVcselPeriodPreRange)));

    // set_sequence_step_timeout() end

    // set_sequence_step_timeout() begin
    // (SequenceStepId == VL53L0X_SEQUENCESTEP_MSRC)

    uint16_t new_msrc_timeout_mclks =
      timeoutMicrosecondsToMclks(timeouts.msrc_dss_tcc_us,
              mVcselPeriodPreRange);

    rc &= writeReg(MSRC_CONFIG_TIMEOUT_MACROP,
      (new_msrc_timeout_mclks > 256) ? 255 : (new_msrc_timeout_mclks - 1));

    // set_sequence_step_timeout() end

    /* Set VcselPeriodFinalRange */

    switch (mVcselPeriodFinalRange) {


    case 8:

        rc &= writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x10);
        rc &= writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
        rc &= writeReg(GLOBAL_CONFIG_VCSEL_WIDTH, 0x02);
        rc &= writeReg(ALGO_PHASECAL_CONFIG_TIMEOUT, 0x0C);
        rc &= writeReg(0xFF, 0x01);
        rc &= writeReg(ALGO_PHASECAL_LIM, 0x30);
        rc &= writeReg(0xFF, 0x00);

        break;


    case 10:

        rc &= writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x28);
        rc &= writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
        rc &= writeReg(GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
        rc &= writeReg(ALGO_PHASECAL_CONFIG_TIMEOUT, 0x09);
        rc &= writeReg(0xFF, 0x01);
        rc &= writeReg(ALGO_PHASECAL_LIM, 0x20);
        rc &= writeReg(0xFF, 0x00);

        break;


    case 12:

        rc &= writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x38);
        rc &= writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
        rc &= writeReg(GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
        rc &= writeReg(ALGO_PHASECAL_CONFIG_TIMEOUT, 0x08);
        rc &= writeReg(0xFF, 0x01);
        rc &= writeReg(ALGO_PHASECAL_LIM, 0x20);
        rc &= writeReg(0xFF, 0x00);

        break;


    case 14:

        rc &= writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x48);
        rc &= writeReg(FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
        rc &= writeReg(GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
        rc &= writeReg(ALGO_PHASECAL_CONFIG_TIMEOUT, 0x07);
        rc &= writeReg(0xFF, 0x01);
        rc &= writeReg(ALGO_PHASECAL_LIM, 0x20);
        rc &= writeReg(0xFF, 0x00);

        break;


    }

    // apply new VCSEL period
    rc &= writeReg(FINAL_RANGE_CONFIG_VCSEL_PERIOD,
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

    rc &= writeReg16Bit(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI,
      encodeTimeout(new_final_range_timeout_mclks));

    // set_sequence_step_timeout end

    rc &= setMeasurementTimingBudget(measurement_timing_budget_us);


  // -- VL53L0X_perform_phase_calibration() begin

    rc &= writeReg(SYSTEM_SEQUENCE_CONFIG, 0x02);

    if (rc) {
        singleRefCalibration()->disconnect(this, nullptr);
        EventObjectOnce(this, singleRefCalibration, this, onPhaseCalibration);
        performSingleRefCalibration(0x00);
    } else {
        shutdown();
        initFailed()->post();
    }

  // -- VL53L0X_perform_phase_calibration() end
}


void VL53L0XAsync::onPhaseCalibration()
{
    // "restore the previous Sequence Config"
    
    if (writeReg(SYSTEM_SEQUENCE_CONFIG, 0xE8)) {
        initFinished()->post();
    } else {
        shutdown();
        initFailed()->post();
    }
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
    bool rc = true;

    if (mTimer.running()) {
        sDrivingXshut = false;
    }

    stopTimer(onStartedTimerExpired);

    if (address != DefaultAddress) {
        unsigned char tmpAddress = address;

        address = DefaultAddress;

        rc &= writeReg(I2C_SLAVE_DEVICE_ADDRESS, tmpAddress & 0x7F);
        address = tmpAddress;
    }

    // VL53L0X_DataInit() begin

    // sensor uses 1V8 mode for I/O by default; switch to 2V8 mode if necessary
    const bool io_2v8 = true;
    ValueContainer<unsigned char> reg;

    if (io_2v8)
    {
        reg = readReg(VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV);

        if (!reg.ok()) {
            shutdown();
            initFailed()->post();

            return;
        }

        rc &= writeReg(VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV, reg.value() | 0x01);
    }

    // "Set I2C standard mode"
    rc &= writeReg(0x88, 0x00);

    rc &= writeReg(0x80, 0x01);
    rc &= writeReg(0xFF, 0x01);
    rc &= writeReg(0x00, 0x00);

    reg = readReg(0x91);

    if (!reg.ok()) {
        shutdown();
        initFailed()->post();

        return;
    }

    stop_variable = reg.value();
    rc &= writeReg(0x00, 0x01);
    rc &= writeReg(0xFF, 0x00);
    rc &= writeReg(0x80, 0x00);

    // disable SIGNAL_RATE_MSRC (bit 1) and SIGNAL_RATE_PRE_RANGE (bit 4) limit checks
    reg = readReg(MSRC_CONFIG_CONTROL);

    if (!reg.ok()) {
        shutdown();
        initFailed()->post();

        return;
    }

    rc &= writeReg(MSRC_CONFIG_CONTROL, reg.value() | 0x12);

    // set final range signal rate limit to 0.25 MCPS (million counts per second)
    setSignalRateLimit(0.25);

    rc &= writeReg(SYSTEM_SEQUENCE_CONFIG, 0xFF);

    if (rc) {
        getSpadInfo();
    } else {
        shutdown();
        initFailed()->post();
    }
}

// Write an 8-bit register
bool VL53L0XAsync::writeReg(uint8_t reg, uint8_t value)
{
    TWI *twi = TWI::instance();
    twi->beginTransmission(address);
    twi->write(reg);
    twi->write(value);

    return twi->endTransmission() == TWI::StatusSuccess;
}

// Write a 16-bit register
bool VL53L0XAsync::writeReg16Bit(uint8_t reg, uint16_t value)
{
    TWI *twi = TWI::instance();
    twi->beginTransmission(address);
    twi->write(reg);
    twi->write((value >> 8) & 0xFF); // value high byte
    twi->write( value       & 0xFF); // value low byte

    return twi->endTransmission() == TWI::StatusSuccess;
}

// Write a 32-bit register
bool VL53L0XAsync::writeReg32Bit(uint8_t reg, uint32_t value)
{
    TWI *twi = TWI::instance();
    twi->beginTransmission(address);
    twi->write(reg);
    twi->write((unsigned char) (value >> 24) & 0xFF); // value highest byte
    twi->write((unsigned char) (value >> 16) & 0xFF);
    twi->write((unsigned char) (value >>  8) & 0xFF);
    twi->write((unsigned char)  value        & 0xFF); // value lowest byte

    return twi->endTransmission() == TWI::StatusSuccess;
}

// Read an 8-bit register
ValueContainer<unsigned char> VL53L0XAsync::readReg(uint8_t reg)
{
    uint8_t value;

    TWI *twi = TWI::instance();
    twi->beginTransmission(address);
    twi->write(reg);
    twi->endTransmission();

    unsigned char n = twi->requestFrom(address, (unsigned char) 1);
    
    if (n == 1) {
        return ValueContainer<unsigned char>(twi->read(), true);
    }

    return ValueContainer<unsigned char>(0, false);
}

// Read a 16-bit register
ValueContainer<uint16_t> VL53L0XAsync::readReg16Bit(uint8_t reg)
{
    TWI *twi = TWI::instance();
    twi->beginTransmission(address);
    twi->write(reg);
    twi->endTransmission();

    unsigned char n = twi->requestFrom(address, (unsigned char) 2);

    if (n == 2) {
        uint16_t value;

        value  = (uint16_t)twi->read() << 8;
        value |=           twi->read();

        return ValueContainer<uint16_t>(value, true);
    }

    return ValueContainer<uint16_t>(0, false);
}


// Write an arbitrary number of bytes from the given array to the sensor,
// starting at the given register
bool VL53L0XAsync::writeMulti(uint8_t reg, uint8_t const * src, uint8_t count)
{
    TWI *twi = TWI::instance();
    twi->beginTransmission(address);
    twi->write(reg);

    while (count-- > 0)
    {
        twi->write(*(src++));
    }

    return twi->endTransmission() == TWI::StatusSuccess;
}

// Read an arbitrary number of bytes from the sensor, starting at the given
// register, into the given array
bool VL53L0XAsync::readMulti(uint8_t reg, uint8_t * dst, uint8_t count)
{
    TWI *twi = TWI::instance();
    twi->beginTransmission(address);
    twi->write(reg);
    twi->endTransmission();

    unsigned char n = twi->requestFrom(address, count);

    if (n != count) {
        debugWarn() << n << "!=" << count;
        return false;
    }

    while (count-- > 0)
    {
        *(dst++) = twi->read();
    }

    return true;
}

// Set the return signal rate limit check value in units of MCPS (mega counts
// per second). "This represents the amplitude of the signal reflected from the
// target and detected by the device"; setting this limit presumably determines
// the minimum measurement necessary for the sensor to report a valid reading.
// Setting a lower limit increases the potential range of the sensor but also
// seems to increase the likelihood of getting an inaccurate reading because of
// unwanted reflections from objects other than the intended target.
// Defaults to 0.25 MCPS as initialized by the ST API and this library.
bool VL53L0XAsync::setSignalRateLimit(float limit_Mcps)
{
    debugAssert(limit_Mcps >= 0 && limit_Mcps <= 511.99);

    // Q9.7 fixed point format (9 integer bits, 7 fractional bits)
    return writeReg16Bit(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT,
            limit_Mcps * (1 << 7));
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


    /*
    if ((!initFinished()->emitting() && initFinished()->lastEmitted() == -1) ||
            did_timeout) {

        return false;
    }
    */

    if (budget_us < MinTimingBudget) { return false; }

    uint32_t used_budget_us = StartOverhead + EndOverhead;

    if (!getSequenceStepEnables(&enables) ||
            !getSequenceStepTimeouts(&enables, &timeouts)) {

        return false;
    }

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

        if (!writeReg16Bit(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI,
                    encodeTimeout(final_range_timeout_mclks)))
        {
            return false;
        }

        // set_sequence_step_timeout() end

        measurement_timing_budget_us = budget_us; // store for internal reuse
    }
    return true;
}

// Get the measurement timing budget in microseconds
// based on VL53L0X_get_measurement_timing_budget_micro_seconds()
// in us
ValueContainer<uint32_t> VL53L0XAsync::getMeasurementTimingBudget(void)
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

    if (!getSequenceStepEnables(&enables) ||
            !getSequenceStepTimeouts(&enables, &timeouts)) {

        return ValueContainer<uint32_t>(0, false);
    }

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

    return ValueContainer<uint32_t>(budget_us, true);
}


// Start continuous ranging measurements. If period_ms (optional) is 0 or not
// given, continuous back-to-back mode is used (the sensor takes measurements as
// often as possible); otherwise, continuous timed mode is used, with the given
// inter-measurement period in milliseconds determining how often the sensor
// takes a measurement.
// based on VL53L0X_StartMeasurement()
bool VL53L0XAsync::startContinuous(uint32_t period_ms)
{
    bool rc = true;

    rc &= writeReg(0x80, 0x01);
    rc &= writeReg(0xFF, 0x01);
    rc &= writeReg(0x00, 0x00);
    rc &= writeReg(0x91, stop_variable);
    rc &= writeReg(0x00, 0x01);
    rc &= writeReg(0xFF, 0x00);
    rc &= writeReg(0x80, 0x00);

    if (period_ms != 0)
    {
        // continuous timed mode

        // VL53L0X_SetInterMeasurementPeriodMilliSeconds() begin

        ValueContainer<uint16_t> reg = readReg16Bit(OSC_CALIBRATE_VAL);

        if (!reg.ok()) {
            return false;
        }

        if (reg.value() != 0)
        {
            period_ms *= reg.value();
        }

        rc &= writeReg32Bit(SYSTEM_INTERMEASUREMENT_PERIOD, period_ms);

        // VL53L0X_SetInterMeasurementPeriodMilliSeconds() end

        rc &= writeReg(SYSRANGE_START, 0x04); // VL53L0X_REG_SYSRANGE_MODE_TIMED
    } else {
        // continuous back-to-back mode
        rc &= writeReg(SYSRANGE_START, 0x02); // VL53L0X_REG_SYSRANGE_MODE_BACKTOBACK
    }

    return rc;
}

// Private Methods /////////////////////////////////////////////////////////////

// Get reference SPAD (single photon avalanche diode) count and type
// based on VL53L0X_get_info_from_device(),
// but only gets reference SPAD count and type
void VL53L0XAsync::getSpadInfo()
{
    bool rc = true;

    rc &= writeReg(0x80, 0x01);
    rc &= writeReg(0xFF, 0x01);
    rc &= writeReg(0x00, 0x00);

    rc &= writeReg(0xFF, 0x06);

    ValueContainer<unsigned char> reg = readReg(0x83);

    if (!reg.ok()) {
        shutdown();
        initFailed()->post();

        return;
    }

    rc &= writeReg(0x83, reg.value() | 0x04);
    rc &= writeReg(0xFF, 0x07);
    rc &= writeReg(0x81, 0x01);

    rc &= writeReg(0x80, 0x01);

    rc &= writeReg(0x94, 0x6b);
    rc &= writeReg(0x83, 0x00);

    if (rc) {
        startTimer(onSpadInfoTimerExpired);
    } else {
        shutdown();
        initFailed()->post();
    }
}

// Get sequence step enables
// based on VL53L0X_GetSequenceStepEnables()
bool VL53L0XAsync::getSequenceStepEnables(SequenceStepEnables * enables)
{
    ValueContainer<unsigned char> reg = readReg(SYSTEM_SEQUENCE_CONFIG);

    if (reg.ok()) {
        enables->tcc          = (reg.value() >> 4) & 0x1;
        enables->dss          = (reg.value() >> 3) & 0x1;
        enables->msrc         = (reg.value() >> 2) & 0x1;
        enables->pre_range    = (reg.value() >> 6) & 0x1;
        enables->final_range  = (reg.value() >> 7) & 0x1;

        return true;
    }

    return false;
}

// Get sequence step timeouts
// based on get_sequence_step_timeout(),
// but gets all timeouts instead of just the requested one, and also stores
// intermediate values
bool VL53L0XAsync::getSequenceStepTimeouts(SequenceStepEnables const * enables,
        SequenceStepTimeouts * timeouts)
{
    ValueContainer<unsigned char> reg = readReg(PRE_RANGE_CONFIG_VCSEL_PERIOD);

    if (!reg.ok()) {
        return false;
    }

    timeouts->pre_range_vcsel_period_pclks = decodeVcselPeriod(reg.value());

    reg = readReg(MSRC_CONFIG_TIMEOUT_MACROP);

    if (!reg.ok()) {
        return false;
    }

    timeouts->msrc_dss_tcc_mclks = reg.value() + 1;
    timeouts->msrc_dss_tcc_us =
        timeoutMclksToMicroseconds(timeouts->msrc_dss_tcc_mclks,
                timeouts->pre_range_vcsel_period_pclks);

    ValueContainer<uint16_t> reg16 =
        readReg16Bit(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI);

    if (!reg16.ok()) {
        return false;
    }

    timeouts->pre_range_mclks = decodeTimeout(reg16.value());
    timeouts->pre_range_us =
        timeoutMclksToMicroseconds(timeouts->pre_range_mclks,
                timeouts->pre_range_vcsel_period_pclks);

    reg = readReg(FINAL_RANGE_CONFIG_VCSEL_PERIOD);

    if (!reg.ok()) {
        return false;
    }

    timeouts->final_range_vcsel_period_pclks = decodeVcselPeriod(reg.value());

    reg16 = readReg16Bit(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI);

    if (!reg16.ok()) {
        return false;
    }

    timeouts->final_range_mclks = decodeTimeout(reg16.value());

    if (enables->pre_range)
    {
        timeouts->final_range_mclks -= timeouts->pre_range_mclks;
    }

    timeouts->final_range_us =
        timeoutMclksToMicroseconds(timeouts->final_range_mclks,
                timeouts->final_range_vcsel_period_pclks);

    return true;
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
    if (writeReg(SYSRANGE_START, 0x01 | vhv_init_byte)) {
        startTimer(onPerformSingleRefCalibrationTimerExpired);
    } else {
        shutdown();
        initFailed()->post();
    }
}


void VL53L0XAsync::onSingleRefCalibrationError()
{
    stopTimer(onPerformSingleRefCalibrationTimerExpired);
    shutdown();
    initFailed()->post();
}


void VL53L0XAsync::onPerformSingleRefCalibrationTimerExpired()
{
    ValueContainer<unsigned char> reg = readReg(RESULT_INTERRUPT_STATUS);

    if (!reg.ok()) {
        onSingleRefCalibrationError();

        return;
    }

    if ((reg.value() & 0x07) == 0) {
        if (tickTimer()) {
            onSingleRefCalibrationError();
        }

        return;
    }

    reg = readReg(I2C_SLAVE_DEVICE_ADDRESS);

    if (!reg.ok() || reg.value() != address) {
        onSingleRefCalibrationError();

        return;
    }

    stopTimer(onPerformSingleRefCalibrationTimerExpired);

    if (writeReg(SYSTEM_INTERRUPT_CLEAR, 0x01) &&
            writeReg(SYSRANGE_START, 0x00)) {

        singleRefCalibration()->post();
    } else {
        shutdown();
        initFailed()->post();
    }
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
