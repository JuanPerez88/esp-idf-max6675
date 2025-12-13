# esp-idf-max6675

Minimal MAX6675 thermocouple driver implemented as a reusable ESP-IDF component.

This component provides a simple C API to read temperature data from MAX6675
thermocouple converters using the ESP-IDF spi_master driver.

---

## Features

- MAX6675 thermocouple support
- ESP-IDF spi_master based implementation
- Read-only SPI device (no MOSI required)
- Plain C interface
- Suitable for reuse in multiple ESP-IDF projects

---

## Usage

Copy this component into your ESP-IDF project:

```text
components/
└─ max6675/
```

Then include it in your application source code:

```text
#include "max6675.h"
```
And add it as a dependency in your main/CMakeLists.txt:

```text
idf_component_register(
    SRCS "main.c"
    REQUIRES max6675
)
```
---

License

MIT License
