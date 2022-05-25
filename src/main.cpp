/* Author:        Hans Christian Diekmann
 * Last update:   04.05.2022
 * Initial start: 04.05.2022
 *
 * Function:
 * 1. Heltec LoRa ESP32 (V2) board, as a gateway can receive incoming LoRa messages.
 * 2. The incoming messages can be forwarded to a MQTT topic when a Wifi connection is established.
 * 
 * Description:
 * 1. Only hardware layer communication (P2P), no LoRaWAN protocol;
 * 2. Communication with any other LoRa device possible, eg. ESP32 LoRa, STM32 LoRa, etc. 
 *
 *
 * */

#include "Arduino.h"
#include "heltec.h" 
#include <WiFi.h>
#include <PubSubClient.h>

#define BAND                      868E6  //you can set band here directly,e.g. 868E6,915E6

const char* ssid = "Kempensebaan 91";
const char* password = "solutio365";
const char* MQTT_SERVER = "broker.mqtt-dashboard.com";
WiFiClient espClient;
PubSubClient client(espClient);

void DisplayData();

String rssi = "RSSI --";
String packSize = "--";
String rxPacket;

void LoRaSetup(){
  Heltec.begin(true /*DisplayEnable*/, true /*LoRaEnable*/, true /*SerialEnable*/, 
               true /*PABOOST Enable*/, BAND /*BandFrequency*/);
  LoRa.setSpreadingFactor(8);
  LoRa.setSignalBandwidth(125E3); // 125kHz
  LoRa.setCodingRate4(4);
  LoRa.setSyncWord(0x12);   //0x34
  LoRa.setPreambleLength(8);
  
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  delay(1500);
  Heltec.display->clear();
}

void cbk(int packetSize) {
  rxPacket ="";
  packSize = String(packetSize,DEC);
  for (int i = 0; i < packetSize; i++) { rxPacket += (char) LoRa.read(); }
  client.publish("ESP32LoRaGPS", rxPacket.c_str());
  rssi = "RSSI " + String(LoRa.packetRssi(), DEC) ;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
      client.subscribe("ESP32LoRaReceiver", 0);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void DisplayData() {
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0 , 15 , "Received "+ packSize + " bytes");
  Heltec.display->drawStringMaxWidth(0 , 26 , 128, rxPacket);
  Heltec.display->drawString(0, 0, rssi);  
  Heltec.display->display();
}

void setup() {
  LoRaSetup();
  setup_wifi();
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(MQTTcallback);

  Heltec.display->drawString(0, 0, "Gateway Started");
  Heltec.display->drawString(0, 10, "Waiting for incoming data...");
  Heltec.display->display();
  delay(1000);
  LoRa.receive();
}

void loop() {
  client.loop();
    if (!client.connected()) {
    reconnect();
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) { cbk(packetSize);  }
  DisplayData();
  delay(10);
}