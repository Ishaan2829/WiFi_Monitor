/*
 * WiFi Packet Monitor for ESP32 + ILI9341
 * Mini Wireshark-style network scanner
 * 
 * Hardware:
 * - ESP32 DevKit V1
 * - ILI9341 240x320 TFT Display (SPI)
 * 
 * Features:
 * - Real-time WiFi network scanning
 * - SSID, RSSI, Channel display
 * - Signal strength graphs
 * - Channel analyzer
 * - Auto-updating display
 */

#include <WiFi.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// Display modes
enum DisplayMode {
  MODE_LIST,
  MODE_GRAPH,
  MODE_CHANNEL_ANALYZER
};

DisplayMode currentMode = MODE_LIST;

// Network data structure
struct NetworkInfo {
  String ssid;
  int32_t rssi;
  uint8_t channel;
  uint8_t encryption;
  bool active;
};

#define MAX_NETWORKS 20
NetworkInfo networks[MAX_NETWORKS];
int networkCount = 0;

// UI Settings
#define HEADER_HEIGHT 30
#define STATUS_HEIGHT 20
#define LIST_ITEM_HEIGHT 35
#define MAX_VISIBLE_ITEMS 7

int scrollOffset = 0;
unsigned long lastScan = 0;
unsigned long lastUpdate = 0;
const unsigned long SCAN_INTERVAL = 3000;  // Scan every 3 seconds
const unsigned long UPDATE_INTERVAL = 100; // UI update every 100ms

// Button pins (optional - for mode switching)
#define BTN_MODE 25  // Boot button on ESP32
bool lastButtonState = HIGH;

// Signal strength history for graphing
#define HISTORY_SIZE 60
int signalHistory[MAX_NETWORKS][HISTORY_SIZE];
int historyIndex = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize display
  tft.init();
  tft.setRotation(0); // Portrait mode
  tft.fillScreen(TFT_BLACK);
  
  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Initialize button
  pinMode(BTN_MODE, INPUT_PULLUP);
  
  // Initialize signal history
  for (int i = 0; i < MAX_NETWORKS; i++) {
    for (int j = 0; j < HISTORY_SIZE; j++) {
      signalHistory[i][j] = -100;
    }
  }
  
  // Show splash screen
  showSplash();
  delay(2000);
  
  // Initial scan
  scanNetworks();
  drawUI();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check mode button
  checkModeButton();
  
  // Periodic network scan
  if (currentTime - lastScan >= SCAN_INTERVAL) {
    lastScan = currentTime;
    scanNetworks();
    updateSignalHistory();
  }
  
  // Update display
  if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = currentTime;
    drawUI();
  }
  
  delay(10);
}

void scanNetworks() {
  Serial.println("Scanning networks...");
  int n = WiFi.scanNetworks(false, true); // async=false, show_hidden=true
  
  networkCount = min(n, MAX_NETWORKS);
  
  for (int i = 0; i < networkCount; i++) {
    networks[i].ssid = WiFi.SSID(i);
    networks[i].rssi = WiFi.RSSI(i);
    networks[i].channel = WiFi.channel(i);
    networks[i].encryption = WiFi.encryptionType(i);
    networks[i].active = true;
    
    // Truncate long SSIDs
    if (networks[i].ssid.length() > 18) {
      networks[i].ssid = networks[i].ssid.substring(0, 15) + "...";
    }
  }
  
  Serial.printf("Found %d networks\n", networkCount);
}

void updateSignalHistory() {
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  
  for (int i = 0; i < networkCount && i < MAX_NETWORKS; i++) {
    signalHistory[i][historyIndex] = networks[i].rssi;
  }
}

void checkModeButton() {
  bool buttonState = digitalRead(BTN_MODE);
  
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Button pressed
    currentMode = (DisplayMode)((currentMode + 1) % 3);
    tft.fillScreen(TFT_BLACK);
    drawUI();
    delay(500); // Debounce
    Serial.printf("button pressed=",buttonState);
  }
  
  lastButtonState = buttonState;
}

void drawUI() {
  switch (currentMode) {
    case MODE_LIST:
      drawListMode();
      break;
    case MODE_GRAPH:
      drawGraphMode();
      break;
    case MODE_CHANNEL_ANALYZER:
      drawChannelAnalyzer();
      break;
  }
}

void showSplash() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(30, 100);
  tft.println("WiFi Monitor");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(50, 140);
  tft.println("ESP32 Network Scanner");
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(60, 170);
  tft.println("Initializing...");
}

