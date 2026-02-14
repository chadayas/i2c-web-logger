# ESP32 Breathalyzer — Real-Time BAC Monitor

A real-time blood alcohol content (BAC) estimation tool built on the ESP32 microcontroller. The device reads analog voltage from an MQ-3 ethanol gas sensor, converts it to a BAC estimate, and streams the data over WebSocket to a browser-based dashboard served directly from the ESP32's flash storage.

## Demo

![Breathalyzer demo](demo_esp2.gif)


## Hardware

| Component | Model | Notes |
|-----------|-------|-------|
| Microcontroller | **ESP32 DevKit C** (ESP-WROOM-32) | Dual-core Xtensa LX6, 4MB flash, 520KB SRAM |
| Gas Sensor | **MQ-3** (Alcohol/Ethanol) | Analog output on AOUT pin, heater requires 5V |

### Wiring

| MQ-3 Pin | ESP32 Pin | Description |
|----------|-----------|-------------|
| VCC | 5V (VIN) | Sensor + heater power |
| GND | GND | Common ground |
| AOUT | GPIO36 (VP) | Analog signal to ADC1 Channel 0 |
| DOUT | — | Not connected (digital threshold not used) |

GPIO36 is one of the input-only ADC1 pins on the ESP32 DevKit C. It does not have an internal pull-up/pull-down, which is ideal for clean analog reads from the sensor.

---

## Software Stack

- **ESP-IDF v5.5.1** — Espressif's official IoT Development Framework (C/C++)
- **FreeRTOS** — Real-time operating system bundled with ESP-IDF, used for task scheduling and event synchronization
- **esp_http_server** — Lightweight HTTP/WebSocket server running on the MCU
- **SPIFFS** — SPI Flash File System for serving static web assets (HTML, CSS, JS) from flash

### FreeRTOS Usage

- **Event Groups** (`xEventGroupCreate`, `xEventGroupWaitBits`, `xEventGroupSetBits`) — used to synchronize WiFi connection state between the async event callbacks and the main task. The main task blocks on `WIFI_CONNECTED_BIT | WIFI_FAIL_BIT` until the WiFi driver reports a result.
- **`vTaskDelay`** — keeps the main task alive in an idle loop after initialization, preventing `app_main` from returning and triggering destructor cleanup of the WiFi and HTTP server objects.
- **`httpd_queue_work`** — schedules WebSocket frame sends from the server context to avoid threading issues with the HTTP server's internal task.

### Architecture

```
app_main
  |
  +-- WifiService (constructor)
  |     +-- NVS init
  |     +-- netif / event loop init
  |     +-- WiFi driver init + event handler registration
  |     +-- Connect (blocks until connected or failed)
  |
  +-- SPIFFS mount
  |
  +-- ADC1 oneshot init + calibration (line fitting)
  |
  +-- Httpserver (constructor)
  |     +-- Register routes: /, /style.css, /graph.js, /ws
  |
  +-- Idle loop (vTaskDelay)
```

The WiFi event callbacks (`wifi_event_cb`, `ip_event_cb`) run asynchronously on the default event loop task. The HTTP server runs on its own internal task. `app_main` only needs to stay alive to prevent stack-allocated objects from being destroyed.

---

## ADC Configuration

| Parameter | Value | Reason |
|-----------|-------|--------|
| Mode | Oneshot | Single read per WebSocket request, no continuous DMA needed |
| Unit | ADC1 | ADC2 is unavailable when WiFi is active on ESP32 |
| Channel | Channel 0 (GPIO36) | Input-only pin, no conflict with other peripherals |
| Attenuation | 12 dB | Full 0-3.3V input range to capture MQ-3's full voltage swing |
| Bit Width | Default (12-bit) | 4096 levels across 0-3.3V, ~0.8mV resolution |
| Calibration | Line Fitting | ESP32 DevKit C does not support curve fitting (no eFuse calibration data); line fitting is the fallback |

### BAC Conversion (Current — Placeholder)

```
voltage = adc_raw * (3.3 / 4095.0)
bac = voltage / 33.0
```

This is a **rough linear approximation** and is **not accurate** for real BAC measurement. The MQ-3 sensor's resistance-to-concentration relationship is nonlinear (logarithmic per the datasheet sensitivity curve). Proper calibration requires:

1. **Sensor warm-up** — the MQ-3 heater needs 24-48 hours of initial burn-in on first use, and several minutes of warm-up on each subsequent power-on, before readings stabilize.
2. **Clean air baseline (R0)** — record the sensor's resistance in clean air to establish the reference resistance.
3. **Rs/R0 curve mapping** — use the MQ-3 datasheet's sensitivity characteristic curve to map the resistance ratio to ethanol concentration (mg/L).
4. **Unit conversion** — convert ethanol gas concentration (mg/L of air) to blood alcohol content (g/dL).

Until this calibration is implemented, the BAC values displayed on the dashboard are **relative estimates only** and should not be used for any real measurement.

---

## Flash Partition Layout

```
# Name,    Type, SubType, Offset,    Size
nvs,       data, nvs,     0x9000,    0x6000    (24 KB)
phy_init,  data, phy,     0xf000,    0x1000    (4 KB)
factory,   app,  factory, 0x10000,   0x1C0000  (~1.75 MB)
storage,   data, spiffs,  (auto),    128K
```

Total flash usage: ~1.9 MB of 4 MB. The factory partition was expanded from the default 1 MB to ~1.75 MB to accommodate the application binary (WiFi + HTTP server + ADC drivers). SPIFFS storage is set to 128 KB — the static web assets (HTML, CSS, JS) total under 8 KB.

---

## Web Dashboard

The ESP32 serves a single-page dashboard over HTTP with real-time data streaming over WebSocket.

**Routes:**

| URI | Method | Handler | Description |
|-----|--------|---------|-------------|
| `/` | GET | `Handlers::root` | Serves `index.html` from SPIFFS |
| `/style.css` | GET | `Handlers::css` | Serves stylesheet from SPIFFS |
| `/graph.js` | GET | `Handlers::js` | Serves graph script from SPIFFS |
| `/ws` | GET (WebSocket) | `Handlers::websock` | Streams sensor data as JSON |

**WebSocket payload:**
```json
{"bac": 0.0012, "raw": 156, "mv": 126}
```

The client sends `"read"` every 500ms. The server reads the ADC, computes BAC, and responds with the JSON payload via `httpd_queue_work` + `httpd_ws_send_frame_async` for thread-safe async delivery.

**Frontend features:**
- Real-time scrolling line graph (Canvas 2D) with gradient fill
- Live BAC, raw ADC, and millivolt readouts
- Peak BAC tracking with reset button
- Pause/resume data stream
- Connection status indicator

---

## Build & Flash

**Prerequisites:** ESP-IDF v5.5.1 installed and sourced (`source $IDF_PATH/export.sh`)

```bash
# Configure WiFi credentials (one-time)
idf.py menuconfig
# Navigate to: Component config → Project Configuration
# Set WIFI_NAME and WIFI_PW

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

The SPIFFS image is built automatically during the build step via `spiffs_create_partition_image` in the root `CMakeLists.txt`.

---

## Project Structure

```
.
├── CMakeLists.txt            # Root build config + SPIFFS image generation
├── partitions.csv            # Custom flash partition table
├── main/
│   ├── CMakeLists.txt        # Component dependencies
│   ├── main.cpp              # All application code
│   └── libs.h                # Includes, macros, class/namespace declarations
└── spiffs/
    ├── index.html            # Dashboard page
    ├── style.css             # Dark theme styling
    └── graph.js              # Canvas graph + WebSocket client
```
