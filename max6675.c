// max6675.c
// ESP32 MAX6675 driver using ESP-IDF spi_master.

#include <math.h>
#include <stdio.h>

#include "driver/spi_master.h"
#include "esp_log.h"

#include "max6675.h"

static const char *TAG = "MAX6675";

#define MAX_HOSTS 4

static bool s_bus_inited[MAX_HOSTS] = { false };
static int  s_clock_hz[MAX_HOSTS]   = { 0 };

static inline int host_index(spi_host_device_t host)
{
    return (int)host;
}

esp_err_t max6675_bus_init_host(spi_host_device_t host,
                                int pin_miso,
                                int pin_mosi,
                                int pin_sck,
                                int clock_hz)
{
    int idx = host_index(host);

    if (idx < 0 || idx >= MAX_HOSTS) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_bus_inited[idx]) {
        return ESP_OK;
    }

    spi_bus_config_t buscfg = {
        .mosi_io_num = pin_mosi,
        .miso_io_num = pin_miso,
        .sclk_io_num = pin_sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 16
    };

    esp_err_t ret = spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed (host=%d): %d", host, ret);
        return ret;
    }

    s_clock_hz[idx]   = clock_hz;
    s_bus_inited[idx] = true;

    ESP_LOGI(TAG,
             "SPI bus OK (host=%d, MISO=%d, MOSI=%d, SCK=%d, CLK=%d Hz)",
             host, pin_miso, pin_mosi, pin_sck, clock_hz);

    return ESP_OK;
}

esp_err_t max6675_add_sensor_host(spi_host_device_t host,
                                  int pin_cs,
                                  spi_device_handle_t *out_dev)
{
    int idx = host_index(host);

    if (idx < 0 || idx >= MAX_HOSTS) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_bus_inited[idx]) {
        ESP_LOGE(TAG, "Bus not initialized for host=%d", host);
        return ESP_ERR_INVALID_STATE;
    }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = s_clock_hz[idx],
        .mode = 0,
        .spics_io_num = pin_cs,
        .queue_size = 1,
    };

    spi_device_handle_t dev;
    esp_err_t ret = spi_bus_add_device(host, &devcfg, &dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG,
                 "spi_bus_add_device failed (host=%d, CS=%d): %d",
                 host, pin_cs, ret);
        return ret;
    }

    *out_dev = dev;
    ESP_LOGI(TAG, "MAX6675 sensor added (host=%d, CS=%d)", host, pin_cs);
    return ESP_OK;
}

// Backward-compatible API (SPI2_HOST)

esp_err_t max6675_bus_init(int pin_miso,
                           int pin_mosi,
                           int pin_sck,
                           int clock_hz)
{
    return max6675_bus_init_host(SPI2_HOST,
                                 pin_miso,
                                 pin_mosi,
                                 pin_sck,
                                 clock_hz);
}

esp_err_t max6675_add_sensor(int pin_cs,
                             spi_device_handle_t *out_dev)
{
    return max6675_add_sensor_host(SPI2_HOST, pin_cs, out_dev);
}

float max6675_read_celsius(spi_device_handle_t dev)
{
    if (dev == NULL) {
        ESP_LOGE(TAG, "Device handle is NULL");
        return NAN;
    }

    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_RXDATA,
        .length = 16,
    };

    esp_err_t ret = spi_device_transmit(dev, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_device_transmit failed: %d", ret);
        return NAN;
    }

    uint16_t raw = ((uint16_t)t.rx_data[0] << 8) | t.rx_data[1];

    if (raw & 0x0004) {
        ESP_LOGW(TAG, "Thermocouple not connected");
        return NAN;
    }

    raw >>= 3;
    return raw * 0.25f;
}
