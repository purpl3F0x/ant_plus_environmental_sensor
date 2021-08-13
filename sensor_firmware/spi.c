#include "spi.h"
#include "app_util_platform.h"
#include "boards.h"
#include "nrf_delay.h"
#include "nrf_drv_spi.h"

//#define NRF_LOG_MODULE_NAME "SPI"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define SPI_INSTANCE 0

void spi_event_handler(nrf_drv_spi_evt_t const *p_event, void *p_contex);

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);
volatile bool spi_xfer_done;
static bool initDone = false;

extern void spi_init(void) {
  uint8_t err_code;
  nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
  spi_config.sck_pin = SPI_SCK_PIN;   //pin number 3
  spi_config.miso_pin = SPI_MISO_PIN; //pin number 28(SDO)
  spi_config.mosi_pin = SPI_MOSI_PIN; // pin number 4(SDI)
  spi_config.ss_pin = SPI_SS_PIN;     // slave select pin - 29
  //spi_config.frequency = NRF_DRV_SPI_FREQ_8M;

  /* Init chipselect for BME280 */
  nrf_gpio_pin_dir_set(SPI_SS_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
  nrf_gpio_cfg_output(SPI_SS_PIN);
  nrf_gpio_pin_set(SPI_SS_PIN);

  err_code = nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, NULL);
  APP_ERROR_CHECK(err_code);
  if (err_code) {
  }
  spi_xfer_done = true;
  initDone = true;
}

extern bool spi_isInitialized(void) {
  return initDone;
}

extern SPI_Ret spi_transfer_bme280(uint8_t *const p_toWrite, uint8_t count, uint8_t *const p_toRead) {
  NRF_LOG_DEBUG("Transferring to bme280\n");

  SPI_Ret retVal = SPI_RET_OK;
  if ((NULL == p_toWrite) || (NULL == p_toRead)) {
    retVal = SPI_RET_ERROR;
  }
  if ((true == spi_xfer_done) && (SPI_RET_OK == retVal)) {
    spi_xfer_done = false;

    nrf_gpio_pin_clear(SPI_SS_PIN);
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, p_toWrite, count, p_toRead, count));
    // Locks if run in interrupt context
    while (!spi_xfer_done) {
      //Requires initialized softdevice
      //uint32_t err_code = sd_app_evt_wait();
      __WFE();
      //NRF_LOG_DEBUG("SPI status %d\r\n", err_code);
    }
    nrf_gpio_pin_set(SPI_SS_PIN);
    retVal = SPI_RET_OK;
  } else {
    retVal = SPI_RET_BUSY;
  }
  return retVal;
}

void spi_event_handler(nrf_drv_spi_evt_t const *p_event, void *p_contex) {
  spi_xfer_done = true;
  //nrf_gpio_pin_set(SPI_SS_PIN);
  NRF_LOG_DEBUG("SPI Xfer done\r\n");
}
