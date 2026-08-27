#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <esp_wifi.h> //用于esp_wifi_restore() 删除保存的wifi信息

DNSServer dnsServer;      // 创建dnsServer实例
const byte DNS_PORT = 53; // 设置DNS端口号

// ==================== BOOT 按键与LED灯 设置 ====================
// 用于长按清除 WiFi 配置 / 重置设备
const int BOOT_PIN = 0; // GPIO0 = BOOT 按键
const int LED_PIN = 4;  // GPIO4 指示LED灯

// ==================== 静态 IP 配置 ====================
// 取消下面这行的注释以启用静态 IP，需要与网关IP保持一致
// #define DHCP_DISABLED
#ifdef DHCP_DISABLED
IPAddress local_IP(192, 168, 1, 154);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 1); // optional
IPAddress secondaryDNS(1, 1, 1, 1);   // optional
#endif


const char *HOST_NAME = "LED-BUG Art-Net to DMX Converter"; // 设置设备名
const char *AP_SSID = "LED-BUG Art-Net Config";             // 设置AP热点名称
IPAddress AP_IP(192, 168, 4, 1);                            // 设置AP的IP地址

/*
 * LED初始化
 * LED灯状态说明：
 * WiFi 未连接时：LED 亮
 * WiFi 连接成功时：LED 灭
 * WiFi 连接过程中：LED 闪烁
 */
void initLED()
{
  pinMode(LED_PIN, OUTPUT);   // 配置LED引脚为输出
  digitalWrite(LED_PIN, LOW); // 初始灯灭
}

/*
 * LED闪烁函数,用不上LED可删除
 * blinkLED(引脚，闪烁次数，時间)
 */
void blinkLED(int led, int n, int t)
{
  for (int i = 0; i < 2 * n; i++)
  {
    digitalWrite(led, !digitalRead(led));
    delay(t);
  }
}

// ==================== DNS (captive portal)初始化 ====================
void initDNS()
{
  // 启动DNS服务器
  if (dnsServer.start(DNS_PORT, "*", AP_IP))
  {
    Serial.println("✅ DNS server started (captive portal)");
  }
  else
  {
    Serial.println("❌ DNS server failed to start");
  }
}

// ==================== mDNS 初始化 ====================
void initMDNS()
{
  if (!MDNS.begin("artnet"))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS started: http://artnet.local");
  MDNS.addService("http", "tcp", 80);
}

// ==================== 配网状态标志 ====================
static bool configPortalActive = false;   // 配网门户是否激活
static WebServer *portalServer = nullptr; // 配网专用WebServer，不污染主server

