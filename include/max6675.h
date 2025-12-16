#ifndef MAX6675_H
#define MAX6675_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "driver/spi_master.h"

/* =========================
 * Status returned by sensor
 * ========================= */
typedef enum {
    MAX6675_STATUS_OK = 0,
    MAX6675_STATUS_INVALID_ARG,
    MAX6675_STATUS_SPI_ERROR,
    MAX6675_STATUS_OPEN_THERMOCOUPLE,
} max6675_status_t;

/* =========================
 * Rate limit behavior
 * ========================= */
typedef enum {
    MAX6675_RL_RETURN_CACHED = 0,
    MAX6675_RL_ENFORCE_MIN_INTERVAL,
} max6675_rate_limit_mode_t;

/* =========================
 * Sensor object
 * ========================= */
typedef struct {
    spi_device_handle_t dev;

    int min_interval_ms;
    max6675_rate_limit_mode_t rl_mode;

    float last_celsius;
    uint16_t last_raw;
    int64_t last_update_us;
    bool valid;
} max6675_sensor_t;

/* =========================
 * SPI bus / device
 * ========================= */
esp_err_t max6675_bus_init_host(spi_host_device_t host,
                                int pin_miso,
                                int pin_mosi,
                                int pin_sck,
                                int clock_hz);

esp_err_t max6675_add_sensor_host(spi_host_device_t host,
                                  int pin_cs,
                                  spi_device_handle_t *out_dev);

/* =========================
 * Sensor API (ONLY API)
 * ========================= */
esp_err_t max6675_sensor_init(max6675_sensor_t *sensor,
                              spi_device_handle_t dev,
                              int min_interval_ms,
                              max6675_rate_limit_mode_t rl_mode);

esp_err_t max6675_sensor_read(max6675_sensor_t *sensor,
                              float *celsius,
                              max6675_status_t *status,
                              bool *is_fresh);

#endif // MAX6675_H
