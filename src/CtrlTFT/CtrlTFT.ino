#include "Wire.h"
#include "Board_Pinout.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "NS2009.h"
#include "ETH.h"
#include "PubSubClient.h"

#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 12

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
NS2009 touchscreen(false, true);
WiFiClient ethClient;
PubSubClient mqttClient(ethClient);

const char* mqttServer = "172.22.35.45";
const int mqttPort = 1883;

bool lightIsOn = false;
bool previousLightIsOn;
bool previousMqttClientConnected;



#define WERKSTATT 1

#ifdef KUECHE
const char* mqttClientId = "KuecheTFT";
const char* mqttTopicRead = "CtrlTFT/Licht/Kueche/status";
const char* mqttTopicWrite = "CtrlTFT/Licht/Kueche/set";
#endif

#ifdef BESPRECHUNG
const char* mqttClientId = "BesprechungTFT";
const char* mqttTopicRead = "CtrlTFT/Licht/Besprechungszimmer/Vorne/status";
const char* mqttTopicWrite = "CtrlTFT/Licht/Besprechungszimmer/Vorne/set";
const char* mqttTopicWrite2 = "CtrlTFT/Licht/Besprechungszimmer/Hinten/set";
#endif

#ifdef WERKSTATT
const char* mqttClientId = "WerkstattTFT";
const char* mqttTopicRead = "CtrlTFT/Licht/Werkstatt/status";
const char* mqttTopicWrite = "CtrlTFT/Licht/Werkstatt/set";
#endif



void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(TFT_DC, OUTPUT);
  tft.begin();
  display();

  WiFi.onEvent(ethEvent); //heißt zwar WiFi, ist aber ETH
  ETH.begin();

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
}


void ethEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH started");
      ETH.setHostname(mqttClientId);
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex())
        Serial.print(", FULL_DUPLEX");
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println(" Mbps");
      mqttReconnect();
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH disconnected");
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH stopped");
      break;
    default:
      break;
  }
}


void mqttReconnect() {
  while (!mqttClient.connected()) {
    display();
    Serial.print("Attempting MQTT connection... ");
    if (mqttClient.connect(mqttClientId)) {
      Serial.println("connected");
      mqttClient.subscribe(mqttTopicRead);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(", retrying in 5 seconds...");
      delay(5000);
    }
  }
}


void display() {
  // only redraw if something changed
  if (lightIsOn != previousLightIsOn || mqttClient.connected() != previousMqttClientConnected) {
    previousLightIsOn = lightIsOn;
    previousMqttClientConnected = mqttClient.connected();

    tft.fillScreen(lightIsOn ? ILI9341_BLACK : ILI9341_WHITE);
    tft.setTextColor(lightIsOn ? ILI9341_WHITE : ILI9341_BLACK);

    tft.setTextSize(10);
    if (lightIsOn)
      tft.setCursor(35, 125);
    else
      tft.setCursor(65, 125);
    tft.println(lightIsOn ? "OFF" : "ON");

    tft.setCursor(2, 311);
    tft.setTextSize(1);
    tft.printf("MQTT %sconnected", !mqttClient.connected() ? "not " : "");
    tft.setCursor(216, 311);
    tft.printf("v1.1");
  }
}


void mqttCallback(char* topic, byte* messageBytes, unsigned int length) {
  Serial.print(topic);
  Serial.print(": ");
  String message;

  for (int i = 0; i < length; i++)
    message += (char) messageBytes[i];
  Serial.println(message);

  if (String(topic) == mqttTopicRead) {
    lightIsOn = message == "true";
    display();
  }
}


void mqttPublish(bool state) {
  Serial.println("Publishing new state");
  mqttClient.publish(mqttTopicWrite, state ? "true" : "false");

#ifdef BESPRECHUNG
  mqttClient.publish(mqttTopicWrite2, state ? "true" : "false");
#endif
}


void loop() {
  if (!mqttClient.connected())
    mqttReconnect();
  mqttClient.loop();

  if (touchscreen.CheckTouched()) {
    mqttPublish(lightIsOn = !lightIsOn);
    delay(200);
  }

  delay(50);
}
