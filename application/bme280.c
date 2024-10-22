/*
 * BOSH BME280 driver.
 *
 * Copyright (c) 2016, Offcode Ltd. All rights reserved.
 * Author: Janne Rosberg <janne@offcode.fi>
 *
 * Reference: BST-BME280-DS001-11 | Revision 1.2 | October 2015
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice,   this list of conditions and the following disclaimer.
 *    * Redistributions in  binary form must  reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the RuuviTag nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND  ANY  EXPRESS  OR  IMPLIED WARRANTIES,  INCLUDING,  BUT NOT LIMITED TO,
 * THE  IMPLIED  WARRANTIES  OF MERCHANTABILITY  AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY, OR
 * CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT  LIMITED  TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE, DATA, OR PROFITS;  OR BUSINESS
 * INTERRUPTION)  HOWEVER CAUSED AND  ON ANY THEORY OF LIABILITY,  WHETHER IN
 * CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Changelog
 *  2016-11-17 Otso Jousimaa (otso@ruuvi.com): Port function calls to use Ruuvi SPI driver 
 *  2016-11-18 Otso Jousimaa (otso@ruuvi.com): Add timer to poll data 
 *  2017-01-28 Otso Jousimaa Add comments.
 *  2017-04-06: Add t_sb register value. 
 *  2017-08-12 Otso Jousimaa (otso@ruuvi.com): Add Error checking, IIR filtering
 */

#include <stdint.h>
#include <stdbool.h>

#include "bme280.h"
#include "spi.h"
//#include "init.h" //Timer ticks - todo: refactor

// #define NRF_LOG_MODULE_NAME "BME280"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"


struct bme280_driver bme280; /* global instance */

/* Prototypes */
void timer_bme280_event_handler(void* p_context);

/** state variable **/
static uint8_t current_mode = BME280_MODE_SLEEP;
static uint8_t current_interval = BME280_STANDBY_1000_MS;

BME280_Ret bme280_init()
{
  //Return error if not in sleep
  if(BME280_MODE_SLEEP != current_mode){ return BME280_RET_ILLEGAL; }
  /* Initialize SPI */
  if (!spi_isInitialized())
  {
    spi_init();
  }
  uint8_t reg = bme280_read_reg(BME280REG_ID);
  bme280.sensor_available = false;

	if (BME280_ID_VALUE == reg)
  {
	  bme280.sensor_available = true;
  }
	else
  {
    //Assume that 0x00 means no response. Other values are self-test errors (invalid who-am-i).
		return (0x00 == reg) ? BME280_RET_ERROR : BME280_RET_ERROR_SELFTEST;
  }

  // load calibration data...
  bme280.cp.dig_T1  = bme280_read_reg(BME280REG_CALIB_00);
  bme280.cp.dig_T1 |= bme280_read_reg(BME280REG_CALIB_00+1) << 8;
  bme280.cp.dig_T2  = bme280_read_reg(BME280REG_CALIB_00+2);
  bme280.cp.dig_T2 |= bme280_read_reg(BME280REG_CALIB_00+3) << 8;
  bme280.cp.dig_T3  = bme280_read_reg(BME280REG_CALIB_00+4);
  bme280.cp.dig_T3 |= bme280_read_reg(BME280REG_CALIB_00+5) << 8;

  bme280.cp.dig_P1  = bme280_read_reg(BME280REG_CALIB_00+6);
  bme280.cp.dig_P1 |= bme280_read_reg(BME280REG_CALIB_00+7) << 8;
  bme280.cp.dig_P2  = bme280_read_reg(BME280REG_CALIB_00+8);
  bme280.cp.dig_P2 |= bme280_read_reg(BME280REG_CALIB_00+9) << 8;
  bme280.cp.dig_P3  = bme280_read_reg(BME280REG_CALIB_00+10);
  bme280.cp.dig_P3 |= bme280_read_reg(BME280REG_CALIB_00+11) << 8;
  bme280.cp.dig_P4  = bme280_read_reg(BME280REG_CALIB_00+12);
  bme280.cp.dig_P4 |= bme280_read_reg(BME280REG_CALIB_00+13) << 8;
  bme280.cp.dig_P5  = bme280_read_reg(BME280REG_CALIB_00+14);
  bme280.cp.dig_P5 |= bme280_read_reg(BME280REG_CALIB_00+15) << 8;
  bme280.cp.dig_P6  = bme280_read_reg(BME280REG_CALIB_00+16);
  bme280.cp.dig_P6 |= bme280_read_reg(BME280REG_CALIB_00+17) << 8;
  bme280.cp.dig_P7  = bme280_read_reg(BME280REG_CALIB_00+18);
  bme280.cp.dig_P7 |= bme280_read_reg(BME280REG_CALIB_00+19) << 8;
  bme280.cp.dig_P8  = bme280_read_reg(BME280REG_CALIB_00+20);
  bme280.cp.dig_P8 |= bme280_read_reg(BME280REG_CALIB_00+21) << 8;
  bme280.cp.dig_P9  = bme280_read_reg(BME280REG_CALIB_00+22);
  bme280.cp.dig_P9 |= bme280_read_reg(BME280REG_CALIB_00+23) << 8;

  bme280.cp.dig_H1  = bme280_read_reg(0xA1);
  bme280.cp.dig_H2  = bme280_read_reg(0xE1);
  bme280.cp.dig_H2 |= bme280_read_reg(0xE2) << 8;
  bme280.cp.dig_H3  = bme280_read_reg(0xE3);

  bme280.cp.dig_H4  = bme280_read_reg(0xE4) << 4;		// 11:4
  bme280.cp.dig_H4 |= bme280_read_reg(0xE5) & 0x0f;	// 3:0

  bme280.cp.dig_H5  = bme280_read_reg(0xE5) >> 4;		// 3:0
  bme280.cp.dig_H5 |= bme280_read_reg(0xE6) << 4;		// 11:4

  bme280.cp.dig_H6  = bme280_read_reg(0xE7);
 
  return BME280_RET_OK;
}


