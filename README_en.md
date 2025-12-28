<div align="center">

# Hi3861 WiFi-IoT Smart Device Project

<!-- Language Switch Buttons -->
<div>
  <a href="README.md" style="background-color: #6c757d; color: white; border: none; padding: 8px 16px; margin: 5px; border-radius: 4px; cursor: pointer; text-decoration: none;">中文</a>
  <a href="README_en.md" style="background-color: #007bff; color: white; border: none; padding: 8px 16px; margin: 5px; border-radius: 4px; cursor: pointer; text-decoration: none;">English</a>
</div>

</div>

## 📖 Project Introduction

This project is an OpenHarmony IoT application based on Huawei Hi3861 WiFi chip, implementing a complete smart device system. The project integrates multiple sensors, actuators, and communication modules, demonstrating the full functionality of IoT devices.

## 🚀 Main Features

### 🔄 Latest Updates
- ✅ **Advanced Key Manager Module** - Supports dynamic multi-key management and various event detection
- ✅ **Standard KV Storage System** - Upgraded from custom NV module, solving compatibility issues
- ✅ **Smart WiFi Connection** - Supports AP/STA dual mode, Web interface configuration
- ✅ **Chinese Web Interface** - Fixed character encoding issues, supports normal Chinese display

### 🎯 Core Modules
- **Buzzer Module** - PWM control, alarm and prompt sound functions
- **WiFi Network Module** - Dual-mode WiFi connection, smart configuration
- **DHT11 Sensor Module** - Temperature and humidity data acquisition
- **Time Management Module** - Precise delay control
- **KV Storage Module** - Flash data persistence
- **PHP API Module** - HTTP server and Web interface
- **Key Manager Module** - Advanced key event detection

## 🛠️ Technical Architecture

### Hardware Requirements
- Puzhong Hi3861 development board
- DHT11 temperature and humidity sensor
- Buzzer module
- LED indicator
- Key switches (GPIO11, GPIO12)
- WiFi antenna

### Software Architecture

## Main Project Directories
- **[Main Program Entry](src/applications/sample/wifi-iot/app/startup/README.md)** - Project startup module, responsible for system initialization and task scheduling

## Main Functional Modules

- **[Buzzer Module](src/applications/sample/wifi-iot/app/startup/src/buzzer/README.md)** - PWM-controlled buzzer with alarm and prompt sound functions
- **[WiFi Network Module](src/applications/sample/wifi-iot/app/startup/src/network/README.md)** - Dual-mode WiFi connection supporting AP and STA modes
- **[DHT11 Sensor Module](src/applications/sample/wifi-iot/app/startup/src/sensors/README.md)** - Temperature and humidity sensor data acquisition
- **[Time Management Module](src/applications/sample/wifi-iot/app/startup/src/time/README.md)** - System time management and precise delay control
- **[KV Storage Module](src/applications/sample/wifi-iot/app/startup/src/kv/README.md)** - Key-value storage based on LiteOS KV storage system and Flash data persistence
- **[PHP API Module](src/applications/sample/wifi-iot/app/startup/src/php_api/README.md)** - HTTP server and Web interface supporting smart WiFi connection
- **[Key Manager Module](src/applications/sample/wifi-iot/app/startup/src/key_manager/README.md)** - Advanced key event detection supporting short press, long press, double click, etc.
- **[Startup Module](src/applications/sample/wifi-iot/app/startup/README.md)** - System initialization and task scheduling
- **[OLED Display Module](src/applications/sample/wifi-iot/app/startup/src/oled/)** - Graphical information display (to be integrated)
- **[Font Library Module](src/applications/sample/wifi-iot/app/startup/src/fonts/)** - Chinese font support (to be integrated)

## Technical Update Log

### 🔄 Key Manager Integration (Latest)
- ✅ **Added advanced key manager module**
- ✅ Supports dynamic multi-key management with configurable debounce time, long press time, double click interval
- ✅ Supports various key events: press, release, short press, long press, double click, hold press
- ✅ Integrated into main program timer, 5ms cycle key state detection
- ✅ Key1: short press toggles LED, long press shows system status, double click flashes LED
- ✅ Key2: short press buzzer prompt, long press triggers alarm, double click shows key status

### 🔄 Storage System Upgrade
- ✅ **Upgraded from custom NV module to standard KV storage system**
- ✅ Uses Hi3861 standard LiteOS KV storage system, solving compatibility issues
- ✅ Encapsulates UtilsSetValue/UtilsGetValue and other underlying APIs as high-level interfaces
- ✅ Supports power-off save verification and automatic configuration initialization
- ✅ Fixed 0x80003016 error code issue

