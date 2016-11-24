#include <SPI.h>
#include <ArduinoJson.h>
#include <WiFi101.h>

#include <RTCZero.h>
#include "time.h"
#include "ssid.h"

RTCZero rtc;

char hostname[] = "";
char authSAS[] = "";

String hubName = "";
String deviceName = "";

String uri = "/devices/" + deviceName +"/messages/events?api-version=2016-02-03";

int status = WL_IDLE_STATUS;
WiFiSSLClient client;

void setup() {
  Serial.begin(9600);
  Serial.println("DHT22 Sensor for Microsoft Azure");

  // check for the presence of the shield :
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true); // stop
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect...");

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    if (status != WL_CONNECTED) {
      // wait 10 seconds for connection:
      delay(10000);
    }
  }
  Serial.println("Connected to wifi");

  rtc.begin();
  setTimeUsingTimeServer(rtc);
}

void loop() {
  float temperature = 100;
  float humidity = 50;

  Serial.print(temperature);
  Serial.print(" *F ");
  Serial.print(humidity);
  Serial.println(" %");

  if (!(isnan(temperature) || isnan(humidity))) {
    sendEvent(deviceName, temperature, humidity); 
  }

  delay(10000);
}

void sendEvent(String deviceName, float temperature, float humidity) {

  String json = createJSON(deviceName, temperature, humidity);
  httpPost(json);

  String response = "";
  char c;
  while (client.available()) {
    c = client.read();
    response.concat(c);
  }

  if (!response.equals("")) {
    if (response.startsWith("HTTP/1.1 204")) {
      Serial.println("Data sent to Azure"); 
    } else {
      Serial.println("Error posting " + json);
      Serial.println(response);
    }
  }

}

String createJSON(String deviceName, float temperature, float humidity) {
  char dateString[25];
  sprintf(dateString, "2%03d-%02d-%02dT%02d:%02d:%02d.000Z", rtc.getYear(), rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());

   
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["deviceId"] = deviceName;
  root["temperature"] = String(temperature);
  root["humidity"] = String(humidity);

  char json[200];
  root.printTo(json, sizeof(json));
  return String(json);
}

void httpPost(String content)
{
    // close any connection before send a new request.
    client.stop();
  
    if (client.connectSSL(hostname, 443)) {
      client.print("POST ");
      client.print(uri);
      client.println(" HTTP/1.1"); 
      client.print("Host: "); 
      client.println(hostname);
      client.print("Authorization: ");
      client.println(authSAS); 
      client.println("Connection: close");

      client.print("Content-Type: ");
      client.println("application/json");
      client.print("Content-Length: ");
      client.println(content.length());
      client.println();
      client.println(content);
      
      delay(200);
      
    } else {
      Serial.println("Connection failed");
    }
}
