// max6675.c
// ESP32 MAX6675 driver using ESP-IDF spi_master (up to 3 sensors per SPI host).

#include <math.h>
#include <stdio.h>

#include "driver/spi_master.h"
#include "esp_log.h"

#include "max6675.h"

#define MAX6675_SPI_HOST  SPI2_HOST      
static const char *TAG = "MAX6675";

static bool s_bus_inited = false;
static int  s_clock_hz   = 1000000;     

esp_err_t max6675_bus_init(int pin_miso,
                           int pin_mosi,
                           int pin_sck,
                           int clock_hz)
{
    if (s_bus_inited) {
        return ESP_OK;
    }

    esp_err_t ret;

    spi_bus_config_t buscfg = {
        .mosi_io_num = pin_mosi,
        .miso_io_num = pin_miso,
        .sclk_io_num = pin_sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 16
    };

    ret = spi_bus_initialize(MAX6675_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %d", ret);
        return ret;
    }

    s_clock_hz   = clock_hz;
    s_bus_inited = true;

    ESP_LOGI(TAG,
             "SPI bus OK (MISO=%d, MOSI=%d, SCK=%d, CLK=%d Hz)",
             pin_miso, pin_mosi, pin_sck, clock_hz);

    return ESP_OK;
}

esp_err_t max6675_add_sensor(int pin_cs,
                             spi_device_handle_t *out_dev)
{
    if (!s_bus_inited) {
        ESP_LOGE(TAG, "Bus not initialized. Call max6675_bus_init() first.");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret;

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = s_clock_hz,  
        .mode = 0,                      
        .spics_io_num = pin_cs,
        .queue_size = 1,
        .flags = 0,
    };

    spi_device_handle_t dev;
    ret = spi_bus_add_device(MAX6675_SPI_HOST, &devcfg, &dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed for CS=%d: %d", pin_cs, ret);
        return ret;
    }

    *out_dev = dev;
    ESP_LOGI(TAG, "MAX6675 sensor added on CS=%d", pin_cs);
    return ESP_OK;
}

float max6675_read_celsius(spi_device_handle_t dev)
{
    if (dev == NULL) {
        ESP_LOGE(TAG, "Device handle is NULL");
        return NAN;
    }

    esp_err_t ret;
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_RXDATA,
        .length = 16,  
    };

    ret = spi_device_transmit(dev, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_device_transmit failed: %d", ret);
        return NAN;
    }

    uint16_t raw = ((uint16_t)t.rx_data[0] << 8) | t.rx_data[1];

    if (raw & 0x0004) {
        ESP_LOGW(TAG, "Thermocouple not connected (D2 bit set)");
        return NAN;
    }

    raw >>= 3;
    float temp_c = raw * 0.25f;
    return temp_c;
}
