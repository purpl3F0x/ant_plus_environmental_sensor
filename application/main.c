/**
 * Copyright (c) 2020 - Stavros Avramidis
 *
 * TODO: get measurment only when page 1 is about to be sent
 */


#include "app_error.h"
#include "app_timer.h"
#include "app_util_platform.h"
#include "boards.h"
#include "bsp.h"
#include "nrf_error.h"
#include "nrf_nvic.h"
#include "nrf_strerror.h"
#include "nrf.h"

#include "nordic_common.h"
//#include "nrf_sdh_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ant.h"
#include "ant_key_manager.h"
#include "ant_env.h"
#include "ant_state_indicator.h"
#include "ant_env_utils.h"
#include "nrf_pwr_mgmt.h"

#include "spi.h"
#include "bme280.h"

//#include "softdevice_handler.h"
#include "nrf_sdm.h"
#include "bsp.h"
#include "app_timer.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nrf_drv_clock.h"
#include "nrf_drv_spi.h"
#include "nrf_drv_saadc.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"


#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"


#define BME280_HUMIDITY_OVERSAMPLING    BME280_OVERSAMPLING_8
#define BME280_TEMPERATURE_OVERSAMPLING BME280_OVERSAMPLING_4
#define BME280_PRESSURE_OVERSAMPLING    BME280_OVERSAMPLING_8
#define BME280_IIR                      BME280_IIR_16
#define BME280_DELAY                    BME280_STANDBY_500_MS

#define ENV_CHANNEL_NUMBER       0x00           /**< Channel number assigned to Enviroment profile. */
#define ANTPLUS_NETWORK_NUMBER   0              /**< Network number. */

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS   600     /**< Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION    6       /**< The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/
#define DIODE_FWD_VOLT_DROP_MILLIVOLTS  270     /**< Typical forward voltage drop of the diode . */
#define ADC_RES_10BIT                   1024       

#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_10BIT) * ADC_PRE_SCALING_COMPENSATION)

#define DFU_MAGIC_COMMAND             42017     ///< When this command num is sent via ant page 73 device enters DFU mode.

static volatile bool measure = true;
static nrf_saadc_value_t adc_buf[2];
//volatile int32_t max_24h_temp;
//volatile int32_t min_24h_temp;
//volatile int32_t max_24h_temp;


/** @snippet [ANT ENV TX Instance] */
void ant_env_evt_handler(ant_env_profile_t * p_profile, ant_env_evt_t event);


static void dont_hlt_but_catch_fire(void);

ENV_SENS_CHANNEL_CONFIG_DEF(m_ant_env,
                            ENV_CHANNEL_NUMBER,
                            CHAN_ID_TRANS_TYPE,
                            0,
                            ANTPLUS_NETWORK_NUM);


ENV_SENS_PROFILE_CONFIG_DEF(m_ant_env,
                            ANT_ENV_PAGE_1,
                            ant_env_evt_handler);

static ant_env_profile_t m_ant_env;
/** @snippet [ANT Enviroment TX Instance] */


NRF_SDH_ANT_OBSERVER(m_ant_observer, ANT_ENV_ANT_OBSERVER_PRIO,
                     ant_env_sens_evt_handler, &m_ant_env);


APP_TIMER_DEF(bme_timer_id);
APP_TIMER_DEF(battery_timer_id);


uint32_t get_rtc_counter_ms(void)
{
    return NRF_RTC1->COUNTER * 1000 * (APP_TIMER_CONFIG_RTC_FREQUENCY + 1) / APP_TIMER_CLOCK_FREQ;
;
}

void do_measurment_timer (void * p_context) {
    if (measure == false)
        measure = true;
}

void battery_timer_handler (void * p_context) {
    nrf_drv_saadc_sample();
}

/**@brief Function for handling the ADC interrupt.
 *
 * @details  This function will fetch the conversion result from the ADC, convert the value into
 *           percentage and send it to peer.
 */
