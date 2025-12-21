# esp-idf-max6675

Reusable MAX6675 thermocouple driver implemented as an ESP-IDF component.

This repository provides a small, plain-C driver to read MAX6675 thermocouple
converters using the ESP-IDF `spi_master` API.

---

## Overview

This repository is **not a standalone application**.

It is intended to be included as a component in ESP-IDF projects that need to
read temperature data from MAX6675 modules.

Note: Proper hardware termination of the SPI MISO line is required for
reliable fault detection (see Hardware requirements).

---

## Features

- MAX6675 thermocouple support
- ESP-IDF `spi_master` based implementation
- Read-only SPI device (no MOSI required)
- Multi-SPI-host support (SPI2 / SPI3)
- Backward-compatible API
- Plain C interface
- Error detection for floating or disconnected devices

---

## Sensor status reference

Each MAX6675 read operation returns a temperature value and a status code.

| Status | Meaning | Typical console output | Notes |
|------|--------|------------------------|------|
| `MAX6675_STATUS_OK` | Valid temperature reading | `21.50 C` | Thermocouple connected and SPI communication OK |
| `MAX6675_STATUS_OPEN_THERMOCOUPLE` | Thermocouple disconnected | `TC OPEN` | One or both thermocouple wires not connected |
| `MAX6675_STATUS_SPI_ERROR` | SPI communication error | `SPI ERR` | Missing module, floating MISO, wiring or power issue |
| `MAX6675_STATUS_INVALID_ARG` | Invalid API usage | `ERR` | Programming error (should not happen in normal use) |

---
## Hardware requirements (important)

When using this driver with one or more MAX6675 modules on a shared SPI bus,
a pull-down resistor on the SPI MISO line is strongly recommended.

### Recommended configuration

- One **10 kΩ pull-down resistor**
- Between **SPI MISO and GND**
- **One resistor per SPI bus**

### Why this matters

The MAX6675 tristates its MISO output when not selected or unpowered.
Without a pull-down resistor, the MISO line may float, resulting in:

- Invalid but plausible temperature values
- `0.00` readings when the sensor is missing
- Unreliable detection of disconnected modules

This driver includes software-level sanity checks, but a pull-down resistor
is required to guarantee stable and deterministic SPI behavior.

---

## Project structure

- `max6675.c`  
  Driver implementation

- `include/max6675.h`  
  Public API

---

## Getting started

### 1. Prerequisites

- ESP-IDF installed and working

### 2. Add the component to your project

Using git submodules (recommended):

```bash
git submodule add https://github.com/JuanPerez88/esp-idf-max6675 components/max6675
```

Or clone with submodules:

```bash
git clone --recurse-submodules <your-project-repo>
```

### 3. Register the component

In `main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "main.c"
    REQUIRES max6675
)
```

### 4. Use the API

```bash
#include "max6675.h"
```

---

## License

MIT License
