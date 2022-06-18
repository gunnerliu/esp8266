#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <ESP8266HTTPClient.h>

#ifndef STASSID
#define STASSID ""
#define STAPSK  ""
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
String URL = "http://192.168.31.106/api/hk/iots/heartBeat";
String iotCode = "light_one";
String deviceType = "LIGHT";

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
// 创建定时器
Ticker ticker;

// 控制继电器的 GPIO
int pin = 0;

void setup() {
  Serial.begin(115200);

  // 继电器的初始值设为低电平，低电平即吸合继电器
  pinMode(pin, OUTPUT);
  digitalWrite(pin, 1);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));

  // Start the server
  server.begin();
  Serial.println(F("Server started"));

  // Print the IP address
  Serial.println(WiFi.localIP());
  // 5 秒上报一次状态
  ticker.attach(5, heart);
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  Serial.println(F("new client"));

  client.setTimeout(5000); // default is 1000

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(F("request: "));
  Serial.println(req);

  // Match the request
  int val;
  if (req.indexOf(F("/gpio/0")) != -1) {
    val = 0;
  } else if (req.indexOf(F("/gpio/1")) != -1) {
    val = 1;
  } else {
    Serial.println(F("invalid request"));
    val = digitalRead(pin);
  }

  // Set LED according to the request
  digitalWrite(pin, val);

  // read/ignore the rest of the request
  // do not client.flush(): it is for output only, see below
  while (client.available()) {
    // byte by byte is not very efficient
    client.read();
  }

  // Send the response to the client
  // it is OK for multiple small client.print/write,
  // because nagle algorithm will group them into one single packet
  client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now "));
  client.print((val) ? F("high") : F("low"));
  client.print(F("<br><br>Click <a href='http://"));
  client.print(WiFi.localIP());
  client.print(F("/gpio/1'>here</a> to switch LED GPIO on, or <a href='http://"));
  client.print(WiFi.localIP());
  client.print(F("/gpio/0'>here</a> to switch LED GPIO off.</html>"));

  // The client will actually be *flushed* then disconnected
  // when the function returns and 'client' object is destroyed (out-of-scope)
  // flush = ensure written data are received by the other side
  Serial.println(F("Disconnecting from client"));
}

void heart(){
//  String heartStr = "iotCode: " + iotCode + ", 设备类型: " + deviceType + ", 心跳检测, IP地址: " +  WiFi.localIP().toString() + F(", 开关值为: ") + digitalRead(pin);
//  Serial.println(heartStr);
  //创建 HTTPClient 对象
  HTTPClient httpClient;
  URL = URL + "?iotCode=" + iotCode + "&ipAddr=" + WiFi.localIP().toString() + "&deviceType=" + deviceType + "&switchValue=" + digitalRead(pin);
  Serial.println(URL);
  httpClient.begin(URL);
  httpClient.GET();
  //关闭ESP8266与服务器连接
  httpClient.end();
}
