#include "max6675.h"

#include <math.h>
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "MAX6675";

#define MAX_HOSTS 4

static bool s_bus_inited[MAX_HOSTS] = { false };
static int  s_clock_hz[MAX_HOSTS]   = { 0 };

static inline int host_index(spi_host_device_t host)
{
    return (int)host;
}

/* =========================
 * SPI BUS
 * ========================= */
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
        .max_transfer_sz = 2
    };

    esp_err_t ret = spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %d", ret);
        return ret;
    }

    s_bus_inited[idx] = true;
    s_clock_hz[idx] = clock_hz;
    return ESP_OK;
}

esp_err_t max6675_add_sensor_host(spi_host_device_t host,
                                  int pin_cs,
                                  spi_device_handle_t *out_dev)
{
    if (!out_dev) {
        return ESP_ERR_INVALID_ARG;
    }

    int idx = host_index(host);
    if (idx < 0 || idx >= MAX_HOSTS || !s_bus_inited[idx]) {
        return ESP_ERR_INVALID_STATE;
    }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = s_clock_hz[idx],
        .mode = 0,
        .spics_io_num = pin_cs,
        .queue_size = 1,
    };

    return spi_bus_add_device(host, &devcfg, out_dev);
}

/* =========================
 * INTERNAL RAW READ
 * ========================= */
static esp_err_t read_raw(spi_device_handle_t dev,
                          uint16_t *raw,
                          max6675_status_t *status)
{
    if (!dev || !raw) {
        if (status) *status = MAX6675_STATUS_INVALID_ARG;
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_RXDATA,
        .length = 16,
    };

    esp_err_t ret = spi_device_transmit(dev, &t);
    if (ret != ESP_OK) {
        if (status) *status = MAX6675_STATUS_SPI_ERROR;
        return ret;
    }

    *raw = ((uint16_t)t.rx_data[0] << 8) | t.rx_data[1];

    /* Basic floating-line detection */
    if (*raw == 0x0000 || *raw == 0xFFFF) {
        if (status) *status = MAX6675_STATUS_SPI_ERROR;
        return ESP_OK;
    }

    /* Bits 2..0 must be zero on MAX6675 */
    if ((*raw & 0x0007) != 0x0000) {
        if (status) *status = MAX6675_STATUS_SPI_ERROR;
        return ESP_OK;
    }

    /* Open thermocouple flag */
    if (*raw & 0x0004) {
        if (status) *status = MAX6675_STATUS_OPEN_THERMOCOUPLE;
        return ESP_OK;
    }

    /* Decode temperature */
    uint16_t temp_raw = *raw >> 3;
    float temp_c = temp_raw * 0.25f;

    /* Sanity temperature range check */
    if (temp_c < -20.0f || temp_c > 1200.0f) {
        if (status) *status = MAX6675_STATUS_SPI_ERROR;
        return ESP_OK;
    }

    if (status) *status = MAX6675_STATUS_OK;
    return ESP_OK;
   
}

/* =========================
 * SENSOR API
 * ========================= */
esp_err_t max6675_sensor_init(max6675_sensor_t *sensor,
                              spi_device_handle_t dev,
                              int min_interval_ms,
                              max6675_rate_limit_mode_t rl_mode)
{
    if (!sensor || !dev) {
        return ESP_ERR_INVALID_ARG;
    }

    sensor->dev = dev;
    sensor->min_interval_ms = min_interval_ms;
    sensor->rl_mode = rl_mode;
    sensor->valid = false;
    sensor->last_update_us = 0;
    return ESP_OK;
}

esp_err_t max6675_sensor_read(max6675_sensor_t *sensor,
                              float *celsius,
                              max6675_status_t *status,
                              bool *is_fresh)
{
    if (!sensor || !celsius) {
        return ESP_ERR_INVALID_ARG;
    }

    int64_t now = esp_timer_get_time();

    if (sensor->valid && sensor->min_interval_ms > 0) {
        int64_t elapsed_ms = (now - sensor->last_update_us) / 1000;
        if (elapsed_ms < sensor->min_interval_ms) {
            if (sensor->rl_mode == MAX6675_RL_ENFORCE_MIN_INTERVAL) {
                return ESP_ERR_INVALID_STATE;
            }

            *celsius = sensor->last_celsius;
            if (status) *status = MAX6675_STATUS_OK;
            if (is_fresh) *is_fresh = false;
            return ESP_OK;
        }
    }

    uint16_t raw;
    max6675_status_t st;

    esp_err_t ret = read_raw(sensor->dev, &raw, &st);
    if (ret != ESP_OK) {
        if (status) *status = st;
        return ret;
    }

    if (st == MAX6675_STATUS_OPEN_THERMOCOUPLE) {
        sensor->valid = false;
        *celsius = NAN;
    } else {
        sensor->last_raw = raw;
        sensor->last_celsius = (float)(raw >> 3) * 0.25f;
        sensor->last_update_us = now;
        sensor->valid = true;
        *celsius = sensor->last_celsius;
    }

    if (status) *status = st;
    if (is_fresh) *is_fresh = true;

    return ESP_OK;
}
