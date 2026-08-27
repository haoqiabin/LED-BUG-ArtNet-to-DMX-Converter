# LED-BUG Art-Net to DMX Converter

A wireless-to-wired bridge that receives [Art-Net](https://en.wikipedia.org/wiki/Art-Net) (DMX over UDP/WiFi) packets and retransmits them as DMX512 serial data over a MAX485 transceiver. Built on PlatformIO / Arduino framework for the ESP32.

**Author:** Ben · [LED-BUG.com](https://led-bug.com/)

---

## Features

### Art-Net / Network

- **Art-Net DMX reception** — listens on UDP port 6454 (standard Art-Net port)
- **Universe filtering** — only processes packets matching a configurable target universe (`startUniverse`)
- **Full 512-channel DMX output** — handles an entire DMX512 universe per packet
- **mDNS discovery** — hostname `artnet`, reachable as `artnet.local` on the network

### WiFi Configuration (Captive Portal)

- **Custom web-based config portal** — no third-party WiFi library; built with `WebServer` + `DNSServer`
- **WiFi scanning** — config page includes a live scan button that lists nearby networks with signal strength bars
- **First boot / failed connection** — automatically opens a config hotspot `LED-BUG Art-Net Config`
- **Web setup flow** — phone connects to the AP, opens `192.168.4.1`, selects a WiFi network and enters the password
- **Credentials auto-save** — saved to ESP32 NVS via `esp_wifi_restore()`, automatically reconnects on subsequent boots
- **Config portal timeout** — configurable via `timeOut_s` parameter (default 10s); falls back to config portal on timeout
- **Connection retry** — failed credentials show an error page and let the user retry without rebooting
- **Optional static IP** — uncomment `#define DHCP_DISABLED` to use a fixed IP instead of DHCP

### Factory Reset (BOOT Button)

- **Runtime reset** — hold BOOT button during normal operation for 3 seconds to clear WiFi credentials and reboot into config portal
- **LED feedback** — LED blinks 5 times on reset to confirm the action

### Status LED (GPIO4)

| State | LED Behavior |
|-------|-------------|
| WiFi connecting | Blinking |
| WiFi connected | Off |
| Config portal active | On (solid) |
| WiFi reset triggered | 5 blinks, then off |

### Device Info Web Dashboard

Once connected to WiFi, visit `http://artnet.local` (or the device IP) to view a live dashboard:

- IP Address & Art-Net port (6454)
- WiFi SSID, signal strength (dBm + bar), MAC address
- Art-Net universe, DMX port, DMX channels (512)
- Uptime, free heap memory

### DMX512 Output

- **TX-only mode** — minimal wiring: MAX485 DE/RE tied to 5V, no RX or enable pin needed
- **esp_dmx driver (v4.1.0)** — standard-compliant DMX512 output
- **No RDM personalities** — configured as a transparent passthrough converter (`personality_count = 0`)
- **Standard DMX512 timing** — 250 kbaud, 8N2; break + start code + 512 data bytes

### Diagnostics

- **Serial debug output (115200 baud)** — WiFi status, IP address, SSID, DMX init state, BOOT button events, config portal heartbeat

---

## Hardware

| Component | Detail |
|-----------|--------|
| **MCU** | ESP32 DOIT DevKit v1 |
| **DMX transceiver** | MAX485 (single, TX-only) |
| **Wiring** | ESP32 GPIO2 → MAX485 DI; DE & RE → 5V; RO unconnected |
| **BOOT button** | GPIO0 (onboard) — long press 3s to reset WiFi |
| **Status LED** | GPIO4 — indicates WiFi / config portal state |
| **Serial monitor** | 115200 baud |

### Wiring Diagram

```
ESP32                MAX485               DMX512 Output
─────                ──────               ─────────────
GPIO2  ───────────►  DI                  XLR Pin 3 (Data+)
       3.3V/5V ───►  VCC                 XLR Pin 2 (Data−)
       GND    ────►  GND                 XLR Pin 1 (GND)
                     DE ──── 5V
                     RE ──── 5V
                     RO     (NC)

GPIO4  ───────────►  Status LED (active HIGH)
BOOT Button (GPIO0) ──── onboard ──── active LOW (press = GND)
```

---

## Configuration

All user settings are at the top of `src/main.cpp` and `src/configwifi.h`:

| Constant | File | Type | Default | Description |
|----------|------|------|---------|-------------|
| `startUniverse` | `main.cpp` | `int` | `0` | Art-Net universe to listen on |
| `transmitPin` | `main.cpp` | `int` | `2` (GPIO2) | DMX TX pin → MAX485 DI |
| `receivePin` | `main.cpp` | `int` | `-1` | DMX RX pin (unused in TX-only mode) |
| `enablePin` | `main.cpp` | `int` | `-1` | DMX enable pin (unused, DE tied to 5V) |
| `BOOT_PIN` | `configwifi.h` | `int` | `0` (GPIO0) | Factory reset button pin |
| `LED_PIN` | `configwifi.h` | `int` | `4` (GPIO4) | Status LED pin |
| `HOST_NAME` | `configwifi.h` | `const char*` | `"LED-BUG Art-Net to DMX Converter"` | mDNS / DHCP hostname |
| `AP_SSID` | `configwifi.h` | `const char*` | `"LED-BUG Art-Net Config"` | Config portal hotspot name |
| `AP_IP` | `configwifi.h` | `IPAddress` | `192.168.4.1` | Config portal IP address |

### Optional Static IP

Uncomment `#define DHCP_DISABLED` in `src/configwifi.h` and edit the IP settings:

```cpp
#define DHCP_DISABLED

#ifdef DHCP_DISABLED
IPAddress local_IP(192, 168, 1, 154);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 1);
IPAddress secondaryDNS(1, 1, 1, 1);
#endif
```

---

## Usage

### First Boot

1. Power on the ESP32
2. No saved WiFi → LED starts blinking, config portal opens after timeout
3. Connect your phone to the `LED-BUG Art-Net Config` WiFi network
4. Open browser → `192.168.4.1` (captive portal redirects automatically)
5. Tap 🔍 to scan for nearby WiFi networks, or type the SSID manually
6. Select a network, enter the password, tap **Connect**
7. LED turns off → WiFi connected, serial monitor shows assigned IP
8. Visit `http://artnet.local` to see the device dashboard

### Subsequent Boots

ESP32 automatically connects to the saved WiFi network. No user action needed.

### Reconfigure WiFi

**While the device is running:**
1. Press and hold the BOOT button
2. Hold for 3 seconds (serial shows confirmation)
3. WiFi credentials are cleared (`esp_wifi_restore()`), LED blinks 5 times
4. Device reboots into config portal automatically

---

## Build & Upload

Requires [PlatformIO](https://platformio.org/).

```bash
# Install dependencies and build
pio run

# Upload via USB
pio run --target upload

# Open serial monitor
pio device monitor
```

### Dependencies (auto-fetched by PlatformIO)

| Library | Version | Purpose |
|---------|---------|---------|
| [ArtnetWifi](https://github.com/rstephan/ArtnetWifi) | 1.6.2 | Art-Net protocol (UDP port 6454) |
| [esp_dmx](https://github.com/someweisguy/esp_dmx) | 4.1.0 | DMX512 TX/RX driver |

---

## How It Works

```
┌──────────────┐    UDP :6454     ┌──────────┐    UART 250kbaud    ┌────────┐
│  Art-Net     │ ───────────────► │  ESP32   │ ──────────────────► │ MAX485 │ ──► DMX512
│  Controller  │   WiFi 2.4 GHz   │          │   GPIO2 (TX)        │        │
└──────────────┘                  └──────────┘                     └────────┘
                                       ▲
                                       │ WiFi config / dashboard
                                 ┌─────┴──────────┐
                                 │ Phone / Laptop │
                                 │ artnet.local   │
                                 └────────────────┘
```

### Boot Sequence

1. **Serial init** — 115200 baud
2. **BOOT button init** — GPIO0 as `INPUT_PULLUP`
3. **LED init** — GPIO4 as output, LED off
4. **WiFi connect** — attempt saved credentials from NVS; timeout → start config portal
5. **Web server** — `handleRoot` (device info dashboard) + 404 handler on port 80
6. **mDNS** — register `artnet.local`, advertise `_http._tcp` service
7. **Art-Net** — begin UDP listener on port 6454, register `onArtNetFrame` callback
8. **DMX driver** — install `esp_dmx` in TX-only mode on port 1, configure GPIO2 as TX

### Runtime Loop

```
loop()
  ├── resetWiFibtn()             ← BOOT button: long press 3s → esp_wifi_restore() & reboot
  ├── if WiFi connected:
  │     └── artnet.read()        ← polls UDP, fires onArtNetFrame callback
  │           └── callback: copy DMX data to buffer, set updateDMX flag
  ├── if updateDMX:
  │     ├── dmx_write()          ← copy buffer to DMX driver
  │     ├── dmx_send_num()       ← transmit DMX frame
  │     └── dmx_wait_sent()      ← wait for transmission complete
  └── server.handleClient()      ← serve device info dashboard
```

### Config Portal Flow

```
connectWIFI() timeout
  └── startConfigPortal()
        ├── WiFi.mode(WIFI_AP)
        ├── WiFi.softAP("LED-BUG Art-Net Config")
        ├── initDNS()           ← captive portal: all domains → 192.168.4.1
        ├── portalServer.begin()
        │     ├── GET  /        → config page (WiFi scan + input form)
        │     ├── GET  /scan    → JSON list of nearby networks
        │     ├── POST /save    → attempt connection, show success/error page
        │     └── *             → 302 redirect to /
        ├── LED solid ON
        └── loop until connected → close portal, return to normal operation
```

---

## Project Structure

```
Artnet-DMX/
├── platformio.ini          ← Build config, board, dependencies
├── README.md               ← This file
├── src/
│   ├── main.cpp            ← Application entry: setup(), loop(), Art-Net callback
│   ├── configwifi.h        ← WiFi config portal, captive DNS, LED control, reset logic
│   └── DeviceInfo.h        ← Device info web dashboard (HTML page)
├── include/                ← (for project headers)
├── lib/                    ← (for local libraries)
├── test/                   ← (for tests)
└── .vscode/                ← VS Code / PlatformIO IDE settings
```

---

## License

Not specified. Contact the author for licensing terms.
