# esp-idf-max6675

Reusable MAX6675 thermocouple driver implemented as an ESP-IDF component.

This repository provides a small, plain-C driver to read MAX6675 thermocouple
converters using the ESP-IDF `spi_master` API.

---

## Overview

This repository is **not a standalone application**.

It is intended to be included as a component in ESP-IDF projects that need to
read temperature data from MAX6675 modules.

---

## Features

- MAX6675 thermocouple support
- ESP-IDF `spi_master` based implementation
- Read-only SPI device (no MOSI required)
- Multi-SPI-host support (SPI2 / SPI3)
- Backward-compatible API
- Plain C interface

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