### Character Encoding Fix
- ✅ Fixed webpage Chinese garbled characters issue
- ✅ Added UTF-8 charset declaration to HTTP response headers
- ✅ Added meta charset tag to HTML pages
- ✅ Supports normal Chinese Web interface display

### Smart WiFi Connection
- ✅ Automatically reads KV storage configuration to connect WiFi on startup
- ✅ AP mode fallback mechanism
- ✅ Configurable timeout settings
- ✅ Web interface remote configuration

## Hardware Requirements

- Puzhong Hi3861 development board
- DHT11 temperature and humidity sensor
- Buzzer module
- LED indicator
- Key switches (GPIO11, GPIO12)
- WiFi antenna

## Quick Start

### 1. Environment Setup
```bash
# Install OpenHarmony development environment
# Configure Hi3861 compilation toolchain
```

### 2. Compile Project
```bash
# Execute in project root directory
hb build -f
```

### 3. Program Burning
```bash
# Use HiBurn tool to burn compiled firmware
```

### 4. Running Effects
- LED starts blinking after system startup
- Buzzer emits startup prompt sound
- KV storage system initializes and performs power-off save verification
- DHT11 sensor starts collecting temperature and humidity data
- WiFi module initializes and prepares for connection
- Key manager initializes, supports GPIO11 and GPIO12 key detection
- HTTP server starts, accessible via browser for device configuration interface

### 5. Key Functions
- **Key1 Short Press**: Toggle LED state
- **Key1 Long Press**: Show system status (WiFi, temperature, humidity, etc.)
- **Key1 Double Click**: LED flashes 3 times
- **Key2 Short Press**: Buzzer prompt sound
- **Key2 Long Press**: Buzzer alarm
- **Key2 Double Click**: Show key status information

### 6. Web Access
- Open browser and visit `http://192.168.0.1/`
- View device status and configure WiFi network
- Chinese interface displays normally without garbled characters

## Key Manager Features

### Core Functions
- **Dynamic Management**: Supports runtime add/remove keys
- **Multiple Events**: Supports complex event detection like short press, long press, double click
- **Configurable Parameters**: Debounce time, long press time, double click interval are configurable
- **Status Query**: Real-time key status and press duration query

### API Interface
```c
// Create key manager
key_manager_t *key_mgr = key_manager_create(2);

// Add key configuration
key_manager_add_key(key_mgr, HI_IO_NAME_GPIO_11, 0, 20, 1000, 300);

// Regular status update
key_manager_update(key_mgr);

// Get key event
key_event_t event = key_manager_get_event(key_mgr, 0);
```

### Timer Integration
Key manager is integrated into the main program's 5ms timer, no need to manually call update function, automatically detects key events.

## KV Storage System Features

### Core Functions
- **High-level Encapsulation**: Encapsulates LiteOS KV storage APIs as easy-to-use interfaces
- **Power-off Save**: Supports Flash data persistence, data not lost after power-off
- **Automatic Verification**: Automatically detects data integrity on startup
- **Configuration Management**: Supports WiFi configuration, device parameters and other critical data storage

### API Interface
```c
// Initialize KV storage system
kv_result_t kv_init(void);

// String operations
kv_result_t kv_set_string(const char *key, const char *value);
kv_result_t kv_get_string(const char *key, char *buffer, size_t buffer_size);

// Integer operations  
kv_result_t kv_set_uint32(const char *key, uint32_t value);
kv_result_t kv_get_uint32(const char *key, uint32_t *value);

// Delete operations
kv_result_t kv_delete(const char *key);
```

### Power-off Save Verification
System automatically performs KV storage power-off save verification on startup:
- First run: Set initial test data
- Subsequent runs: Verify data successfully saved
- Display startup count and critical data status

## Project Structure

```

src/applications/sample/wifi-iot/app/startup/ 
├── src/ # Module source code directory 
│ ├── buzzer/ # Buzzer module 
│ ├── network/ # WiFi network module 
│ ├── sensors/ # Sensor module 
│ ├── time/ # Time management module 
│ ├── kv/ # KV storage module 
│ ├── nfc/ # NFC module 
│ ├── php_api/ # PHP API module 
│ ├── key_manager/ # Key manager module 
│ ├── oled/ # OLED display module (to be integrated)
│ └── fonts/ # Font library module (to be integrated) 
├── main.c # Main program entry 
├── debug.h # Debugging header file
└── BUILD.gn # Build configuration file

```