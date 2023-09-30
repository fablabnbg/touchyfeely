
#include "Wire.h"
#include "Board_Pinout.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "NS2009.h"
#include "ETH.h"
#include "PubSubClient.h"

#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 12

#define LED_PIN 33

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
WiFiClient Client;
PubSubClient Mqttclient(Client);
 

NS2009 TS(false, true);

bool LightIsOn = false;

static bool Eth_connected = false;
static bool BlockNextMessage = false;

const char *Mqttserver = "172.22.35.45";
const int Mqttport = 1883;

#define WERKSTATT 1

#ifdef KUECHE
const char * MqttClientID = "KuecheTFT";
const char * MqttTopicRead = "CtrlTFT/Licht/Kueche/status";
const char * MqttTopicWrite = "CtrlTFT/Licht/Kueche/set";
#endif

#ifdef BESPRECHUNG
const char * MqttClientID = "BesprechungTFT";
const char * MqttTopicRead = "CtrlTFT/Licht/Besprechungszimmer/Vorne/status";
const char * MqttTopicWrite = "CtrlTFT/Licht/Besprechungszimmer/Vorne/set";
const char * MqttTopicWrite2 = "CtrlTFT/Licht/Besprechungszimmer/Hinten/set";
#endif

#ifdef WERKSTATT
const char * MqttClientID = "WerkstattTFT";
const char * MqttTopicRead = "CtrlTFT/Licht/Werkstatt/status";
const char * MqttTopicWrite = "CtrlTFT/Licht/Werkstatt/set";
#endif

void MQTTReconnect()
{
  while (!Mqttclient.connected()) {
    Display();
    Serial.print("Attempting MQTT connection...");
    if (Mqttclient.connect(MqttClientID)) {
      Serial.println("connected");
      Mqttclient.subscribe(MqttTopicRead);
    } else {
      Serial.print("failed, rc=");
      Serial.print(Mqttclient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

//heißt zwar WiFiEvent, ist aber ETH
void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      ETH.setHostname("esp32-ethernet");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      Eth_connected = true;
      MQTTReconnect();
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      Eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      Eth_connected = false;
      break;
    default:
      break;
  }
}

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  pinMode(TFT_DC, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  tft.begin();
  Display();
  //heißt zwar WiFi, ist aber ETH
  WiFi.onEvent(WiFiEvent);
  ETH.begin();

  Mqttclient.setServer(Mqttserver, Mqttport);
  Mqttclient.setCallback(mqtt_callback);
}

bool previousLightIsOn;
bool previousMqttclientConnected;
void Display() {
  // only redraw if something changed
  if (LightIsOn != previousLightIsOn || Mqttclient.connected() != previousMqttclientConnected) {
    previousLightIsOn = LightIsOn;
    previousMqttclientConnected = Mqttclient.connected();

    tft.fillScreen(LightIsOn ? ILI9341_BLACK : ILI9341_WHITE);
    tft.setTextColor(LightIsOn ? ILI9341_WHITE : ILI9341_BLACK);

    tft.setTextSize(10);
    if (LightIsOn)
      tft.setCursor(35, 125);
    else
      tft.setCursor(65, 125);
    tft.println(LightIsOn ? "OFF" : "ON");

    tft.setCursor(2, 311);
    tft.setTextSize(1);
    tft.printf("MQTT %sconnected", !Mqttclient.connected() ? "not " : "");
    tft.setCursor(216, 311);
    tft.printf("v1.1");
  }
}

void mqtt_callback(char* topic, byte* message, unsigned int length)
{
  Serial.print(topic);
  Serial.print(": ");
  String messageTemp;

  for (int i = 0; i < length; i++)
    messageTemp += (char) message[i];
  Serial.println(messageTemp);

  if (String(topic) == MqttTopicRead) {
    LightIsOn = messageTemp == "true";
    Display();
  }
}


void SendMQTTMessage(bool on)
{
  Serial.println("Sending MQTT message");
  Mqttclient.publish(MqttTopicWrite, on ? "true" : "false");

#ifdef BESPRECHUNG
  Mqttclient.publish(MqttTopicWrite2, on ? "true" : "false");
#endif
}

void loop(void)
{
  if (!Mqttclient.connected())
    MQTTReconnect();
  Mqttclient.loop();

  if (TS.CheckTouched()) {
    SendMQTTMessage(LightIsOn = !LightIsOn);
    delay(200);
  }

  delay(50);
}

