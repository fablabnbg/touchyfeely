
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

#ifdef  ARDUINO_OLIMEXINO_STM32F3
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
#else
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
#endif

WiFiClient Client;
PubSubClient Mqttclient(Client);
 

NS2009  TS(false, true);

int Light_mode = 0;

static bool Eth_connected = false;
static bool BlockNextMessage = false;

const char * Mqttserver = "172.22.35.45";
const int Mqttport = 1883;

#define BESPRECHUNG 1

#ifdef KUECHE // Panel Kueche
const char * MqttClientID = "WerkstattTFT";
const char * MqttTopicRead = "homie/kuesp3bf9d2/CTLicht1/state";//"/SmartPanelControl/State/Licht/Werkstatt/";
const char * MqttTopicWrite = "homie/kuesp3bf9d2/CTLicht1/state/set";//"/SmartPanelControl/Ctrl/Licht/Werkstatt/";
#endif

#ifdef BESPRECHUNG // Besprechung
const char * MqttClientID = "BesprechungTFT";
const char * MqttTopicRead = "homie/bzespb3a46d/CTLicht1/state";
const char * MqttTopicWrite = "homie/bzespb3a46d/CTLicht1/state/set";
const char * MqttTopicWrite2 = "homie/bzespb3a46d/CTLicht2/state/set";
#endif

void MQTTReconnect()
{
  while (!Mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (Mqttclient.connect(MqttClientID)) {
      Serial.println("connected");
      // Subscribe
      Mqttclient.subscribe(MqttTopicRead);
    } else {
      Serial.print("failed, rc=");
      Serial.print(Mqttclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
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
      //set eth hostname here
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
  Serial.begin (115200);
  Wire.begin();
  pinMode(TFT_DC, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // off
  tft.begin();
  //heißt zwar WiFi, ist aber ETH
  WiFi.onEvent(WiFiEvent);
  ETH.begin();

  Mqttclient.setServer(Mqttserver, Mqttport);
  Mqttclient.setCallback(mqtt_callback);
  paint_on();
}

void paint_on()
{
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(100,120);
  tft.setTextSize(3);
  tft.println("ON");
  
}

void paint_off()
{
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(100,120);
  tft.setTextSize(3);
  tft.println("OFF");
}

void mqtt_callback(char* topic, byte* message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == MqttTopicRead) {
    
    Serial.print("Changing output to ");
    if(messageTemp == "on" || messageTemp == "ON"){
      Serial.println("on");
      DisplayLightToOff();
    }
    else if(messageTemp == "off" || messageTemp == "OFF"){
      Serial.println("off");
      DisplayLightToOn();
    }
  }
}


//kontrolliert nur das display und den internen licht status
void DisplayLightToOn()
{
  paint_on();
  Light_mode = 0;
}

//kontrolliert nur das display und den internen licht status
void DisplayLightToOff()
{
  paint_off();
  Light_mode = 1;
}

void SendMQTTMessage(bool on)
{
  Serial.println("Sending MQTT message");
  Mqttclient.publish(MqttTopicWrite, on ? "on" : "off");
  
#ifdef BESPRECHUNG // Besprechung
  Mqttclient.publish(MqttTopicWrite2, on ? "on" : "off");
#endif
}

void loop(void)
{

  if(!Mqttclient.connected())
  {
    MQTTReconnect();
  }
  Mqttclient.loop();

  if(TS.CheckTouched())
  {
    if(Light_mode != 0)
    {
      DisplayLightToOn();
      
    }else
    {
      DisplayLightToOff();
    }
    SendMQTTMessage(Light_mode);
    
    delay(1000);
  }

  delay(50);
}

/*void loop ()
{
  TS.ScanBlocking ();
  Serial.printf ("Screen touched!\n\r");
  Handle_Touch ();
  while (TS.CheckTouched ());
  Serial.printf ("Screen released!\n\r");
}*/
