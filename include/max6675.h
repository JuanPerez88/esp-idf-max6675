// max6675.h
// ESP32 MAX6675 driver using ESP-IDF spi_master.

#ifndef MAX6675_H
#define MAX6675_H

#include "esp_err.h"
#include "driver/spi_master.h"

esp_err_t max6675_bus_init_host(spi_host_device_t host,
                                int pin_miso,
                                int pin_mosi,
                                int pin_sck,
                                int clock_hz);

esp_err_t max6675_add_sensor_host(spi_host_device_t host,
                                  int pin_cs,
                                  spi_device_handle_t *out_dev);

float max6675_read_celsius(spi_device_handle_t dev);

// Backward-compatible API (defaults to SPI2_HOST)
esp_err_t max6675_bus_init(int pin_miso,
                           int pin_mosi,
                           int pin_sck,
                           int clock_hz);

esp_err_t max6675_add_sensor(int pin_cs,
                             spi_device_handle_t *out_dev);

#endif // MAX6675_H