// ==================== 配网页面 HTML ====================
String getConfigPage(String message = "", bool isError = false)
{
  String msgColor = isError ? "#ef5350" : "#4ade80";
  String msgBg = isError ? "rgba(239,83,80,0.12)" : "rgba(74,222,128,0.12)";

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ArtNet WiFi Config</title>";
  html += "<style>";
  html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
  html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;";
  html += " background: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);";
  html += " min-height: 100vh; color: #e2e8f0; padding: 20px; display: flex;";
  html += " justify-content: center; align-items: center; }";
  html += ".card { max-width: 420px; width: 100%; background: rgba(30, 41, 59, 0.6);";
  html += " border: 1px solid rgba(148, 163, 184, 0.1); border-radius: 16px;";
  html += " padding: 32px 28px; backdrop-filter: blur(10px); }";
  html += ".header { text-align: center; margin-bottom: 28px; }";
  html += ".logo { font-size: 3em; margin-bottom: 8px; }";
  html += ".title { font-size: 1.3em; font-weight: 600; color: #f1f5f9; margin-bottom: 6px; }";
  html += ".subtitle { color: #94a3b8; font-size: 0.9em; }";
  html += ".form-group { margin-bottom: 18px; }";
  html += "label { display: block; color: #94a3b8; font-size: 0.8em; text-transform: uppercase;";
  html += " letter-spacing: 0.06em; margin-bottom: 6px; }";
  html += "input[type='text'], input[type='password'] { width: 100%; padding: 12px 14px;";
  html += " background: rgba(15, 23, 42, 0.6); border: 1px solid rgba(148, 163, 184, 0.2);";
  html += " border-radius: 10px; color: #f1f5f9; font-size: 1em; outline: none;";
  html += " transition: border-color 0.2s; }";
  html += "input:focus { border-color: rgba(59, 130, 246, 0.6); }";
  html += "input::placeholder { color: #475569; }";
  html += ".btn { width: 100%; padding: 13px; background: linear-gradient(135deg, #3b82f6, #2563eb);";
  html += " color: white; border: none; border-radius: 10px; font-size: 1em;";
  html += " font-weight: 600; cursor: pointer; transition: transform 0.15s, box-shadow 0.15s;";
  html += " margin-top: 8px; }";
  html += ".btn:hover { transform: translateY(-1px); box-shadow: 0 4px 16px rgba(59,130,246,0.3); }";
  html += ".btn:active { transform: translateY(0); }";
  html += ".msg { padding: 10px 14px; border-radius: 10px; margin-bottom: 18px;";
  html += " font-size: 0.9em; text-align: center; color: " + msgColor + ";";
  html += " background: " + msgBg + "; }";
  html += ".footer { text-align: center; margin-top: 20px; color: #475569; font-size: 0.78em; }";
  html += ".hint { color: #94a3b8; font-size: 0.8em; margin-top: 4px; }";
  // WiFi扫描相关样式
  html += ".btn-scan { padding: 12px 14px; background: rgba(59,130,246,0.15); border: 1px solid rgba(59,130,246,0.3);";
  html += " border-radius: 10px; color: #60a5fa; font-size: 1.1em; cursor: pointer; transition: all 0.2s; }";
  html += ".btn-scan:hover { background: rgba(59,130,246,0.25); border-color: rgba(59,130,246,0.5); }";
  html += ".btn-scan:disabled { opacity: 0.5; cursor: not-allowed; }";
  html += ".wifi-list { margin-top: 8px; max-height: 220px; overflow-y: auto; border-radius: 10px;";
  html += " background: rgba(15, 23, 42, 0.4); -webkit-overflow-scrolling: touch; scroll-behavior: smooth; }";
  html += ".wifi-list:empty { display: none; }";
  // 滚动条美化 (webkit)
  html += ".wifi-list::-webkit-scrollbar { width: 4px; }";
  html += ".wifi-list::-webkit-scrollbar-track { background: transparent; }";
  html += ".wifi-list::-webkit-scrollbar-thumb { background: rgba(148,163,184,0.3); border-radius: 2px; }";
  html += ".wifi-list::-webkit-scrollbar-thumb:hover { background: rgba(148,163,184,0.5); }";
  html += ".wifi-item { padding: 12px 14px; cursor: pointer; border-bottom: 1px solid rgba(148,163,184,0.1);";
  html += " transition: background 0.15s; display: flex; justify-content: space-between; align-items: center;";
  html += " -webkit-tap-highlight-color: rgba(59,130,246,0.2); touch-action: manipulation; }";
  html += ".wifi-item:last-child { border-bottom: none; }";
  html += ".wifi-item:hover { background: rgba(59,130,246,0.1); }";
  html += ".wifi-name { color: #e2e8f0; font-size: 0.9em; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }";
  html += ".wifi-signal { display: flex; gap: 2px; align-items: flex-end; }";
  html += ".wifi-bar { width: 3px; background: #475569; border-radius: 1px; }";
  html += ".wifi-bar.active { background: #4ade80; }";
  html += ".wifi-loading { padding: 12px 14px; text-align: center; color: #94a3b8; font-size: 0.85em; }";
  html += ".wifi-error { padding: 12px 14px; text-align: center; color: #ef5350; font-size: 0.85em; }";
  html += ".wifi-lock { font-size: 0.75em; margin-right: 4px; }";
  html += "</style></head><body>";
  html += "<div class='card'>";
  html += "<div class='header'>";
  html += "<div class='logo'>📡</div>";
  html += "<div class='title'>Art-Net to DMX Converter</br>WiFi Configuration</div>";
  html += "<div class='subtitle'>Enter your WiFi credentials to connect</div>";
  html += "<div class='hint'>Visit <a href='http://artnet.local' style='color:#60a5fa;'>http://artnet.local</a> to get Art-Net IP and port</div>";
  html += "</div>";

  if (message.length() > 0)
  {
    html += "<div class='msg'>" + message + "</div>";
  }

  html += "<form action='/save' method='POST'>";
  html += "<div class='form-group'>";
  html += "<label>WiFi Name (SSID)</label>";
  html += "<div style='display:flex;gap:8px;'>";
  html += "<input type='text' id='ssidInput' name='ssid' placeholder='Enter WiFi name' required style='flex:1;'>";
  html += "<button type='button' class='btn-scan' onclick='scanWiFi()' id='scanBtn' title='Scan nearby WiFi networks'>🔍</button>";
  html += "</div>";
  html += "<div id='wifiList' class='wifi-list'></div>";
  html += "</div>";
  html += "<div class='form-group'>";
  html += "<label>WiFi Password</label>";
  html += "<input type='password' name='password' placeholder='Enter WiFi password'>";
  html += "<div class='hint'>Leave empty for open networks</div>";
  html += "</div>";
  html += "<button type='submit' class='btn'>Connect</button>";
  html += "</form>";
  html += "<div class='footer'>LED-BUG Art-Net to DMX Converter<br>@ Made By Ben · <a href='https://led-bug.com/' target='_blank'>LED-BUG.com</a></div>";
  html += "</div>";
  // JavaScript for WiFi scanning
  html += "<script>";
  html += "function scanWiFi() {";
  html += "  var btn = document.getElementById('scanBtn');";
  html += "  var list = document.getElementById('wifiList');";
  html += "  btn.disabled = true;";
  html += "  btn.textContent = '⏳';";
  html += "  list.innerHTML = '<div class=\"wifi-loading\">Scanning for nearby WiFi networks...</div>';";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/scan', true);";
  html += "  xhr.timeout = 15000;";
  html += "  xhr.onload = function() {";
  html += "    btn.disabled = false;";
  html += "    btn.textContent = '🔍';";
  html += "    if (xhr.status === 200) {";
  html += "      var data = JSON.parse(xhr.responseText);";
  html += "      if (data.networks && data.networks.length > 0) {";
  html += "        var listHtml = '';";
  html += "        for (var i = 0; i < data.networks.length; i++) {";
  html += "          var net = data.networks[i];";
  html += "          var bars = getSignalBars(net.rssi);";
  html += "          var lock = net.secure ? '🔒 ' : '';";
  html += "          listHtml += '<div class=\"wifi-item\" data-ssid=\"' + escapeHtml(net.ssid) + '\" onclick=\"selectWiFi(this)\">';";
  html += "          listHtml += '<span class=\"wifi-name\">' + lock + escapeHtml(net.ssid) + '</span>';";
  html += "          listHtml += '<span class=\"wifi-signal\">' + bars + '</span>';";
  html += "          listHtml += '</div>';";
  html += "        }";
  html += "        list.innerHTML = listHtml;";
  html += "      } else {";
  html += "        list.innerHTML = '<div class=\"wifi-loading\">No WiFi networks found</div>';";
  html += "      }";
  html += "    } else {";
  html += "      list.innerHTML = '<div class=\"wifi-error\">Scan failed. Please try again.</div>';";
  html += "    }";
  html += "  };";
  html += "  xhr.onerror = function() {";
  html += "    btn.disabled = false;";
  html += "    btn.textContent = '🔍';";
  html += "    list.innerHTML = '<div class=\"wifi-error\">Scan failed. Please try again.</div>';";
  html += "  };";
  html += "  xhr.ontimeout = function() {";
  html += "    btn.disabled = false;";
  html += "    btn.textContent = '🔍';";
  html += "    list.innerHTML = '<div class=\"wifi-error\">Scan timed out. Please try again.</div>';";
  html += "  };";
  html += "  xhr.send();";
  html += "}";
  html += "function selectWiFi(el) {";
  html += "  var ssid = el.getAttribute('data-ssid');";
  html += "  document.getElementById('ssidInput').value = ssid;";
  html += "  document.getElementById('wifiList').innerHTML = '';";
  html += "  var pwdInput = document.querySelector('input[name=\"password\"]');";
  html += "  if (pwdInput) pwdInput.focus();";
  html += "}";
  html += "function getSignalBars(rssi) {";
  html += "  var bars = '';";
  html += "  var level = rssi >= -50 ? 4 : rssi >= -60 ? 3 : rssi >= -70 ? 2 : 1;";
  html += "  for (var i = 1; i <= 4; i++) {";
  html += "    var h = 4 + i * 3;";
  html += "    var active = i <= level ? ' active' : '';";
  html += "    bars += '<span class=\"wifi-bar' + active + '\" style=\"height:' + h + 'px;\"></span>';";
  html += "  }";
  html += "  return bars;";
  html += "}";
  html += "function escapeHtml(str) {";
  html += "  return str.replace(/[&<>\"]/g, function(m) {";
  html += "    return {'&':'&amp;','<':'&lt;','>':'&gt;','\"':'&quot;'}[m];";
  html += "  });";
  html += "}";
  html += "</script>";
  html += "</body></html>";

  return html;
}

