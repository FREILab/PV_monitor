#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <time.h>

#include "Shelly.h" // Shelly Klasse

// include private credentials
#include "private.h"


const char* shellyURL = "/rpc/Switch.GetStatus?id=0";
char shelly1_url[100];

// create Shelly object
Shelly shelly1(1, shelly1_url);

int pwmPin = D1;  // z. B. LED oder Motor
int ledPin = D4;

// Power variables
float cur_power = 0; // Current power
float cur_power_percent = 0; // Current power in percent of max power
const float max_power = 2000.0; // Maximum expected power for scaling

// Sleep time configuration
const int SLEEP_START_HOUR = 22; // Stunde, ab der geschlafen wird
const int SLEEP_END_HOUR = 6;   // Stunde, bis zu der geschlafen wird


int pwmValue;
unsigned long time_past = 0;
bool ledState = false;
bool timeSynced = false;

void checkSleepTime() {
  if (!timeSynced) return; // Wenn Zeit nicht synced, nicht schlafen

  time_t now = time(nullptr);
  struct tm * timeinfo = localtime(&now);
  int hour = timeinfo->tm_hour;

  if (hour >= SLEEP_START_HOUR || hour < SLEEP_END_HOUR) {
    // Schlafenszeit: Berechne Zeit bis 6 Uhr
    int sleepSeconds;
    if (hour >= SLEEP_START_HOUR) {
      // Bis Mitternacht + SLEEP_END_HOUR Stunden
      sleepSeconds = (24 - hour + SLEEP_END_HOUR) * 3600 - timeinfo->tm_min * 60 - timeinfo->tm_sec;
    } else {
      // Bis SLEEP_END_HOUR Uhr heute
      sleepSeconds = (SLEEP_END_HOUR - hour) * 3600 - timeinfo->tm_min * 60 - timeinfo->tm_sec;
    }
    Serial.print("Sleeping for ");
    Serial.print(sleepSeconds);
    Serial.println(" seconds until 6 AM.");
    WiFi.disconnect(); // WiFi abschalten, um Strom zu sparen
    // sleep for calculated seconds
    ESP.deepSleep(sleepSeconds * 1000000LL);
  }
}


void setup() {
  Serial.begin(115200); // Must match ESP8266 default baud rate
  
  // Wifi
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // NTP Zeit einrichten (CET, UTC+1)
  // Hinweis: Für Sommerzeit (DST) manuell auf UTC+2 ändern oder Bibliothek verwenden
  configTime(3600, 0, "pool.ntp.org"); // UTC+1 für Deutschland (keine automatische Sommerzeit)
  Serial.print("Waiting for NTP time sync...");
  unsigned long startTime = millis();
  while (time(nullptr) < 1000000 && millis() - startTime < 30000) { // Timeout nach 30 Sekunden
    delay(100);
    Serial.print(".");
  }
  if (time(nullptr) < 1000000) {
    Serial.println(" NTP sync failed! Continuing without time check.");
    timeSynced = false;
  } else {
    Serial.println(" Time synced!");
    timeSynced = true;
  }

  checkSleepTime();

  // setup Shelly connection
  strcpy(shelly1_url, "http://");
  strcat(shelly1_url, id1);
  strcat(shelly1_url, shellyURL);

  shelly1.setUrl(shelly1_url);

  //setup power meter
  pinMode(pwmPin, OUTPUT);
  analogWrite(pwmPin, 0); // Start mit 0 PWM

  //setup blinky LED
  pinMode(ledPin,OUTPUT);
  digitalWrite(ledPin, HIGH);
  ledState = true;

}

void loop() {
  // Your code here (e.g., send data to a server)

  shelly1.fetchData();

  cur_power = shelly1.getPower();
  Serial.print("Current Power: ");
  Serial.println(cur_power);

  cur_power_percent = (cur_power / max_power) * 100.0;

  Serial.print("Current Power (% of max): ");
  Serial.println(cur_power_percent);


  //int percent = 50; // 50 %
    
  //pwmValue = map(75, 0, 100, 0, 255);
  pwmValue = map((int)cur_power_percent, 0, 100, 0, 255);
  Serial.print("PWM Value: ");
  Serial.println(pwmValue);

  analogWrite(pwmPin, pwmValue);



  // Blink LED to show activity (toggle)
  if (millis() - time_past >= 500) {
    ledState = !ledState;
    digitalWrite(ledPin, ledState ? HIGH : LOW);
    time_past = millis();
  }

  checkSleepTime();

  delay(1000); // Warte 1 Sekunde zwischen Messungen, um CPU zu schonen

}