void saadc_event_handler(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        nrf_saadc_value_t adc_result;
        uint16_t          batt_lvl_in_milli_volts;

        adc_result = p_event->data.done.p_buffer[0];

        APP_ERROR_CHECK(nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1));

        batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(adc_result) + DIODE_FWD_VOLT_DROP_MILLIVOLTS;

        NRF_LOG_INFO("Battery Voltage %u mV", batt_lvl_in_milli_volts);
        
        // Update page 82
        m_ant_env.page_82.fract_bat_volt = batt_lvl_in_milli_volts % 1000;
        m_ant_env.page_82.descriptive_bit_field.coarse_bat_volt = batt_lvl_in_milli_volts / 1000;

        // based on battery_level_in_percent() inaccuracy is key here ;)
        if (batt_lvl_in_milli_volts >= 3000)
            m_ant_env.page_82.descriptive_bit_field.battery_status = COMMON_PAGE_82_BAT_STATUS_NEW;
        else if (batt_lvl_in_milli_volts > 2900)
            m_ant_env.page_82.descriptive_bit_field.battery_status = COMMON_PAGE_82_BAT_STATUS_GOOD;
        else if (batt_lvl_in_milli_volts > 2600)
            m_ant_env.page_82.descriptive_bit_field.battery_status = COMMON_PAGE_82_BAT_STATUS_OK;
        else if (batt_lvl_in_milli_volts > 2100)
            m_ant_env.page_82.descriptive_bit_field.battery_status = COMMON_PAGE_82_BAT_STATUS_LOW;
        else
            m_ant_env.page_82.descriptive_bit_field.battery_status = COMMON_PAGE_82_BAT_STATUS_CRITICAL;
          
        m_ant_env.page_82.cumul_operating_time = get_rtc_counter_ms() / 16000;
    }

 }


void bme280_handler(void)
{
    int32_t  raw_t  = 0;
    uint64_t raw_p  = 0;
    uint32_t raw_h  = 0;

    
    bme280_read_measurements();

    raw_t = bme280_get_temperature();
    raw_p = bme280_get_pressure();
    raw_h = bme280_get_humidity();

    //NRF_LOG_INFO("Temp: "NRF_LOG_FLOAT_MARKER " *C", NRF_LOG_FLOAT(raw_t / 100.0));
    //NRF_LOG_INFO("Hum : " NRF_LOG_FLOAT_MARKER " %%Rh", NRF_LOG_FLOAT(raw_h / 1024.0));
    //NRF_LOG_INFO("Pres: " NRF_LOG_FLOAT_MARKER " hPa\n\r", NRF_LOG_FLOAT(raw_p / 256.0));
    //NRF_LOG_FLUSH();

    m_ant_env.page_1.current_temp = raw_t;
    m_ant_env.page_84.data_field_1 = (raw_h * 100) / 1024;
    m_ant_env.page_84.data_field_2 = (raw_p) / 2560;


}


/**@brief Function for handling ANT ENV events.
 */
/** @snippet [ANT ENV simulator call] */
void ant_env_evt_handler(ant_env_profile_t * p_profile, ant_env_evt_t event)
{
    nrf_pwr_mgmt_feed();

    switch (event)
    {
        case ANT_ENV_PAGE_0_UPDATED:
          break;

        case ANT_ENV_PAGE_1_UPDATED:
          m_ant_env.page_1.event_count++;
          break;

        case ANT_ENV_PAGE_70_REQUESTED:
          if (p_profile->page_70.page_number == ANT_ENV_PAGE_82)
          {
              nrf_drv_saadc_sample();   // Sample for the upcoming data page
          }
          break;

        case ANT_ENV_PAGE_73_REQUESTED:
          if (p_profile->page_73.command_number == DFU_MAGIC_COMMAND)
          {
              dont_hlt_but_catch_fire();
          }
          break;

        case ANT_ENV_PAGE_81_UPDATED:
          break;

        case ANT_ENV_PAGE_82_UPDATED:
          battery_timer_handler(NULL);
          break;

        default:
            break;
    }
}



/**@brief Function for ANT stack initialization.
 *
 * @details Initializes the SoftDevice and the ANT event interrupt.
 */
static void softdevice_setup(void)
{
    ret_code_t err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    ASSERT(nrf_sdh_is_enabled());

    err_code = nrf_sdh_ant_enable();
    APP_ERROR_CHECK(err_code);

    err_code = ant_plus_key_set(ANTPLUS_NETWORK_NUM);
    APP_ERROR_CHECK(err_code);
}


/**
 * @brief Function for ENV profile initialization.
 *
 * @details Initializes the ENV profile and open ANT channel.
 */