String getSuccessPage(String ip, String ssidName)
{
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>WiFi Connected</title>";
  html += "<style>";
  html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
  html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;";
  html += " background: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);";
  html += " min-height: 100vh; color: #e2e8f0; padding: 20px; display: flex;";
  html += " justify-content: center; align-items: center; }";
  html += ".card { max-width: 420px; width: 100%; background: rgba(30, 41, 59, 0.6);";
  html += " border: 1px solid rgba(148, 163, 184, 0.1); border-radius: 16px;";
  html += " padding: 32px 28px; backdrop-filter: blur(10px); text-align: center; }";
  html += ".icon { font-size: 4em; margin-bottom: 12px; }";
  html += ".title { font-size: 1.4em; font-weight: 600; color: #4ade80; margin-bottom: 8px; }";
  html += ".info { color: #94a3b8; font-size: 0.9em; margin-bottom: 6px; }";
  html += ".value { color: #f1f5f9; font-weight: 600; font-size: 1.1em; }";
  html += ".mono { font-family: 'SF Mono', Consolas, 'Courier New', monospace; }";
  html += ".highlight { color: #60a5fa; }";
  html += ".divider { height: 1px; background: rgba(148, 163, 184, 0.15); margin: 16px 0; }";
  html += ".footer { margin-top: 20px; color: #475569; font-size: 0.78em; }";
  html += ".note { color: #64748b; font-size: 0.82em; margin-top: 14px; }";
  html += "</style></head><body>";
  html += "<div class='card'>";
  html += "<div class='icon'>✅</div>";
  html += "<div class='title'>WiFi Connected!</div>";
  html += "<div class='info'>Connected to</div>";
  html += "<div class='value'>" + ssidName + "</div>";
  html += "<div class='divider'></div>";
  html += "<div class='info'>Device IP Address</div>";
  html += "<div class='value mono highlight' style='font-size:1.3em;'>" + ip + "</div>";
  html += "<div class='note'>You can now close this page.<br>Visit <b>http://artnet.local</b> on the same network.</div>";
  html += "<div class='footer'>LED-BUG Art-Net to DMX Converter · Made By Ben</div>";
  html += "</div></body></html>";

  return html;
}