void drawListMode() {
  // Draw header
  drawHeader("NETWORK LIST");
  
  // Draw network list
  int startY = HEADER_HEIGHT;
  int visibleCount = min(MAX_VISIBLE_ITEMS, networkCount);
  
  for (int i = 0; i < visibleCount; i++) {
    int idx = i + scrollOffset;
    if (idx >= networkCount) break;
    
    int y = startY + (i * LIST_ITEM_HEIGHT);
    drawNetworkItem(networks[idx], y, i % 2 == 0);
  }
  
  // Draw status bar
  drawStatusBar();
}

void drawNetworkItem(NetworkInfo &net, int y, bool alternate) {
  // Background
  uint16_t bgColor = alternate ? TFT_NAVY : TFT_BLACK;
  tft.fillRect(0, y, 240, LIST_ITEM_HEIGHT, bgColor);
  
  // SSID
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, bgColor);
  tft.setCursor(5, y + 5);
  tft.println(net.ssid.length() > 0 ? net.ssid : "<Hidden>");
  
  // Signal strength bar
  int signalPercent = constrain(map(net.rssi, -100, -40, 0, 100), 0, 100);
  drawSignalBar(5, y + 18, 100, 12, signalPercent, net.rssi);
  
  // Channel
  tft.setTextColor(TFT_YELLOW, bgColor);
  tft.setCursor(110, y + 5);
  tft.printf("Ch:%d", net.channel);
  
  // RSSI value
  tft.setTextColor(getRSSIColor(net.rssi), bgColor);
  tft.setCursor(110, y + 18);
  tft.printf("%ddBm", net.rssi);
  
  // Encryption icon
  tft.setTextColor(net.encryption == WIFI_AUTH_OPEN ? TFT_RED : TFT_GREEN, bgColor);
  tft.setCursor(180, y + 12);
  tft.println(net.encryption == WIFI_AUTH_OPEN ? "OPEN" : "SEC");
}

void drawSignalBar(int x, int y, int w, int h, int percent, int rssi) {
  // Border
  tft.drawRect(x, y, w, h, TFT_DARKGREY);
  
  // Fill based on signal strength
  int fillWidth = (w - 2) * percent / 100;
  uint16_t color = getRSSIColor(rssi);
  
  tft.fillRect(x + 1, y + 1, fillWidth, h - 2, color);
  tft.fillRect(x + 1 + fillWidth, y + 1, w - 2 - fillWidth, h - 2, TFT_BLACK);
}

void drawGraphMode() {
  drawHeader("SIGNAL GRAPH");
  
  // Draw graph area
  int graphX = 10;
  int graphY = HEADER_HEIGHT + 10;
  int graphW = 220;
  int graphH = 200;
  
  // Background
  tft.fillRect(graphX, graphY, graphW, graphH, TFT_BLACK);
  
  // Draw grid
  tft.drawRect(graphX, graphY, graphW, graphH, TFT_DARKGREY);
  
  // Horizontal grid lines (RSSI levels)
  for (int i = 0; i <= 4; i++) {
    int y = graphY + (graphH * i / 4);
    tft.drawLine(graphX, y, graphX + graphW, y, TFT_DARKGREY);
    
    // RSSI labels
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    int rssiValue = -40 - (15 * i);
    tft.setCursor(graphX + graphW + 5, y - 4);
    tft.printf("%d", rssiValue);
  }
  
  // Draw signal lines for top networks
  int maxLines = min(5, networkCount);
  uint16_t colors[] = {TFT_GREEN, TFT_CYAN, TFT_YELLOW, TFT_MAGENTA, TFT_ORANGE};
  
  for (int netIdx = 0; netIdx < maxLines; netIdx++) {
    uint16_t color = colors[netIdx % 5];
    
    for (int i = 1; i < HISTORY_SIZE; i++) {
      int idx1 = (historyIndex + i - 1) % HISTORY_SIZE;
      int idx2 = (historyIndex + i) % HISTORY_SIZE;
      
      int rssi1 = signalHistory[netIdx][idx1];
      int rssi2 = signalHistory[netIdx][idx2];
      
      if (rssi1 > -100 && rssi2 > -100) {
        int x1 = graphX + (i - 1) * graphW / HISTORY_SIZE;
        int x2 = graphX + i * graphW / HISTORY_SIZE;
        
        int y1 = graphY + graphH - map(constrain(rssi1, -100, -40), -100, -40, 0, graphH);
        int y2 = graphY + graphH - map(constrain(rssi2, -100, -40), -100, -40, 0, graphH);
        
        tft.drawLine(x1, y1, x2, y2, color);
      }
    }
  }
  
  // Draw legend
  int legendY = graphY + graphH + 15;
  tft.setTextSize(1);
  for (int i = 0; i < maxLines; i++) {
    int x = 10 + (i % 2) * 120;
    int y = legendY + (i / 2) * 15;
    
    tft.fillRect(x, y, 10, 10, colors[i % 5]);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(x + 15, y + 2);
    
    String ssid = networks[i].ssid;
    if (ssid.length() > 12) ssid = ssid.substring(0, 9) + "...";
    tft.println(ssid);
  }
  
  drawStatusBar();
}

