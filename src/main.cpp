/*

  ESP32 ArtNet to DMX Converter

  This sketch allows you to receive 1 ArtNet universes from a WiFi network
  and to send the received data in 1 physical DMX universes.

  Based on the DMX_write sketch by Mitch Weisbord (https://github.com/someweisguy/esp_dmx)
  and on ArtNetWifiNeoPixel sketch by rstephan (https://github.com/rstephan/ArtnetWifi)

*/

#include <Arduino.h>
#include <esp_dmx.h>    //v4.1.0
#include <ArtnetWifi.h> //v1.6.2
#include "configwifi.h"
#include "DeviceInfo.h"

ArtnetWifi artnet;
WebServer server(80);

// ==================== DMX Hardware Pins ====================
/* First, lets define the hardware pins that we are using with our ESP32. We
  need to define which pin is transmitting data and which pin is receiving data.
  DMX circuits also often need to be told when we are transmitting and when we
  are receiving data. We can do this by defining an enable pin. */
int transmitPin = 2; // 发送引脚
int receivePin = -1; // 不接引脚
int enablePin = -1;  // 直接5V

/* Make sure to double-check that these pins are compatible with your ESP32!
  Some ESP32s, such as the ESP32-WROVER series, do not allow you to read or
  write data on pins 16 or 17, so it's always good to read the manuals. */

/* Next, lets decide which DMX port to use. The ESP32 has either 2 or 3 ports.
  Port 0 is typically used to transmit serial data back to your Serial Monitor,
  so we shouldn't use that port. Lets use port 1! */
dmx_port_t dmxPort = 1;

/* Now we want somewhere to store our DMX data. Since a single packet of DMX
  data can be up to 513 bytes long, we want our array to be at least that long.
  This library knows that the max DMX packet size is 513, so we can fill in the
  array size with `DMX_PACKET_SIZE`. */

byte data[DMX_PACKET_SIZE];

// ArtNet settings
const int startUniverse = 0; // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.

// 全局变量
volatile bool updateDMX = false;

// ArtNet接收的函数
void onArtNetFrame(uint16_t universe, uint16_t numberOfChannels, uint8_t sequence, uint8_t *dmxData)
{
  if (universe == startUniverse)
  {
    for (int i = 0; i < numberOfChannels; i++)
      data[i + 1] = dmxData[i];
    updateDMX = true;
  }
}

void setup()
{
  /* Start the serial connection back to the computer so that we can log messages to the Serial Monitor. Lets set the baud rate to 115200. */
  Serial.begin(115200);

  Serial.println("\n=== LED-BUG Art-Net to DMX Converter ===");

  // 检查 BOOT 按键（长按 3 秒清除 WiFi 配置）
  pinMode(BOOT_PIN, INPUT_PULLUP);
  // 初始化指示灯
  initLED();

  // 连接WIFI
  connectWIFI(10);

  // 网页与mDNS服务
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started on port 80");

  initMDNS();

  // ---- Art-Net Setup ----
  artnet.setArtDmxCallback(onArtNetFrame);
  artnet.begin("ESP32-ArtNet-to-DMX-Converter");

  // ---- DMX Driver Setup ----
  /* Now we will install the DMX driver! We'll tell it which DMX port to use,
     what device configuration to use, and what DMX personalities it should have.
     If you aren't sure which configuration to use, you can use the macros
     `DMX_CONFIG_DEFAULT` to set the configuration to its default settings.
     Because the device is being setup as a DMX controller, this device won't use
     any DMX personalities. */
  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_personality_t personalities[] = {};
  int personality_count = 0;
  dmx_driver_install(dmxPort, &config, personalities, personality_count);

  /* Set the DMX hardware pins to the pins that we want to use. */
  dmx_set_pin(dmxPort, transmitPin, receivePin, enablePin);
}

void loop()
{
  resetWiFibtn();
  // 连接WIFI后读取ArtNet信号
  if ((WiFi.status() == WL_CONNECTED))
  {
    artnet.read();
  }

  // 发送DMX信号
  if (updateDMX)
  {
    dmx_write(dmxPort, data, DMX_PACKET_SIZE);
    dmx_send_num(dmxPort, DMX_PACKET_SIZE);
    dmx_wait_sent(dmxPort, DMX_TIMEOUT_TICK);
    updateDMX = false;
  }

  // 处理网页请求
  server.handleClient();
}