// ==================== Web 配网处理函数 ====================
void handleConfigRoot()
{
  portalServer->send(200, "text/html", getConfigPage());
}

// WiFi扫描接口 - 返回附近WiFi网络列表 (JSON)
void handleWiFiScan()
{
  Serial.println("🔍 Scanning WiFi networks...");

  // WiFi.scanNetworks() 会阻塞，返回扫描到的网络数量
  int n = WiFi.scanNetworks();

  String json = "{\"networks\":[";
  for (int i = 0; i < n; i++)
  {
    if (i > 0) json += ",";
    String ssid = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    bool secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;

    // 转义SSID中的特殊字符 (JSON安全)
    ssid.replace("\\", "\\\\");
    ssid.replace("\"", "\\\"");

    json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(rssi) + ",\"secure\":" + (secure ? "true" : "false") + "}";
  }
  json += "]}";

  Serial.printf("🔍 Found %d networks\n", n);
  portalServer->send(200, "application/json", json);
}

void handleConfigSave()
{
  String newSsid = portalServer->arg("ssid");
  String newPass = portalServer->arg("password");

  Serial.println("\n📝 Received WiFi config:");
  Serial.print("  SSID: ");
  Serial.println(newSsid);
  Serial.print("  Password: ");
  Serial.println(newPass.length() > 0 ? newPass : "(open)");

  // 尝试连接WiFi (AP_STA模式: AP保持活跃，方便失败后重试)
  WiFi.disconnect();
  delay(200);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(newSsid.c_str(), newPass.length() > 0 ? newPass.c_str() : nullptr);

  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\n✅ WiFi 连接成功!");
    Serial.print("📡 IP 地址: ");
    Serial.println(WiFi.localIP());

    // 发送成功页面
    portalServer->send(200, "text/html", getSuccessPage(WiFi.localIP().toString(), newSsid));
    delay(500);

    // 关闭配网门户
    configPortalActive = false;
  }
  else
  {
    Serial.println("\n❌ WiFi 连接失败");
    WiFi.disconnect();
    delay(200);

    // 发送错误页面，让用户重试
    portalServer->send(200, "text/html", getConfigPage("Connection failed. Please check your credentials and try again.", true));
  }
}