void drawChannelAnalyzer() {
  drawHeader("CHANNEL ANALYZER");
  
  // Count networks per channel
  int channelCount[14] = {0};
  int channelMaxRSSI[14];
  for (int i = 0; i < 14; i++) channelMaxRSSI[i] = -100;
  
  for (int i = 0; i < networkCount; i++) {
    int ch = networks[i].channel;
    if (ch >= 1 && ch <= 14) {
      channelCount[ch - 1]++;
      if (networks[i].rssi > channelMaxRSSI[ch - 1]) {
        channelMaxRSSI[ch - 1] = networks[i].rssi;
      }
    }
  }
  
  // Draw channel bars
  int barWidth = 15;
  int spacing = 2;
  int maxBarHeight = 180;
  int startX = 10;
  int startY = HEADER_HEIGHT + 20;
  
  // Find max count for scaling
  int maxCount = 1;
  for (int i = 0; i < 14; i++) {
    if (channelCount[i] > maxCount) maxCount = channelCount[i];
  }
  
  // Draw bars
  for (int i = 0; i < 14; i++) {
    int x = startX + i * (barWidth + spacing);
    int barHeight = (channelCount[i] * maxBarHeight) / maxCount;
    int y = startY + maxBarHeight - barHeight;
    
    // Color based on signal strength
    uint16_t color = getRSSIColor(channelMaxRSSI[i]);
    if (channelCount[i] == 0) color = TFT_DARKGREY;
    
    tft.fillRect(x, y, barWidth, barHeight, color);
    
    // Channel number
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(x, startY + maxBarHeight + 5);
    tft.printf("%d", i + 1);
    
    // Network count
    if (channelCount[i] > 0) {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setCursor(x + 3, y - 10);
      tft.printf("%d", channelCount[i]);
    }
  }
  
  // Draw recommended channel
  int minInterference = 999;
  int recommendedCh = 1;
  
  for (int i = 0; i < 14; i++) {
    int interference = 0;
    for (int j = max(0, i - 2); j <= min(13, i + 2); j++) {
      interference += channelCount[j];
    }
    if (interference < minInterference) {
      minInterference = interference;
      recommendedCh = i + 1;
    }
  }
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(10, startY + maxBarHeight + 25);
  tft.printf("Recommended: Channel %d", recommendedCh);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, startY + maxBarHeight + 40);
  tft.printf("Total Networks: %d", networkCount);
  
  drawStatusBar();
}

void drawHeader(const char* title) {
  tft.fillRect(0, 0, 240, HEADER_HEIGHT, TFT_BLUE);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  
  // Center the title
  int textWidth = strlen(title) * 12;
  int x = (240 - textWidth) / 2;
  tft.setCursor(x, 8);
  tft.println(title);
}

void drawStatusBar() {
  int y = 320 - STATUS_HEIGHT;
  tft.fillRect(0, y, 240, STATUS_HEIGHT, TFT_DARKGREY);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(5, y + 6);
  
  switch (currentMode) {
    case MODE_LIST:
      tft.printf("Mode: LIST | APs: %d", networkCount);
      break;
    case MODE_GRAPH:
      tft.printf("Mode: GRAPH | APs: %d", networkCount);
      break;
    case MODE_CHANNEL_ANALYZER:
      tft.printf("Mode: CHANNEL | APs: %d", networkCount);
      break;
  }
  
  // Show scan indicator
  tft.fillCircle(230, y + 10, 4, TFT_GREEN);
}

uint16_t getRSSIColor(int rssi) {
  if (rssi >= -50) return TFT_GREEN;
  else if (rssi >= -60) return TFT_GREENYELLOW;
  else if (rssi >= -70) return TFT_YELLOW;
  else if (rssi >= -80) return TFT_ORANGE;
  else return TFT_RED;
}
