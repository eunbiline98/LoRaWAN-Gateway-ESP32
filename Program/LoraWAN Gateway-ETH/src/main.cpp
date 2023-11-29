#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <LoRa.h>
#include <EthernetSPI2.h>

#define ss 5    // LoRa radio chip select
#define rst 26  // LoRa radio reset
#define dio0 15 // change for your board; must be a hardware interrupt pin

#define led_data 21

const char *MQTT_SERVER = "192.168.8.144";
const char *MQTT_STATUS = "tele/loraWAN/eth/LWT";

const char *DATA_CMD = "loraWAN/nodes/cmnd";
const char *DATA_STATE = "loraWAN/nodes/stat";

const char *MQTT_CLIENT_ID = "LoRaWAN-03Paditech";
const char *MQTT_USERNAME = "padinet";
const char *MQTT_PASSWORD = "p4d1n3t";

const long frequency = 433E6; // LoRa Frequency

String rssi_1, rssi_2, rssi_3;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
    0x02, 0xAB, 0xCD, 0xEF, 0x00, 0x01};
IPAddress ip(192, 168, 8, 159);
IPAddress dns(8, 8, 8, 8);
IPAddress gateway(192, 168, 8, 1);
IPAddress subnet(255, 255, 255, 0);

EthernetClient ethClient;
PubSubClient client(ethClient);
String id, Data, Data1, Data2, Data3;

void setup_eth()
{
  Ethernet.init(2);
  Ethernet.begin(mac, ip, dns, gateway, subnet);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true)
    {
      Serial.print(".");
      delay(200);
    }
  }
  while (Ethernet.linkStatus() == LinkOFF)
  {
    Serial.println("Ethernet cable is NOT connected.");
    digitalWrite(led_data, HIGH);
    delay(200);
    digitalWrite(led_data, LOW);
    delay(200);
  }

  Serial.println("Ethernet cable is now connected.");

  delay(500);
  // print ip address
  Serial.print("Local IP : ");
  Serial.println(Ethernet.localIP());
  Serial.print("Subnet Mask : ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("Gateway IP : ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("DNS Server : ");
  Serial.println(Ethernet.dnsServerIP());
}

void LoRa_rxMode()
{
  LoRa.disableInvertIQ(); // normal mode
  LoRa.receive();         // set receive mode
}

void LoRa_txMode()
{
  LoRa.idle();           // set standby mode
  LoRa.enableInvertIQ(); // active invert I and Q signals
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
  int len_ID, len_DT;
  String message = "";

  while (LoRa.available())
  {
    message += (char)LoRa.read();
  }

  len_ID = message.indexOf(",");
  len_DT = message.indexOf("#");

  id = message.substring(0, len_ID);            // parsing id
  Data = message.substring(len_ID + 1, len_DT); // parsing data

  if (id == "001")
  {
    Data1 = Data;
    rssi_1 = String(LoRa.packetRssi()).c_str();
    // Serial.print("001, ");
    // Serial.println(Data1);
  }
  if (id == "002")
  {
    Data2 = Data;
    rssi_2 = String(LoRa.packetRssi()).c_str();
    // Serial.print("002, ");
    // Serial.println(Data2);
  }
  if (id == "003")
  {
    Data3 = Data;
    rssi_3 = String(LoRa.packetRssi()).c_str();
    // Serial.print("003, ");
    // Serial.println(Data3);
  }
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

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD, MQTT_STATUS, 1, 1, "Offline"))
    {
      Serial.println("connected");
      // subscribe
      client.subscribe(DATA_CMD);
      client.publish(MQTT_STATUS, "Online", true);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      digitalWrite(led_data, HIGH);
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200); // initialize serial
  pinMode(led_data, OUTPUT);

  while (!Serial)
    ;
  Serial.println("LoRaWAN Gateway ESP32-Ethernet V1.0");
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
  LoRa_rxMode();

  setup_eth();
  // client.setCallback(callback);
  client.setServer(MQTT_SERVER, 1883);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  if (runEvery(1000))
  { // repeat every time
    digitalWrite(led_data, HIGH);
    Serial.println("Gateway Get Data");
    Serial.print("01: ");
    Serial.println(Data1);
    Serial.print("02: ");
    Serial.println(Data2);
    Serial.print("03: ");
    Serial.println(Data3);
    Serial.println("==================");

    // json format print
    StaticJsonBuffer<300> JSONbuffer;
    JsonObject &JSONencoder = JSONbuffer.createObject();

    JSONencoder["data_1"] = Data1;
    JSONencoder["data_2"] = Data2;
    JSONencoder["data_3"] = Data3;
    JSONencoder["rssi_1"] = rssi_1;
    JSONencoder["rssi_2"] = rssi_2;
    JSONencoder["rssi_3"] = rssi_3;

    char JSONmessageBuffer_data[200];
    JSONencoder.printTo(JSONmessageBuffer_data, sizeof(JSONmessageBuffer_data));
    Serial.println("Sending message to MQTT topic..");
    Serial.println(JSONmessageBuffer_data);

    if (client.publish(DATA_STATE, JSONmessageBuffer_data) == true)
    {
      Serial.println("Success sending message");
    }
    else
    {
      Serial.println("Error sending message");
    }
  }
  while (Ethernet.linkStatus() == LinkOFF)
  {
    Serial.println("Ethernet cable is not connected.");
    digitalWrite(led_data, HIGH);
    delay(200);
    digitalWrite(led_data, LOW);
    delay(200);
  }
  digitalWrite(led_data, LOW);
}
