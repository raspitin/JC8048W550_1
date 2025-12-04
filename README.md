# 🌡️ Smart Thermostat ESP32-S3 (JC8048W550)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Build-orange.svg)](https://platformio.org/)
[![LVGL](https://img.shields.io/badge/LVGL-v9.1-blue.svg)](https://lvgl.io/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

An advanced smart thermostat based on **ESP32-S3** and **JC8048W550** capacitive touch display (800x480). It features a modern interface built with LVGL, remote control via WiFi relay (ESP-01S), "drag & drop" touch weekly scheduling, weather forecasts, and smart boost logic.

![Screenshot Home](assets/images/PXL_20231130_225143662.jpg) *(Replace with a real screenshot of your display)*

## ✨ Key Features

* **Graphical User Interface (GUI):** Built on LVGL 9, fluid and responsive.
* **Weekly Scheduling:** Innovative *Drag & Drop* interface to select heating time slots (30-min steps) directly from the touch screen.
* **Timed Boost:** Quick button for forced heating with timer (30, 60, 90... min) and visual countdown.
* **Integrated Weather:** Current temperature and weather icon display (OpenWeatherMap API).
* **Remote & Local Relay:**
    * Support for a relay connected directly to the ESP32.
    * Support for a remote **Slave Relay (ESP-01S)** via WiFi with Rolling Token authentication and Heartbeat for maximum security.
* **Reliability:**
    * Watchdog Timer to prevent freezes.
    * Warning popup in case of connection loss with the relay.
    * Automatic WiFi recovery.
* **Time Synchronization:** Automatic NTP.
* **Easy Configuration:** Captive Portal (WiFiManager) to set WiFi credentials, Weather API Key, and Timezone without recompiling.

## 🛠️ Hardware Required

1.  **Master Unit:**
    * **Guition JC8048W550** Board (ESP32-S3 + 4.3"/5.0" IPS 800x480 Capacitive Touch).
    * Temperature sensor (e.g., DHT22, DS18B20 - *to be implemented in the `readLocalSensor` function*).
2.  **Slave Unit (Optional):**
    * **ESP-01S** Module + Relay Module v1.0.

## 🚀 Installation and Configuration (Wiki)

### Software Prerequisites

* [Visual Studio Code (VSCode)](https://code.visualstudio.com/)
* **PlatformIO IDE** extension for VSCode.

### 1. Clone the Repository

```bash
git clone [https://github.com/raspitin/JC8048W550_1.git](https://github.com/raspitin/JC8048W550_1.git)
cd REPO_NAME
```

### 2. Project Configuration (Master)

The project is already configured for the JC8048W550 board.

1.  Open the project folder in VSCode.
2.  Wait for PlatformIO to download the dependencies.
3.  Edit the `include/config.h` file to adapt it to your needs (if you want to change pins or default IPs):

```cpp
// Example configuration in include/config.h
#define RELAY_PIN           4               // Local relay pin
#define REMOTE_RELAY_IP     "192.168.1.33"  // Static IP of the remote relay
#define RELAY_AUTH_TOKEN    "YOUR_PASSWORD" // Must match the Slave's token
```

### 3. Slave Configuration (Remote Relay)

If you use the remote ESP-01S relay:

1.  Open the `ESP-01S-SlaveRelay` folder in a new VSCode window.
2.  Edit `src/main.cpp` to set the desired static IP (must match `REMOTE_RELAY_IP` in the Master).
3.  Upload the firmware to the ESP-01S (requires a USB-TTL adapter).

### 4. Compilation and Upload (Master)

1.  Connect the JC8048W550 board via USB (holding the BOOT button if necessary).
2.  In PlatformIO, click on **Build** (checkmark icon) to verify.
3.  Click on **Upload** (right arrow icon) to flash the firmware.
4.  *Note:* The first compilation might take some time to download LVGL and the ESP32 framework.

### 5. First Boot

1.  On the first boot, if no WiFi networks are found, the display will create an Access Point named **`Termostato_Setup`**.
2.  Connect to this network with your phone.
3.  A web page (Captive Portal) will open. Enter:
    * SSID and Password of your home WiFi.
    * OpenWeatherMap API Key.
    * City and Country.
4.  Save, and the device will restart and connect to the network.

## 📁 Code Structure

* `src/main.cpp`: Main loop, WiFi management, Watchdog, and Heartbeat.
* `src/ui.cpp`: All LVGL GUI logic (Widgets, Touch Events, Timeline).
* `src/thermostat.cpp`: Core thermostat logic (hysteresis, boost, relay communication).
* `src/config_manager.cpp`: Data saving management on LittleFS (JSON).
* `lib/esp32-smartdisplay`: Optimized drivers for the ST7262 display (Parallel RGB).

## 🔧 Common Troubleshooting

* **Black Screen / Artifacts:** Check the flags in `platformio.ini`. The current stable configuration uses `PCLK=16MHz` and `PCLK_ACTIVE_NEG=1`.
* **LVGL Compilation Error:** Ensure you have copied `lv_conf.h` to the correct folder (already included in the repo).
* **Relay Not Responding:** Verify that the slave IP is static and correct in `config.h`, and that the Tokens in `secrets.h` match.

## 🤝 Contributing

Pull Requests are welcome! For major changes, please open an Issue first to discuss what you would like to change.

## 📄 License

This project is distributed under the **MIT** license. See the `LICENSE` file for details.

---
*Developed with ❤️ using ESP32 and LVGL.*
