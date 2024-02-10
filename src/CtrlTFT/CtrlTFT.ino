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

bool firstDraw = true;
bool lightIsOn;
bool previousLightIsOn;
bool lightIsOn2;
bool previousLightIsOn2;
bool previousMqttClientConnected;

char* mqttClientId;
char* mqttTopicRead;
char* mqttTopicWrite;
char* mqttTopicRead2;
char* mqttTopicWrite2;



void mqttSelfSetup() {
  String macAddress = ETH.macAddress();

  if (macAddress == "E0:5A:1B:6E:3F:A7") {
    mqttClientId = "WerkstattTFT";
    mqttTopicRead = "CtrlTFT/Licht/Werkstatt/status";
    mqttTopicWrite = "CtrlTFT/Licht/Werkstatt/set";
  } else if (macAddress == "40:22:D8:15:CB:CF") {
    mqttClientId = "BueroTFT";
    mqttTopicRead = "CtrlTFT/Licht/Buero/status";
    mqttTopicWrite = "CtrlTFT/Licht/Buero/set";
  } else if (macAddress == "E0:5A:1B:6E:40:FB") {
    mqttClientId = "KuecheTFT";
    mqttTopicRead = "CtrlTFT/Licht/Kueche/OnOff/status";
    mqttTopicWrite = "CtrlTFT/Licht/Kueche/OnOff/set";
  } else if (macAddress == "40:22:D8:15:CB:C7") {
    mqttClientId = "BesprechungTFT";
    mqttTopicRead = "CtrlTFT/Licht/Besprechungszimmer/Vorne/OnOff/status";
    mqttTopicWrite = "CtrlTFT/Licht/Besprechungszimmer/Vorne/OnOff/set";
    mqttTopicRead2 = "CtrlTFT/Licht/Besprechungszimmer/Hinten/OnOff/status";
    mqttTopicWrite2 = "CtrlTFT/Licht/Besprechungszimmer/Hinten/OnOff/set";
  }

  if (mqttClientId) {
    Serial.print("MQTT self setup: mqttClientId = ");
    Serial.println(mqttClientId);
  } else {
    Serial.print("MQTT self setup failed for ETH MAC ");
    Serial.println(ETH.macAddress());
  }
}


void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(TFT_DC, OUTPUT);
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  display();

  WiFi.onEvent(ethEvent); // heißt zwar WiFi, ist aber Ethernet
  ETH.begin();
  
  mqttSelfSetup();

  if (mqttClientId != NULL) {
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(mqttCallback);
  }
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
  while (!mqttClient.connected() && mqttClientId != NULL) {
    display();
    Serial.print("Attempting MQTT connection... ");
    if (mqttClient.connect(mqttClientId)) {
      Serial.print("connected, got IP ");
      Serial.println(ETH.localIP());
      mqttClient.subscribe(mqttTopicRead);
      if (mqttClientId == "BesprechungTFT")
        mqttClient.subscribe(mqttTopicRead2);
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
  if (lightIsOn != previousLightIsOn || lightIsOn2 != previousLightIsOn2 || mqttClient.connected() != previousMqttClientConnected || firstDraw) {
    firstDraw = false;
    previousLightIsOn = lightIsOn;
    previousLightIsOn2 = lightIsOn2;
    previousMqttClientConnected = mqttClient.connected();

    tft.fillScreen(lightIsOn ? ILI9341_BLACK : ILI9341_WHITE);
    tft.setTextColor(lightIsOn ? ILI9341_WHITE : ILI9341_BLACK);

    if (mqttClient.connected()) {
      if (mqttClientId == "BesprechungTFT") {
        tft.setCursor(6, 6);
        tft.setTextSize(3);
        tft.println("Vorne");
        if (lightIsOn)
          tft.setCursor(35, 45);
        else
          tft.setCursor(65, 45);
        tft.setTextSize(10);
        tft.println(lightIsOn ? "OFF" : "ON");

        tft.fillRect(0, 160, 240, 160, lightIsOn2 ? ILI9341_BLACK : ILI9341_WHITE);
        tft.setTextColor(lightIsOn2 ? ILI9341_WHITE : ILI9341_BLACK);
        tft.setCursor(6, 166);
        tft.setTextSize(3);
        tft.println("Hinten");
        if (lightIsOn2)
          tft.setCursor(35, 205);
        else
          tft.setCursor(65, 205);
        tft.setTextSize(10);
        tft.println(lightIsOn2 ? "OFF" : "ON");
      } else {
        if (lightIsOn)
          tft.setCursor(35, 125);
        else
          tft.setCursor(65, 125);
        tft.setTextSize(10);
        tft.println(lightIsOn ? "OFF" : "ON");
      }
    }

    tft.setCursor(2, 311);
    tft.setTextSize(1);
    tft.printf("MQTT %sconnected", !mqttClient.connected() ? "not " : "");
    tft.setCursor(216, 311);
    tft.println("v1.3");
  }
}


void mqttCallback(char* topic, byte* messageBytes, unsigned int length) {
  Serial.print(topic);
  Serial.print(": ");

  String message;
  for (int i = 0; i < length; i++)
    message += (char) messageBytes[i];
  Serial.println(message);

  if (String(topic) == mqttTopicRead)
    lightIsOn = message == "true";
  else if (String(topic) == mqttTopicRead2)
    lightIsOn2 = message == "true";

  display();
}


void mqttPublish(char* topic, bool state) {
  Serial.print("Publishing new state: ");
  Serial.println(state ? "true" : "false");
  mqttClient.publish(topic, state ? "true" : "false");
}


void loop() {
  display();
  if (!mqttClient.connected()) {
    mqttReconnect();
    mqttClient.loop();

    if (touchscreen.CheckTouched()) {
      touchscreen.Scan();

      if (mqttClientId == "BesprechungTFT" && touchscreen.Y >= 160)
        mqttPublish(mqttTopicWrite2, !lightIsOn2);
      else
        mqttPublish(mqttTopicWrite, !lightIsOn);

      delay(200);
    }
  }

  delay(50);
}
