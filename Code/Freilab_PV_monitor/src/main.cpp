#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <time.h>

#include "Shelly.h" // Shelly Klasse



const char* ssid = "xxx"; // Your network name
const char* password = "xxx"; // Your network password


const char* shellyURL = "/rpc/Switch.GetStatus?id=0";
char shelly1_url[100];

const char* id1 = "10.30.0.144";

Shelly shelly1(1, shelly1_url);


int pwmPin = D1;  // z. B. LED oder Motor


float cur_power = 0; // Current power
float cur_power_percent = 0; // Current power in percent of max power
const float max_power = 2000.0; // Maximum expected power for scaling

void setup() {
  Serial.begin(115200); // Must match ESP8266 default baud rate
  /*WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  strcpy(shelly1_url, "http://");
  strcat(shelly1_url, id1);
  strcat(shelly1_url, shellyURL);

  shelly1.setUrl(shelly1_url);*/
}

void loop() {
  // Your code here (e.g., send data to a server)

  //shelly1.fetchData();

  //cur_power = shelly1.getPower();
  //  Serial.print("Current Power: ");
  //Serial.println(cur_power);

  //cur_power_percent = (cur_power / max_power) * 100.0;

    //Serial.print("Current Power (% of max): ");
    //Serial.println(cur_power_percent);


      //int percent = 50; // 50 %
    
    int pwmValue = map(75, 0, 100, 0, 255);


    //long pwmValue = map(10, 0, 100, 0, 255);

    analogWrite(pwmPin, pwmValue);


/*
    pwmValue = map(50, 0, 100, 0, 1023);

    analogWrite(pwmPin, pwmValue);
    Serial.println(50);
    delay(5000);

    pwmValue = map(100, 0, 100, 0, 1023);


    Serial.println(100);
    analogWrite(pwmPin, pwmValue);
    delay(5000);


*/
   //delay(5000);


    
}
