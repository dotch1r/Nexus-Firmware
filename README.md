# ⚡ NEXUS — M5StickC Plus2 Pentest Toolkit

> **A portable, animated, sound-enabled WiFi / IR attack toolkit.**  
> All the power you need — packed into a device the size of your thumb. 🔋

---

```
  _  _ ___ _  _ _   _ ___ 
 | \| | __| \/ | | | | / __|
 | .` | _| >  < | |_| \__ \
 |_|\_|___/_/\_|\___/|___/
         v1.0 — M5StickC Plus2
```

---

## 🚀 What Can It Do?

### 📡 WiFi Attacks
| Feature | Description |
|---|---|
| 🔍 **WiFi Scan** | Scan nearby networks and view signal strength |
| 🔗 **Connect WiFi** | Connect directly to a network from the device |
| 💀 **Deauth Attack** | Kick all clients off a target network |
| 🌊 **Deauth Flood** | Send deauth frames to all visible networks simultaneously |
| 📢 **Beacon Spam** | Flood the air with fake SSIDs |
| 👥 **Evil Twin** | Clone a target network and launch a captive portal |

### 🎣 Evil Twin — Two Modes
| Mode | Description |
|---|---|
| 🔐 **WiFi Password** | Victim is redirected to a fake WiFi login page |
| 🔵 **Google Login** | A pixel-perfect Google login page clone |

> Captured credentials are saved to LittleFS. View or delete them under **Files → Captured PWs**.

---

### 📺 IR Attacks — TV-B Gone
Cycles through 20 brand power codes:

`Samsung` `LG` `Sharp` `Philips` `Vizio` `Hisense` `TCL` `Toshiba` `Sanyo` `Sony` `JVC` `Panasonic` `Vestel` `Finlux` `Arçelik` `Beko` `Grundig` `Profilo` `Regal` `SEG`

---

### 💾 File Management
- 📁 LittleFS file list with size info
- 🗝️ View and delete captured credentials
- 💿 SD Card support *(coming soon)*

---

### ⚙️ Settings
- 🔆 Brightness control
- 🔊 Sound on/off toggle
- 😴 Sleep timer (10s / 15s / 30s / 60s / Never)
- 🕐 Date & Time configuration (RTC)
- ℹ️ Device info

---

## 🎵 Boot Animation

On startup, the ASCII NEXUS logo fades in line by line, followed by a rising boot melody, while each module is individually verified as `[OK]`.

---

## 🕹️ Button Layout

| Button | Short Press | Long Press |
|---|---|---|
| **BtnA** (large) | Select / Add character | Confirm / Save |
| **BtnB** (side) | Down / Next | Backspace / Clear |
| **BtnPWR** (power) | Up / Back | Return to main menu |

---

## 📦 Installation

### Requirements
- [Arduino IDE](https://www.arduino.cc/en/software) or VSCode + PlatformIO
- **M5StickC Plus2** board package
- No external libraries required ✅ — only `M5StickCPlus2.h`

### Steps
```bash
git clone https://github.com/dotch1r/nexus-fw
```
1. Open `nexus_firmware.ino` in Arduino IDE
2. Select board: **M5StickC Plus2**
3. Upload and watch it boot 🚀

---

## 🔧 Hardware

| Spec | Detail |
|---|---|
| Device | M5StickC Plus2 |
| Chip | ESP32-PICO-V3-02 |
| Display | 1.14" IPS 240×135 px |
| IR LED | GPIO 19 |
| Buzzer | GPIO 2 |
| WiFi | 802.11 b/g/n |

---

## ⚠️ Legal Disclaimer

> This software is developed strictly for **educational purposes** and **authorized testing on your own networks only**.  
> Attacking networks without explicit permission is illegal.  
> The author takes no responsibility for any misuse of this software.

---

## 📜 License

© 2025 [dotch1r](https://github.com/dotch1r) — All Rights Reserved.  
See [LICENSE](./LICENSE) for details. You must obtain written permission from the author before using, distributing, or modifying this software.

---

<p align="center">
  <b>Built with ❤️ and a lot of ☕ for the M5StickC Plus2</b><br>
  <a href="https://github.com/dotch1r">github.com/dotch1r</a>
</p>
