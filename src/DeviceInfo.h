#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_dmx.h>

// 引用 main.cpp 中定义的全局变量
extern WebServer server;
extern const int startUniverse;
extern dmx_port_t dmxPort;


// ==================== Web 请求处理 ====================
void handleRoot();
void handleNotFound();

// ==================== 设备信息 HTML 页面 ====================
String getHtmlPage()
{
  IPAddress ip = WiFi.localIP();
  String ipStr = ip.toString();
  String mac = WiFi.macAddress();
  String ssid = WiFi.SSID();
  int rssi = WiFi.RSSI();
  int uptime = millis() / 1000;
  int freeHeap = ESP.getFreeHeap();

  // 信号强度百分比和颜色
  int pct = 0;
  if (rssi >= -50)
    pct = 100;
  else if (rssi <= -100)
    pct = 0;
  else
    pct = 2 * (rssi + 100);

  String sigColor = "#ef5350"; // red
  if (rssi >= -50)
    sigColor = "#4ade80";
  else if (rssi >= -70)
    sigColor = "#facc15";

  // 信号强度文字
  String sigText = "Weak";
  if (rssi >= -50)
    sigText = "Excellent";
  else if (rssi >= -60)
    sigText = "Good";
  else if (rssi >= -70)
    sigText = "Fair";
  else if (rssi >= -80)
    sigText = "Weak";

  // 运行时间格式化
  int h = uptime / 3600;
  int m = (uptime % 3600) / 60;
  int s = uptime % 60;
  String upStr = String(h) + "h " + String(m) + "m " + String(s) + "s";

  // 堆内存格式化 (KB)
  String heapStr = String(freeHeap / 1024) + "." + String((freeHeap % 1024) * 10 / 1024) + " KB";

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Art-Net to DMX Converter</title>";
  html += "<style>";
  html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
  html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;";
  html += " background: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);";
  html += " min-height: 100vh; color: #e2e8f0; padding: 20px; display: flex;";
  html += " justify-content: center; align-items: flex-start; }";
  html += ".container { max-width: 720px; width: 100%; }";
  html += ".header { text-align: center; padding: 32px 20px; margin-bottom: 24px;";
  html += " background: rgba(30, 41, 59, 0.6); border-radius: 16px;";
  html += " border: 1px solid rgba(148, 163, 184, 0.1);";
  html += " backdrop-filter: blur(10px); }";
  html += ".logo { font-size: 3em; margin-bottom: 8px; }";
  html += ".title { font-size: 1.5em; font-weight: 600; color: #f1f5f9; margin-bottom: 6px; }";
  html += ".subtitle { color: #94a3b8; font-size: 0.95em; }";
  html += ".status { display: inline-flex; align-items: center; gap: 8px;";
  html += " margin-top: 16px; padding: 6px 14px; border-radius: 20px;";
  html += " background: rgba(74, 222, 128, 0.12); color: #4ade80;";
  html += " font-size: 0.85em; font-weight: 500; }";
  html += ".dot { width: 8px; height: 8px; border-radius: 50%; background: #4ade80;";
  html += " box-shadow: 0 0 10px #4ade80; animation: pulse 2s infinite; }";
  html += "@keyframes pulse { 0%,100% { opacity: 1; } 50% { opacity: 0.4; } }";
  html += ".grid { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; }";
  html += "@media (max-width: 560px) { .grid { grid-template-columns: 1fr; } }";
  html += ".card { background: rgba(30, 41, 59, 0.6); border: 1px solid rgba(148, 163, 184, 0.1);";
  html += " border-radius: 14px; padding: 18px 20px; backdrop-filter: blur(10px);";
  html += " transition: transform 0.2s, border-color 0.2s; }";
  html += ".card:hover { transform: translateY(-2px); border-color: rgba(59, 130, 246, 0.4); }";
  html += ".card.full { grid-column: 1 / -1; }";
  html += ".card-label { color: #94a3b8; font-size: 0.75em; text-transform: uppercase;";
  html += " letter-spacing: 0.08em; margin-bottom: 8px; display: flex; align-items: center; gap: 6px; }";
  html += ".card-value { color: #f1f5f9; font-size: 1.15em; font-weight: 600; word-break: break-all; }";
  html += ".mono { font-family: 'SF Mono', Consolas, 'Courier New', monospace; }";
  html += ".highlight { color: #60a5fa; }";
  html += ".bar-wrap { margin-top: 10px; height: 6px; background: rgba(148, 163, 184, 0.15);";
  html += " border-radius: 3px; overflow: hidden; }";
  html += ".bar { height: 100%; border-radius: 3px; transition: width 0.6s;";
  html += " background: " + sigColor + "; box-shadow: 0 0 8px " + sigColor + "80; }";
  html += ".bar-meta { display: flex; justify-content: space-between; margin-top: 6px;";
  html += " font-size: 0.8em; color: #94a3b8; }";
  html += ".footer { text-align: center; margin-top: 24px; padding: 16px;";
  html += " color: #64748b; font-size: 0.8em; }";
  html += ".footer a { color: #60a5fa; text-decoration: none; font-weight: 500; }";
  html += ".footer a:hover { text-decoration: underline; }";
  html += ".split-row { display: flex; align-items: center; justify-content: space-between; gap: 16px; }";
  html += ".split-divider { width: 1px; align-self: stretch; background: rgba(148, 163, 184, 0.15); }";
  html += ".split-right { text-align: right; }";
  html += ".split-right .card-label { justify-content: flex-end; }";
  html += "@media (max-width: 560px) {";
  html += "  .split-row { flex-direction: column; align-items: stretch; gap: 12px; }";
  html += "  .split-divider { width: 100%; height: 1px; }";
  html += "  .split-right { text-align: left; }";
  html += "  .split-right .card-label { justify-content: flex-start; }";
  html += "}";
  html += "</style></head><body>";
  html += "<div class='container'>";

  // Header
  html += "<div class='header'>";
  html += "<div class='logo'>🎛️</div>";
  html += "<div class='title'>LED-BUG Art-Net to DMX Converter</div>";
  html += "<div class='subtitle'>Device Dashboard</div>";
  html += "<div class='status'><span class='dot'></span> Online</div>";
  html += "</div>";

  // Info grid
  html += "<div class='grid'>";

  // IP Address + Art-Net Port (full width, split layout)
  html += "<div class='card full'>";
  html += "<div class='split-row'>";
  html += "<div style='flex:1; min-width:0;'>";
  html += "<div class='card-label'>📡 IP Address</div>";
  html += "<div class='card-value mono highlight' style='font-size:1.6em;'>" + ipStr + "</div>";
  html += "<div style='color:#94a3b8; font-size:0.85em; margin-top:6px;'>🌐 artnet.local</div>";
  html += "</div>";
  html += "<div class='split-divider'></div>";
  html += "<div class='split-right'>";
  html += "<div class='card-label'>🎯 Art-Net Port</div>";
  html += "<div class='card-value mono highlight' style='font-size:1.6em;'>6454</div>";
  html += "<div style='color:#94a3b8; font-size:0.85em; margin-top:6px;'>UDP</div>";
  html += "</div></div>";
  html += "</div>";

  // WiFi SSID
  html += "<div class='card'>";
  html += "<div class='card-label'>📶 WiFi Network</div>";
  html += "<div class='card-value'>" + ssid + "</div>";
  html += "</div>";

  // Signal Strength
  html += "<div class='card'>";
  html += "<div class='card-label'>📊 Signal Strength</div>";
  html += "<div class='card-value' style='color:" + sigColor + ";'>" + String(rssi) + " dBm</div>";
  html += "<div class='bar-wrap'><div class='bar' style='width:" + String(pct) + "%;'></div></div>";
  html += "<div class='bar-meta'><span>" + sigText + "</span><span>" + String(pct) + "%</span></div>";
  html += "</div>";

  // DMX Universe
  html += "<div class='card'>";
  html += "<div class='card-label'>🎚️ Art-Net Universe</div>";
  html += "<div class='card-value mono highlight'>" + String(startUniverse) + "</div>";
  html += "</div>";

  // DMX Port
  html += "<div class='card'>";
  html += "<div class='card-label'>🔌 DMX Port</div>";
  html += "<div class='card-value mono highlight'>" + String(dmxPort) + "</div>";
  html += "</div>";

  // Channels
  html += "<div class='card'>";
  html += "<div class='card-label'>🎛️ DMX Channels</div>";
  html += "<div class='card-value mono highlight'>512</div>";
  html += "</div>";

  // MAC Address
  html += "<div class='card'>";
  html += "<div class='card-label'>🔖 MAC Address</div>";
  html += "<div class='card-value mono' style='font-size:0.95em;'>" + mac + "</div>";
  html += "</div>";

  // Uptime
  html += "<div class='card'>";
  html += "<div class='card-label'>⏱️ Uptime</div>";
  html += "<div class='card-value'>" + upStr + "</div>";
  html += "</div>";

  // Free Heap
  html += "<div class='card'>";
  html += "<div class='card-label'>💾 Free Heap</div>";
  html += "<div class='card-value'>" + heapStr + "</div>";
  html += "</div>";

  html += "</div>"; // end grid

  // Footer
  html += "<div class='footer'>LED-BUG Art-Net to DMX Converter </br> @ Made By Ben · <a href='https://led-bug.com/' target='_blank'>LED-BUG.com</a></div>";
  html += "</div></body></html>";

  return html;
}

void handleRoot()
{
  server.send(200, "text/html", getHtmlPage());
}

void handleNotFound()
{
  server.send(404, "text/plain", "404 Not Found");
}
