# WiFi Monitor ESP32

A portable, real-time WiFi network scanner with graphical visualization built on ESP32 and ILI9341 TFT display. This is a mini Wireshark-style network monitoring tool that lets you see all nearby WiFi networks, their signal strength, channels, and security status at a glance.

## Features

- **Real-time Network Scanning**: Automatically scans for WiFi networks every 3 seconds
- **Multiple Display Modes**:
  - **Network List Mode**: View all networks with SSID, signal strength, channel, and security status
  - **Signal Graph Mode**: Historical line graph showing signal strength trends over time for up to 5 networks
  - **Channel Analyzer**: Bar chart visualization of network distribution across WiFi channels with interference recommendations
- **Signal Strength Visualization**: Color-coded signal bars and RSSI values (green = strong, red = weak)
- **Security Indicator**: Shows whether networks are open or secured (WPA/WPA2/WPA3)
- **Hidden Network Support**: Detects and displays hidden SSIDs
- **Recommended Channel Display**: AI-based channel recommendation based on current interference levels

## Hardware Requirements

### Components
- **ESP32 DevKit V1** (or any ESP32 board with SPI pins)
- **ILI9341 240×320 TFT Display** (SPI interface)
- **Micro-USB cable** for power/programming
- **Jumper wires** (for connections)

### Pin Connections (SPI)
```
ILI9341 Pin    →    ESP32 Pin
CS             →    GPIO 15
DC             →    GPIO 2
RST            →    GPIO 4
SDA (MOSI)     →    GPIO 23
SCL (CLK)      →    GPIO 18
VCC            →    3.3V
GND            →    GND
```

**Note**: These pins are configured in the TFT_eSPI library. You may need to customize them in the User_Setup.h file if using different pins.

### Power
- USB power (5V) is typically sufficient for the ESP32 + display setup
- For portable use, consider a 2000mAh+ power bank

## Software Setup

### Prerequisites
- Arduino IDE 1.8.19+ or PlatformIO
- ESP32 Board Support Package installed
- Required Libraries:
  - `WiFi.h` (built-in with ESP32 core)
  - `TFT_eSPI` by Bodmer (install via Library Manager)

### Installation Steps

1. **Install ESP32 Board Support** (if not already done):
   - In Arduino IDE: Tools → Board Manager
   - Search for "esp32" and install "ESP32 by Espressif Systems"

2. **Install TFT_eSPI Library**:
   - Sketch → Include Library → Manage Libraries
   - Search for "TFT_eSPI" by Bodmer
   - Install the latest version

3. **Configure TFT_eSPI**:
   - Locate `TFT_eSPI/User_Setup.h` in your Arduino libraries folder
   - Ensure these settings match your hardware:
     ```cpp
     #define ILI9341_DRIVER
     #define TFT_WIDTH  240
     #define TFT_HEIGHT 320
     ```
   - Verify SPI pin assignments match your wiring

4. **Upload the Sketch**:
   - Open `WiFi_Monitor_ESP32.ino` in Arduino IDE
   - Select Board: ESP32 Dev Module
   - Select COM port for your ESP32
   - Click Upload

## Usage

### Basic Operation

1. **Power On**: Connect USB cable to ESP32 – the display will show a splash screen
2. **Automatic Scanning**: After initialization, the device begins scanning for networks every 3 seconds
3. **Mode Switching**: Press the **Boot button** (GPIO 25) on the ESP32 to cycle through display modes

### Display Modes

#### Mode 1: Network List
Shows all detected networks in a scrollable list with:
- **SSID**: Network name (truncated if >15 characters)
- **Signal Bar**: Visual representation of signal strength (0-100%)
- **Channel**: Which WiFi channel (1-14) the network operates on
- **RSSI**: Received Signal Strength Indicator in dBm (-100 to -40)
- **Security**: "SEC" (secured) or "OPEN" (unencrypted)