/*
 *  TODO: Adjust timer frequency by BME280 sampling speed.
 *  TODO: return APP_ERROR_CHECK values?
 */
BME280_Ret bme280_set_mode(enum BME280_MODE mode)
{
  NRF_LOG_DEBUG("Setting BME mode: %x\r\n", mode);
  if(!bme280.sensor_available) { return BME280_RET_ERROR;  }
  uint8_t conf, reg;
  
  BME280_Ret status = BME280_RET_ERROR;
  reg = bme280_read_reg(BME280REG_CTRL_HUM);
  conf = bme280_read_reg(BME280REG_CTRL_MEAS);
  NRF_LOG_DEBUG("CONFIG before mode: %x\r\n", conf);
  status |= bme280_write_reg(BME280REG_CTRL_HUM, reg);  //HUMIDITY must be written first
  conf = conf & 0b11111100;
  conf |= mode;

  switch(mode)
  {
    case BME280_MODE_NORMAL:
      status |= bme280_write_reg(BME280REG_CTRL_MEAS, conf);
      //conf = bme280_read_reg(BME280REG_CTRL_MEAS);
      //NRF_LOG_DEBUG("Mode: %x\r\n", conf);
      break;

    case BME280_MODE_FORCED:
      status |= bme280_write_reg(BME280REG_CTRL_MEAS, conf); //start new measurement
      break;

    case BME280_MODE_SLEEP:    
      status |= bme280_write_reg(BME280REG_CTRL_MEAS, conf);     
      break;

    default:
      break;
  }

  if(BME280_RET_OK == status) {current_mode = mode;}
  return status;
}

BME280_Ret bme280_set_interval(enum BME280_INTERVAL interval)
{
  if(BME280_MODE_SLEEP != current_mode){ return BME280_RET_ILLEGAL; }
  uint8_t conf;
  BME280_Ret status = BME280_RET_ERROR;

  conf   = bme280_read_reg(BME280REG_CONFIG);
  conf   = conf &~ BME280_INTERVAL_MASK;
  conf  |= interval;      
  status = bme280_write_reg(BME280REG_CONFIG, conf);
  
  if(NRF_SUCCESS == status) { current_interval = interval; }

  return status;
}

