#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
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

unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
const long interval = 5000;





BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);


time_t now;
time_t nowish = 1510592825;

void NTPConnect(void)
{
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 3600, 0 * 3600, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < nowish)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}


void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  NTPConnect();
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


void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["humidity"] = h;
  doc["temperature"] = t;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void setup()
{
  Serial.begin(9600);
  connectAWS();
  dht.begin();
}

int mytimer (int timer1){
        static unsigned long t1, dt;
        int ret = 0;
        dt = millis() - t1;
        if (dt >= timer1) {
            t1 = millis();
            ret = 1;
        }
        return ret;
    } 

void loop ()
{
  if (mytimer(900000)){
      h = dht.readHumidity();
      t = dht.readTemperature();

      if (isnan(h) || isnan(t)) // Check if any reads failed and exit early (to try again).
      {
          Serial.println(F("Failed to read from DHT sensor!"));
          return;
      }

      Serial.print(" Time: ");
      Serial.print(now);
      Serial.print(F(" Humidity: "));
      Serial.print(h);
      Serial.print(F("% Temperature: "));
      Serial.print(t);
      Serial.println(F("°C "));

      now = time(nullptr);

      if (!client.connected())
      {
          connectAWS();
      } else 
      {
          client.loop();
          if (millis() - lastMillis > 5000)
          {
          lastMillis = millis();
          publishMessage();
          }
      }
  }
}