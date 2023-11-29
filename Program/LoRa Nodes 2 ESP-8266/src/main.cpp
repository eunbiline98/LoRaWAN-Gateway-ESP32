#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include <DHT_U.h>

#define ss 15   // LoRa radio chip select
#define rst 16  // LoRa radio reset
#define dio0 13 // change for your board; must be a hardware interrupt pin

#define DHTPIN 0
#define DHTTYPE DHT22

const long frequency = 433E6; // LoRa Frequency

float temperature;
float humidity;

DHT dht(DHTPIN, DHTTYPE);

void LoRa_rxMode()
{
  LoRa.enableInvertIQ(); // active invert I and Q signals
  LoRa.receive();        // set receive mode
}

void LoRa_txMode()
{
  LoRa.idle();            // set standby mode
  LoRa.disableInvertIQ(); // normal mode
}
void LoRa_sendMessage(String message)
{
  LoRa_txMode();       // set tx mode
  LoRa.beginPacket();  // start packet
  LoRa.print(message); // add payload
  LoRa.endPacket();    // finish packet and send it
  LoRa_rxMode();       // set rx mode
}

void onReceive(int packetSize)
{
  String message = "";
  while (LoRa.available())
  {
    message += (char)LoRa.read();
  }
  Serial.print("Node Receive: ");
  Serial.println(message);
}
boolean runEvery(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

void setup()
{
  Serial.begin(115200); // initialize serial
  dht.begin();
  while (!Serial)
    ;
  Serial.println("LoRa Nodes 2");
  LoRa.setPins(ss, rst, dio0);

  if (!LoRa.begin(frequency))
  {
    Serial.println("Starting LoRa failed!");
    while (true)
      ; // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
  Serial.println();

  LoRa.onReceive(onReceive);
  // LoRa_rxMode();
}

void loop()
{
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  if (runEvery(1000))
  {                            // repeat every 1000 millis
    String message = "003,";   // id
    message += millis();    // kirim data millis,
    message += "#";            // tanda akhir data
    LoRa_sendMessage(message); // send a message
  }
}