void handleConfigNotFound()
{
  //  captive portal: 所有未知请求重定向到配网页面
  portalServer->sendHeader("Location", "http://192.168.4.1/", true);
  portalServer->send(302, "text/plain", "");
}

// ==================== AP 配网门户 ====================
void startConfigPortal()
{
  Serial.println("\n📡 Starting WiFi Config Portal...");

  // 启动AP模式
  WiFi.mode(WIFI_AP);
  delay(200);
  WiFi.softAP(AP_SSID); // 无密码的开放热点
  delay(200);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));

  Serial.print("📡 AP started: ");
  Serial.println(AP_SSID);
  Serial.print("📡 AP IP: ");
  Serial.println(AP_IP);

  // 启动DNS服务器 (captive portal)
  initDNS();

  // 使用独立的WebServer，避免handler污染主server
  portalServer = new WebServer(80);
  portalServer->on("/", handleConfigRoot);
  portalServer->on("/scan", HTTP_GET, handleWiFiScan);
  portalServer->on("/save", HTTP_POST, handleConfigSave);
  portalServer->onNotFound(handleConfigNotFound);
  portalServer->begin();
  Serial.println("✅ Config web server started (独立实例)");

  // LED快闪指示进入配网模式
  blinkLED(LED_PIN, 3, 200);
  digitalWrite(LED_PIN, HIGH); // LED常亮表示配网模式

  configPortalActive = true;

  // 配网循环 — 直到连接成功
  Serial.println("\n⏳ Waiting for WiFi configuration...");
  Serial.println("   Connect to '" + String(AP_SSID) + "' and visit http://192.168.4.1");

  unsigned long portalStartTime = millis();
  while (configPortalActive)
  {
    dnsServer.processNextRequest();
    portalServer->handleClient();
    delay(10);

    // 每1分钟打印一次提示，表示配网门户仍在运行
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 60000)
    {
      unsigned long mins = (millis() - portalStartTime) / 60000;
      Serial.printf("\n⏳ Config portal still active... (%lu min)\n", mins);
      lastHeartbeat = millis();
    }
  }

  // 连接成功，关闭配网门户
  Serial.println("🔒 Closing config portal...");
  portalServer->stop();
  delete portalServer;
  portalServer = nullptr;
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  delay(200);

  digitalWrite(LED_PIN, LOW); // LED灭表示连接成功
  Serial.println("✅ Config portal closed");
}