static void profile_setup(void)
{
/** @snippet [ANT ENV TX Profile Setup] */
    ret_code_t err_code;

    m_ant_env_channel_env_sens_config.device_number = CHAN_ID_DEV_NUM;

    err_code = ant_env_sens_init(&m_ant_env,
                                 ENV_SENS_CHANNEL_CONFIG(m_ant_env),
                                 ENV_SENS_PROFILE_CONFIG(m_ant_env));
    APP_ERROR_CHECK(err_code);
    
    m_ant_env.page_0.supported_pages = 0b0011;

    m_ant_env.page_0.transmission_info.default_trans_rate = ANT_ENV_PAGE_0_TRANS_RATE_4_HZ;
    m_ant_env.page_80.manufacturer_id = ENV_MFG_ID;   // development manufacturer ID
    m_ant_env.page_81.serial_number = NRF_FICR->DEVICEID[0];
    m_ant_env.page_80.hw_revision = ENV_HW_VERSION;
    m_ant_env.page_81.sw_revision_major = ENV_SW_VERSION_MAJOR;
    m_ant_env.page_81.sw_revision_minor = ENV_SW_VERSION_MAJOR;
    m_ant_env.page_80.model_number = ENV_MODEL_NUMBER;

    m_ant_env.page_82.descriptive_bit_field.battery_status = COMMON_PAGE_82_BAT_STATUS_NEW;
    m_ant_env.page_82.fract_bat_volt = 17;
    m_ant_env.page_82.cumul_operating_time = get_rtc_counter_ms() / 16000;
    m_ant_env.page_82.descriptive_bit_field.coarse_bat_volt = 3;
    m_ant_env.page_82.descriptive_bit_field.cumul_operating_time_res = 0;

    m_ant_env.page_84.subpage_1 = COMMON_PAGE_84_SUBFIELD_HUMIDITY;
    m_ant_env.page_84.subpage_2 = COMMON_PAGE_84_SUBFIELD_BAROMETRIC_PRESSURE;

    err_code = ant_env_sens_open(&m_ant_env);
    APP_ERROR_CHECK(err_code);

    //err_code = ant_state_indicator_channel_opened();
    //APP_ERROR_CHECK(err_code);
/** @snippet [ANT ENV TX Profile Setup] */
}


/**@brief Function for configuring ADC to do battery level conversion.
 */
static void adc_configure(void)
{
    ret_code_t err_code = nrf_drv_saadc_init(NULL, saadc_event_handler);
    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);
    err_code = nrf_drv_saadc_channel_init(0, &config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[0], 1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[1], 1);
    APP_ERROR_CHECK(err_code);
}


/**
 * @brief Exit and reset to Bootloader
 */
static void dont_hlt_but_catch_fire(void)
{
    // DONE: ooof
    NRF_LOG_INFO("GOING TO DFU MODE");

    APP_ERROR_CHECK(app_timer_stop_all());

    if (nrf_sdh_is_enabled())
    {
        // DONE: ...ig I could just kill the softdevice
        sd_power_gpregret_set(0, 0xB1);
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_DFU);
    }

    NRF_POWER->GPREGRET = 0xB1;
    __DSB();    // TODO: memory barried redundant (??)

    nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_DFU);
} 


int main(void)
{   
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    APP_ERROR_CHECK(nrf_pwr_mgmt_init());

    softdevice_setup();
    profile_setup();
    
    adc_configure();


    APP_ERROR_CHECK(app_timer_init());
    APP_ERROR_CHECK(app_timer_create(&bme_timer_id, APP_TIMER_MODE_REPEATED, do_measurment_timer));
    APP_ERROR_CHECK(app_timer_create(&battery_timer_id, APP_TIMER_MODE_REPEATED, battery_timer_handler));
    
    bsp_init(BSP_INIT_LEDS, NULL);

    battery_timer_handler(NULL);
    do_measurment_timer(NULL);

    spi_init();
    APP_ERROR_CHECK(bme280_init());
    bme280_set_oversampling_hum(BME280_HUMIDITY_OVERSAMPLING);
    bme280_set_oversampling_temp(BME280_TEMPERATURE_OVERSAMPLING);
    bme280_set_oversampling_press(BME280_PRESSURE_OVERSAMPLING);
    bme280_set_iir(BME280_IIR);
    bme280_set_interval(BME280_DELAY);
    bme280_set_mode(BME280_MODE_NORMAL);
  
    

    NRF_LOG_INFO("BME280 Sesnsor Started");
    NRF_LOG_FLUSH();
    
    APP_ERROR_CHECK(app_timer_start(bme_timer_id, APP_TIMER_TICKS(1000), NULL));
    APP_ERROR_CHECK(app_timer_start(battery_timer_id, APP_TIMER_TICKS(10000), NULL));

 
    for (;;)
    {   
        if (measure)
        {
          bme280_handler();
          measure = false;
        }
        NRF_LOG_FLUSH();
        nrf_pwr_mgmt_run();
    }
}
