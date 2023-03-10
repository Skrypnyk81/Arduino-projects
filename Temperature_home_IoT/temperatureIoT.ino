#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT11

#define AWS_IOT_PUBLISH_TOPIC    "ESPHomeTemp/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC  "ESPHomeTemp/sub"

float h;
float t;

DHT dht(DHTPIN, DHTTYPE);

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

unsigned long previousMillis = 0;
const long interval = 900000; // 15 minutes in milliseconds

BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);

void connectAWS() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);

  client.setServer(MQTT_HOST, 8883);

  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(1000);
  }

  if (!client.connected()) {
    Serial.println("AWS IOT Timeout!");
    return;
  }
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IOT Connected!");
}


void publishMessage(float temperature, float humidity) {
  char payload[128];
  sprintf(payload, "{\"temperature\":%.2f,\"humidity\":%.2f}", temperature, humidity);
  client.publish(AWS_IOT_PUBLISH_TOPIC, payload);
}

void setup()
{
  Serial.begin(9600);
  dht.begin();
  connectAWS();
}


void loop () {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read temperature and humidity from DHT11 sensor");
      return;
    }
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    if (!client.connected()) {
      connectAWS();
    }
    publishMessage(temperature, humidity);
  }
  client.loop();
}