// ==================== WiFi 配网连接函数 ====================
void connectWIFI(int timeOut_s)
{
  WiFi.hostname(HOST_NAME); // 设置设备名
  WiFi.mode(WIFI_STA);

  // ---- 尝试已保存的WiFi凭据 (NVS) ----
  WiFi.begin();
  Serial.println("Trying saved WiFi credentials...");

  // 在 timeOut_s 秒内等待连接
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // LED闪烁表示正在连接
    delay(500);

    // 超时 → 启动配网门户
    if (millis() - startTime >= (unsigned long)timeOut_s * 1000)
    {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("\n❌ WiFi connection timeout (" + String(timeOut_s) + "s). Starting config portal...");
      WiFi.disconnect();
      delay(200);
      startConfigPortal();
      return;
    }
  }

  // ---- WiFi 连接成功 ----
  WiFi.setAutoReconnect(true); // 断线后自动重连
  Serial.println("\n✅ WiFi 已连接");
  Serial.print("📡 IP 地址: ");
  Serial.println(WiFi.localIP());
  Serial.print("📶 SSID: ");
  Serial.println(WiFi.SSID());
  digitalWrite(LED_PIN, LOW); // LED灭表示连接成功
#ifdef DHCP_DISABLED
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
    Serial.println("STA Failed to configure");
  }
  else
  {
    Serial.print("🔒 Static IP: ");
    Serial.println(local_IP);
  }
#endif
}

/*
 * 删除保存的wifi信息，这里的删除是删除存储在flash的信息。删除后wifi读不到上次连接的记录，需重新配网
 */
void restoreWiFi()
{
  esp_wifi_restore(); // 删除保存的wifi信息
  Serial.println("连接信息已清空,准备重启设备...");
  delay(10);
  blinkLED(LED_PIN, 5, 200);  // LED闪烁5次,指示LED，不需要可删除
  digitalWrite(LED_PIN, LOW); // 指示LED，不需要可删除
}

// 以下为BOOT按键长按3秒后重置网络
static unsigned long buttonPressTime = 0; // 记录按钮按下时间
static bool buttonPressed = false;        // 记录按钮状态
static bool buttonHandled = false;        // 防止重复处理
void resetWiFibtn()
{
  // 检查按钮状态
  if (digitalRead(BOOT_PIN) == LOW)
  { // 按钮被按下（因为外部上拉，按下时为低电平）
    if (!buttonPressed)
    { // 如果按钮之前未被按下
      buttonPressed = true;
      buttonPressTime = millis();             // 记录按下时间
      Serial.println("RESET Button pressed"); // 调试信息
    }
  }
  else
  { // 按钮被释放
    if (buttonPressed)
    { // 如果按钮之前是按下状态
      buttonPressed = false;
      buttonHandled = false;                   // 重置处理标志
      Serial.println("RESET Button released"); // 调试信息
    }
  }

  // 检查是否长按超过3秒
  if (buttonPressed && !buttonHandled && (millis() - buttonPressTime >= 3000))
  {
    buttonHandled = true; // 标记已处理
    Serial.println("\n按键已长按3秒,正在清空网络连保存接信息。");
    restoreWiFi(); // 删除保存的wifi信息
    ESP.restart(); // 重启复位esp32
    Serial.println("已重启设备。");
  }
}
