// Shelly.cpp for ESP8266
// Fetches power and energy data from a Shelly device via HTTP API

#include "Shelly.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "utilities.h"

// Constructor
Shelly::Shelly(int id, const String& url) : id(id), url(url), power(0), total_energy(0), total_energy_today(0), timestring("") {}

// Set URL for Shelly device
void Shelly::setUrl(const String& url) {
    this->url = url;
}

// Fetch data from Shelly device
void Shelly::fetchData() {

    WiFiClient client;
    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();
    int retryCount = 0;

    Serial.print("Connect to Shelly Nr. ");
    Serial.println(id);

    // Retry-Logik für HTTP-Anfrage
    while (httpCode != 200 && retryCount < 10) {
        http.end();
        delay(1000);
        Serial.println("HTTP Fehler: " + String(httpCode) + ", versuche erneut...");
        http.begin(client, url);
        httpCode = http.GET();
        retryCount++;
    }
    // Wenn nach 5 Versuchen immer noch ein Fehler auftritt, abbrechen
    if (httpCode == 200) {
        String payload = http.getString();
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            Serial.println("Daten von Shelly Nr. " + String(id) + " empfangen.");
            
            this->power = doc["apower"];
            all_time_energy = doc["aenergy"]["total"];

            // Minuten-Timestamp holen
            unsigned long minute_ts = doc["aenergy"]["minute_ts"] | 0;

            Serial.print("Power:");
            Serial1.println(this->power);
            // Minuten-Timestamp in Zeit umwandeln
            convertTimestampToTime(minute_ts, timestring);
            Serial.print("Start-Minuten-Timestamp in Zeit: ");
            Serial.println(timestring);

        } else {
            Serial.println("JSON Error: " + String(error.c_str()));
        }
    }
    else {
        Serial.println("HTTP Fehler: " + String(httpCode));
    }
    http.end();
}

// Calculate energy based on reference values
void Shelly::calculateEnergy(float reference_energy_today, float reference_energy_total) {
    // Berechne die Energie basierend auf den Referenzwerten
    if (reference_energy_today >= 0 && reference_energy_total >= 0 && all_time_energy >= 0) {
        total_energy_today = all_time_energy - reference_energy_today;
        total_energy = all_time_energy - reference_energy_total;
    }
    else {
        total_energy_today = 0; // Falls kein Startwert gesetzt, Energie auf 0 setzen
        total_energy = 0;
    }
}

// Getter-Methoden
// Get current power
float Shelly::getPower() const {
  return power;
}

// Get all-time energy
float Shelly::getAllTimeEnergy() const {
  return all_time_energy;
}

// Get total energy
float Shelly::getTotalEnergy() const {
  return total_energy;
}

// Get today's energy
float Shelly::getTodayEnergy() const {
  return total_energy_today;
}

// Get timestring
const char* Shelly::getTimestring() const {
  return timestring;
}
