
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <SoftwareSerial.h>
#include <PZEM004T.h>
//#include <avr/wdt.h>
double sens[4];

//unsigned long last_mls,last_send = millis();
//unsigned long count;

PZEM004T pzem(13,15);  // RX,TX
IPAddress ip(192,168,1,1);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // 0x3F - адрес
//EDIT THESE LINES TO MATCH YOUR SETUP


const char* host = "esp8266-webupdate";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "admin";
const char* ssid = "*******";
const char* password = "*********";

#define MQTT_SERVER "10.10.100.14"  ///YourMQTTBroker'sIP
#define mqtt_port  1883
const char* mqtt_user="orangepi";
const char* mqtt_pass="orangepi";

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}
WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);


#define BUFFER_SIZE 100

String macToStr(const uint8_t* mac){
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);}
  return result;
}

void re_connect() {

  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){
   Serial.print("Connecting to ");
   lcd.clear();  
   lcd.print("Wi-Fi Connecting");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, password);}
    else {
      delay(5000);
    }


  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){
  // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {
      Serial.print("\tAttempting MQTT connection...");
      lcd.clear();  
      lcd.print("MQTT connection...");
  
      if (client.connect("esp8266-AD14C5",mqtt_user,mqtt_pass)) {
        Serial.println("MQTT Connected");
        lcd.clear();  
        lcd.print("MQTT Connected");
      }
      //otherwise print failed for debugging
      else { Serial.println("\tFailed."); abort(); }
    }
  }
}
void setup() {

  Serial.begin(115200);
  pzem.setAddress(ip);
  pinMode(2, OUTPUT);    
  digitalWrite(2, LOW);    // выключаем светодиод
  delay(100);
  Wire.begin(4, 5);      // 4 - sda 5 - scl
  lcd.init();            // инициализация дисплея
  lcd.backlight();       // подсветка
  //lcd.blink();           // моргание пикселем
  lcd.print("System started"); // че то пишет

  
  //start wifi subsystem 
  WiFi.begin(ssid, password);
  //attempt to connect to the WIFI network and then connect to the MQTT server
  re_connect();
  client.publish("home/sensors/sensor1/alive","true");
  MDNS.begin(host);
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);


}
void loop(){

  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {re_connect();}
  //maintain MQTT connection
   delay(200);  
  httpServer.handleClient();
  sens[0] = pzem.voltage(ip);
  if (sens[0] < 0.0) sens[0] = 0.0; Serial.print(sens[0]);Serial.print(" V; ");
  client.publish("home/sensors/sensor1/v",String(sens[0]).c_str());
  sens[1] = pzem.current(ip);
  if(sens[1] >= 0.0){ Serial.print(sens[1]);Serial.print(" A; "); 
   client.publish("home/sensors/sensor1/i",String(sens[1]).c_str());}
  sens[2]= pzem.power(ip);
  if(sens[2] >= 0.0){ Serial.print(sens[2]);Serial.print(" W; "); 
    client.publish("home/sensors/sensor1/w",String(sens[2]).c_str());}
  wdt_reset();
  sens[3] = pzem.energy(ip);
  if(sens[3] >= 0.0){ Serial.print(sens[3]);Serial.println(" Wh; "); 
  client.publish("home/sensors/sensor1/wh",String(sens[3]).c_str());}

  //Print a message to the LCD.
  lcd.backlight();
  lcd.clear();   
  lcd.setCursor(0,0);
  lcd.print("V: ");
  lcd.setCursor(9,0);
  lcd.print("I: ");
  lcd.setCursor(2,0);
  lcd.print(sens[0]);
  lcd.setCursor(11,0);
  lcd.print(sens[1]);
  lcd.setCursor(0,1);
  lcd.print("W: ");
  lcd.setCursor(2,1);
  lcd.print(sens[2]);
  lcd.setCursor(7,1);
  lcd.print("Wh:");
  lcd.setCursor(10,1);
  lcd.print(sens[3]);

   client.loop();
}//void

