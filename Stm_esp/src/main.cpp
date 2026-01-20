#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

String input = "";
WiFiClient client;
WiFiUDP udp;
bool isUDPActive = false;

// =========================================================
//                    BASIC UTILITY FUNCTIONS
// =========================================================
void sendOK(){ Serial.println("OK"); }
void ERROR() { Serial.println("ERROR"); }

// =========================================================
//                    AT BASIC COMMANDS
// =========================================================`
void at_test() {
  Serial.println("OK");
}

void at_restart() {
  Serial.println("RESTARTING");
  delay(1000);
  ESP.restart();
}

void at_gmr() {
  Serial.println("ESP32 AT CUSTOM FIRMWARE v1.0");
}

// =========================================================
//                    WIFI COMMANDS
// =========================================================

// AT+CWJAP="ssid","pass"
void at_joinAP(String cmd) {
  cmd.replace("AT+CWJAP=", "");
  cmd.replace("\"", "");

  int comma = cmd.indexOf(',');
  if (comma < 0) return ERROR();

  String ssid = cmd.substring(0, comma);
  String pass = cmd.substring(comma + 1);

  Serial.println("WIFI CONNECTING...");

  WiFi.begin(ssid.c_str(), pass.c_str());

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWIFI CONNECTED");
    Serial.print("IP:");
    Serial.println(WiFi.localIP());
    sendOK();
  } else {
    ERROR();
  }
}

// AT+CWQAP
void at_quitAP() {
  WiFi.disconnect();
  Serial.println("WIFI DISCONNECTED");
  sendOK();
}

// AT+CWSCAN
void at_scan() {
  Serial.println("+SCAN:");

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    Serial.print(i);
    Serial.print(") ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" | RSSI=");
    Serial.println(WiFi.RSSI(i));
  }

  sendOK();
}

// AT+CIPSTA?
void at_getIP() {
  Serial.print("+CIPSTA: ");
  Serial.println(WiFi.localIP());
  sendOK();
}

// AT+CIPSTAMAC?
void at_getMAC() {
  Serial.print("+CIPSTAMAC:");
  Serial.println(WiFi.macAddress());
  sendOK();
}

// =========================================================
//                        TCP COMMANDS
// =========================================================

// AT+CIPSTART="TCP","IP",PORT
void at_tcp_start(String cmd) {
  cmd.replace("AT+CIPSTART=", "");
  cmd.replace("\"", "");

  int c1 = cmd.indexOf(',');
  int c2 = cmd.lastIndexOf(',');

  if (c1 < 0 || c2 < 0) return ERROR();

  String proto = cmd.substring(0, c1);
  String ip = cmd.substring(c1 + 1, c2);
  int port = cmd.substring(c2 + 1).toInt();

  if (proto != "TCP") return ERROR();

  isUDPActive = false;
  if (client.connect(ip.c_str(), port)) {
    Serial.println("CONNECT");
    sendOK();
  } else {
    ERROR();
  }
}

// AT+CIPSEND=LEN
int expectedLength = -1;
void at_cipsend(String cmd) {
  cmd.replace("AT+CIPSEND=", "");
  expectedLength = cmd.toInt();
  Serial.println(">");
}

// When STM32 sends raw data after CIPSEND
void tcp_receive_data(String data) {
  if (expectedLength <= 0) return;
  
  if (isUDPActive) {
    // Send via UDP
    udp.print(data);
    if (udp.endPacket()) {
      Serial.println("SEND OK");
    } else {
      Serial.println("SEND FAIL");
    }
  } else {
    // Send via TCP
    client.print(data);
    Serial.println("SEND OK");
  }
  
  expectedLength = -1;
}

// AT+CIPCLOSE
void at_close() {
  client.stop();
  isUDPActive = false;
  Serial.println("CLOSED");
  sendOK();
}

// =========================================================
//                        UDP COMMANDS
// =========================================================

// AT+CIPSTART="UDP","IP",PORT
void at_udp_start(String cmd) {
  cmd.replace("AT+CIPSTART=", "");
  cmd.replace("\"", "");

  int c1 = cmd.indexOf(',');
  int c2 = cmd.lastIndexOf(',');

  if (c1 < 0 || c2 < 0) return ERROR();

  String proto = cmd.substring(0, c1);
  String ip = cmd.substring(c1 + 1, c2);
  int port = cmd.substring(c2 + 1).toInt();

  if (proto != "UDP") return ERROR();

  isUDPActive = true;
  udp.beginPacket(ip.c_str(), port);
  Serial.println("UDP CONNECTED");
  sendOK();
}

// AT+CIPSEND=LEN for UDP is same

// AT+CIPCLOSE for UDP
void at_udp_close() {
  udp.stop();
  isUDPActive = false;
  Serial.println("UDP CLOSED");
  sendOK();
}

// =========================================================
//                          HTTP COMMANDS
// =========================================================

// AT+HTTPGET="http://yoururl.com"
void at_http_get(String cmd) {
  cmd.replace("AT+HTTPGET=", "");
  cmd.replace("\"", "");

  HTTPClient http;
  http.begin(cmd);

  int code = http.GET();
  Serial.print("+HTTP_CODE:");
  Serial.println(code);

  if (code > 0) {
    String payload = http.getString();
    Serial.println("+HTTP_DATA:");
    Serial.println(payload);
  }

  http.end();
  sendOK();
}

// AT+HTTPPOST="url","data"
void at_http_post(String cmd) {
  cmd.replace("AT+HTTPPOST=", "");
  cmd.replace("\"", "");

  int comma = cmd.indexOf(',');
  if (comma < 0) return ERROR();

  String url = cmd.substring(0, comma);
  String data = cmd.substring(comma + 1);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int code = http.POST(data);
  Serial.print("+HTTP_CODE:");
  Serial.println(code);

  if (code > 0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();
  sendOK();
}

// =========================================================
//                     MAIN COMMAND PARSER
// =========================================================
void parseAT(String cmd) {
  cmd.trim();

  // BASIC
  if (cmd == "AT") return at_test();
  if (cmd == "AT+RST") return at_restart();
  if (cmd == "AT+GMR") return at_gmr();

  // WIFI
  if (cmd.startsWith("AT+CWJAP=")) return at_joinAP(cmd);
  if (cmd == "AT+CWQAP") return at_quitAP();
  if (cmd == "AT+CWSCAN") return at_scan();
  if (cmd == "AT+CIPSTA?") return at_getIP();
  if (cmd == "AT+CIPSTAMAC?") return at_getMAC();

  // TCP / UDP
  if (cmd.startsWith("AT+CIPSTART=")) {
    if (cmd.indexOf("TCP") > 0) return at_tcp_start(cmd);
    if (cmd.indexOf("UDP") > 0) return at_udp_start(cmd);
  }

  if (cmd.startsWith("AT+CIPSEND=")) return at_cipsend(cmd);
  if (cmd == "AT+CIPCLOSE") {
    if (client.connected()) return at_close();
    else return at_udp_close();
  }

  // HTTP
  if (cmd.startsWith("AT+HTTPGET=")) return at_http_get(cmd);
  if (cmd.startsWith("AT+HTTPPOST=")) return at_http_post(cmd);

  // RAW TCP DATA
  if (expectedLength > 0) return tcp_receive_data(cmd);

  Serial.println("ERROR:UNKNOWN");
}

// =========================================================
//                     MAIN LOOP
// =========================================================
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 FULL AT FIRMWARE READY");
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      parseAT(input);
      input = "";
    } else {
      input += c;
    }
  }
}