**Signal Strength Legend**:
- 🟢 Green: Excellent (-50 dBm or higher)
- 🟢 Yellow-Green: Very Good (-60 to -50 dBm)
- 🟡 Yellow: Good (-70 to -60 dBm)
- 🟠 Orange: Fair (-80 to -70 dBm)
- 🔴 Red: Poor (below -80 dBm)

#### Mode 2: Signal Graph
Line graph showing historical signal strength trends for the top 5 networks over 60 scan intervals (~3 minutes of history):
- Each network is color-coded (green, cyan, yellow, magenta, orange)
- Y-axis shows RSSI values from -40 to -100 dBm
- X-axis represents time (left = oldest, right = most recent)
- Legend displays network SSIDs with corresponding colors

#### Mode 3: Channel Analyzer
Bar chart visualization of network density across WiFi channels:
- **Bar Height**: Number of networks on each channel
- **Bar Color**: Signal strength of strongest network on that channel
- **Recommended Channel**: Suggests the least congested channel based on interference calculations
- **Total Networks**: Shows total count of detected networks

## Code Structure

### Key Functions

| Function | Purpose |
|----------|---------|
| `setup()` | Initialize display, WiFi, and perform initial scan |
| `loop()` | Main loop - handles scanning and UI updates |
| `scanNetworks()` | Performs WiFi scan and updates network array |
| `updateSignalHistory()` | Records RSSI values for historical graphing |
| `checkModeButton()` | Handles mode switching via button press |
| `drawUI()` | Router function for rendering active display mode |
| `drawListMode()` | Renders network list view |
| `drawGraphMode()` | Renders historical signal graph |
| `drawChannelAnalyzer()` | Renders channel interference visualization |
| `getRSSIColor()` | Returns color based on signal strength |

### Configuration Constants

```cpp
#define MAX_NETWORKS 20              // Max networks to track
#define SCAN_INTERVAL 3000           // Scan every 3 seconds
#define UPDATE_INTERVAL 100          // UI refresh every 100ms
#define HISTORY_SIZE 60              // Store 60 signal samples
#define BTN_MODE 25                  // Boot button pin
```

## Customization

### Change Scan Interval
```cpp
const unsigned long SCAN_INTERVAL = 3000;  // Change to 5000 for 5 seconds
```

### Modify Display Colors
Look for color definitions in the `drawNetworkItem()` and other rendering functions:
```cpp
TFT_GREEN, TFT_CYAN, TFT_YELLOW, etc.
```

### Increase Network Capacity
```cpp
#define MAX_NETWORKS 50  // Increased from 20
```

### Adjust UI Layout
Modify these constants to reshape the display:
```cpp
#define HEADER_HEIGHT 30
#define LIST_ITEM_HEIGHT 35
#define MAX_VISIBLE_ITEMS 7
```

## Troubleshooting

### Display Not Showing
- **Check pin connections**: Verify all SPI pins are correctly wired
- **Verify TFT_eSPI config**: Ensure User_Setup.h has correct driver (ILI9341) and pins
- **Test with TFT_eSPI example**: Run a simple TFT example to isolate display issues

### WiFi Scan Not Working
- **Check antenna**: Ensure ESP32's antenna is not blocked
- **Verify WiFi mode**: The sketch uses `WiFi_STA` mode (station, not AP)
- **Check Serial Monitor** (115200 baud) for debug messages

### Scanning Too Fast/Slow
- Adjust `SCAN_INTERVAL` constant (in milliseconds)
- Note: Very fast scans (< 1 second) may cause instability

## License

Open source - feel free to modify and distribute for educational/personal use.

## References

- [ESP32 WiFi Library](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi)
- [TFT_eSPI Library](https://github.com/Bodmer/TFT_eSPI)
- [ILI9341 Datasheet](https://www.displaytech.com/content/files/lcd%20datasheets/ILI9341_DS_V1.10.pdf)
- [WiFi Channels Reference](https://www.wifi-freqs.com/)

---