enum BME280_INTERVAL bme280_get_interval(void)
{
  return current_interval;
}


int bme280_is_measuring(void)
{
  uint8_t s;

  s = bme280_read_reg(BME280REG_STATUS);
  if (s & 0b00001000)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}


BME280_Ret bme280_set_oversampling_hum(uint8_t os)
{
  if(BME280_MODE_SLEEP != current_mode){ return BME280_RET_ILLEGAL; }
  uint8_t meas;
  meas = bme280_read_reg(BME280REG_CTRL_MEAS);
  bme280_write_reg(BME280REG_CTRL_HUM, os);
  return bme280_write_reg(BME280REG_CTRL_MEAS, meas); //Changes to humi take effect after write to meas
}


BME280_Ret bme280_set_oversampling_temp(uint8_t os)
{
  if(BME280_MODE_SLEEP != current_mode){ return BME280_RET_ILLEGAL; }
  uint8_t humi, meas;
  humi = bme280_read_reg(BME280REG_CTRL_HUM);
  meas = bme280_read_reg(BME280REG_CTRL_MEAS);
  bme280_write_reg(BME280REG_CTRL_HUM, humi);
  meas &= 0b00011111;
  meas |= (os<<5);
  return bme280_write_reg(BME280REG_CTRL_MEAS, meas);
}


BME280_Ret bme280_set_oversampling_press(uint8_t os)
{
  if(BME280_MODE_SLEEP != current_mode){ return BME280_RET_ILLEGAL; }
  uint8_t humi, meas;
  humi = bme280_read_reg(BME280REG_CTRL_HUM);
  meas = bme280_read_reg(BME280REG_CTRL_MEAS);
  bme280_write_reg(BME280REG_CTRL_HUM, humi);
  meas &= 0b11100011;
  meas |= (os<<2);
  return bme280_write_reg(BME280REG_CTRL_MEAS, meas);
}
	
BME280_Ret bme280_set_iir(uint8_t iir)
{
   if(BME280_MODE_SLEEP != current_mode){ return BME280_RET_ILLEGAL; }
   uint8_t conf = bme280_read_reg(BME280REG_CONFIG);
   conf &= ~BME280_IIR_MASK;
   conf |= BME280_IIR_MASK & iir;
   NRF_LOG_DEBUG("Writing %d to %d\r\n", conf, BME280REG_CONFIG);
   return bme280_write_reg(BME280REG_CONFIG, conf);
}

/**
 * @brief Read new raw values.
 */
BME280_Ret bme280_read_measurements()
{

  if(!bme280.sensor_available) { return BME280_RET_ERROR;  }
  uint8_t data[BME280_BURST_READ_LENGTH];
  
  BME280_Ret err_code = bme280_read_burst(BME280REG_PRESS_MSB, BME280_BURST_READ_LENGTH, data);

  bme280.adc_h = data[8] + ((uint32_t)data[7] << 8);

  bme280.adc_t  = (uint32_t) data[6] >> 4;
  bme280.adc_t |= (uint32_t) data[5] << 4;
  bme280.adc_t |= (uint32_t) data[4] << 12;

  bme280.adc_p  = (uint32_t) data[3] >> 4;
  bme280.adc_p |= (uint32_t) data[2] << 4;
  bme280.adc_p |= (uint32_t) data[1] << 12;

  return err_code;
}


