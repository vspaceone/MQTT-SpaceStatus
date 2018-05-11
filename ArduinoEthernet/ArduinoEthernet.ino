//Requirements:
//You must install an Ethernet Shield with a w5100 Chip that uses the hardware SPI!
//

//      |
//libs \|/
//      '
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>

//             |
//other stuff \|/
//             '
#define CientName "SpaceStat"
#define Topic "SpaceStat"
#define OpenMsg "Open"
#define ClosedMsg "Closed"
#define EventMsg "Event"
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED }; //MAC of Shield
IPAddress ip(x, x, x, x); //Own IP
IPAddress server(x, x, x, x);// Server IP
EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);
const int rs = 12, en = 11, d4 = 4, d5 = 5, d6 = 6, d7 = 7; //LCD Data Pins
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Callback function
void callback(char* topic, byte* payload, unsigned int length) {

  //turn the LED ON if the payload is '1' and publish to the MQTT server a confirmation message
  if (payload[0] == 'EventMsg') {
    lcd.clear();
    lcd.print("Status: Event");
  }

  //turn the LED OFF if the payload is '0' and publish to the MQTT server a confirmation message
  if (payload[0] == OpenMsg) {
    lcd.clear();
    lcd.print("Status: Open");
  }
  if (payload[0] == ClosedMsg) {
    lcd.clear();
    lcd.print("Status: Closed");
  }
} // void callback

void setup() {
  lcd.begin(16, 2);
  lcd.print("Wait...");
  Ethernet.begin(mac, ip);
  lcd.clear();
  lcd.print("Connected ETH");
  client.connect(ClientName);
  lcd.clear();
  lcd.print("Connected MQTT");
  client.subscribe(Topic);
  lcd.clear();
  lcd.print("Subscirbed");
}

void loop() {
  if (digitalRead(2) or digitalRead(3)) {
    lcd.clear();
    lcd.print("Wait...");
    if (digitalRead(2)) {
      client.publish(Topic, OpenMsg);
    }
    if (digitalRead(3)) {
      client.publish(Topic, ClosedMsg);
    }
    if (digitalRead(2) and digitalRead(3)) {
      client.publish(Topic, EventMsg);
    }
  }
}


