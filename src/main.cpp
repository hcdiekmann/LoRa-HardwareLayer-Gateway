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
#include "EspMQTTClient.h"

#define BAND                      868E6  //you can set band here directly,e.g. 868E6,915E6

EspMQTTClient client(
  "WifiSSID",
  "WifiPassword",
  "192.168.1.100",                // MQTT Broker server ip
  "MQTTUsername",                 // Can be omitted if not needed
  "MQTTPassword",                 // Can be omitted if not needed
  "TestClient",                   // Client name that uniquely identify your device
  1883                            // The MQTT port, default to 1883. this line can be omitted
);

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

void ClientSetup(){
  client.enableDebuggingMessages();
  client.enableLastWillMessage("LoRa/Gateway1", "I am going offline", true); // activate retain flag by passing true as third parameter 
}

void onConnectionEstablished()
{
  client.publish("LoRa/Gateway1", "This is a message from LoRa Gateway");
}

void cbk(int packetSize) {
  rxPacket ="";
  packSize = String(packetSize,DEC);
  for (int i = 0; i < packetSize; i++) { rxPacket += (char) LoRa.read(); }
  rssi = "RSSI " + String(LoRa.packetRssi(), DEC) ;
  DisplayData();
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
  ClientSetup();

  Heltec.display->drawString(0, 0, "Gateway Started");
  Heltec.display->drawString(0, 10, "Waiting for incoming data...");
  Heltec.display->display();
  delay(1000);
  LoRa.receive();
}

void loop() {
  client.loop();
  
  int packetSize = LoRa.parsePacket();
  if (packetSize) { cbk(packetSize);  }
  delay(10);
}