static uint32_t compensate_P_int64(int32_t adc_P)
{
	int64_t var1, var2, p;

	var1 = ((int64_t)bme280.t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)bme280.cp.dig_P6;
	var2 = var2 + ((var1*(int64_t)bme280.cp.dig_P5) << 17);
	var2 = var2 + (((int64_t)bme280.cp.dig_P4) << 35);
	var1 = ((var1 * var1 * (int64_t)bme280.cp.dig_P3) >> 8) + ((var1 * (int64_t)bme280.cp.dig_P2) << 12);
	var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bme280.cp.dig_P1) >> 33;
	if (var1 == 0) {
		return 0;
	}

	p = 1048576 - adc_P;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (((int64_t)bme280.cp.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
	var2 = (((int64_t)bme280.cp.dig_P8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((int64_t)bme280.cp.dig_P7) << 4);

	return (uint32_t)p;
}


static uint32_t compensate_H_int32(int32_t adc_H)
{
	int32_t v_x1_u32r;

	v_x1_u32r = (bme280.t_fine - ((int32_t)76800));
	v_x1_u32r = (((((adc_H << 14) - (((int32_t)bme280.cp.dig_H4) << 20) - (((int32_t)bme280.cp.dig_H5) * v_x1_u32r)) +
		       ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)bme280.cp.dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)bme280.cp.dig_H3)) >> 11) +
		       ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)bme280.cp.dig_H2) + 8192) >> 14));

	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)bme280.cp.dig_H1)) >> 4));
	v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
	v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

	return (uint32_t)(v_x1_u32r >> 12);
}


static uint32_t compensate_T_int32(int32_t adc_T)
{
	int32_t var1, var2;

	var1 = ((((adc_T>>3) - ((int32_t)bme280.cp.dig_T1<<1))) * 
               ((int32_t)bme280.cp.dig_T2)) >> 11;
	var2 = (((((adc_T>>4) - ((int32_t)bme280.cp.dig_T1)) *
               ((adc_T>>4) - ((int32_t)bme280.cp.dig_T1))) >> 12) * 
               ((int32_t)bme280.cp.dig_T3)) >> 14;

	bme280.t_fine = var1 + var2;

	uint32_t T = (bme280.t_fine * 5 + 128) >> 8;
	//printf("T=%f\n",T);
  return T; ///100.0f; // to
}


/**
 * Returns temperature in DegC, resolution is 0.01 DegC.
 * Output value of �2134� equals 21.34 DegC.
 */
uint32_t bme280_get_temperature(void)
{
	uint32_t temp = compensate_T_int32(bme280.adc_t);
	return temp;
}


/**
 * Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format
 * (24 integer bits and 8 fractional bits).
 * Output value of �24674867� represents 24674867/256 = 96386.2 Pa = 963.862 hPa
 */
uint32_t bme280_get_pressure(void)
{
	uint32_t press = compensate_P_int64(bme280.adc_p);
	return press;
}


/**
 * Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format
 * (22 integer and 10 fractional bits).
 * Output value of �50532� represents 50532/1024 = 49.356 %RH
 */
uint32_t bme280_get_humidity(void)
{
	uint32_t humi = compensate_H_int32(bme280.adc_h);
	return humi;
}

uint8_t bme280_read_reg(uint8_t reg)
{
	uint8_t tx[2];
	uint8_t rx[2] = {0};

	tx[0] = reg | 0x80;
	tx[1] = 0x00;
	spi_transfer_bme280(tx, 2, rx);

	return rx[1];
}

BME280_Ret bme280_read_burst(uint8_t start, uint8_t length, uint8_t* buffer)
{
  uint8_t tx[BME280_MAX_READ_LENGTH] = {0};

  tx[0] = start | 0x80;

  return spi_transfer_bme280(tx, length, buffer);
}


BME280_Ret bme280_write_reg(uint8_t reg, uint8_t value)
{
	uint8_t tx[2];
	uint8_t rx[2] = {0};

	tx[0] = reg & 0x7F;
	tx[1] = value;
	return (SPI_RET_OK == spi_transfer_bme280(tx, 2, rx)) ? BME280_RET_OK : BME280_RET_ERROR;
}


/**
 * Event Handler that is called by the timer to read the sensor values.
 *
 * @param [in] p_context Timer Context
 */
void timer_bme280_event_handler(void* p_context)
{
    NRF_LOG_DEBUG("BME280 event \r\n");
    bme280_read_measurements(); //read previous data
}