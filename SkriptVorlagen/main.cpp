#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
// Hilfsfunktion: aktuelle Uhrzeit holen
#include <time.h>
#include "SH1106Wire.h" // oder #include "SSD1306Wire.h" für SSD1306
#include <Preferences.h> // für EEPROM speichern

#include "private.h" // Enthält WLAN Einstellungen und andere private Konstanten
#include "Shelly.h" // Shelly Klasse
#include "Nvs.h"    // NVS Klasse
#include "utilities.h" // Utilities

Preferences prefs;

#define SDA_PIN 21
#define SCL_PIN 22

#define SDA_PIN1 17
#define SCL_PIN1 16

SH1106Wire display1(0x3C, SDA_PIN, SCL_PIN, GEOMETRY_128_64, I2C_ONE);  // Bus 1
SH1106Wire display2(0x3C, SDA_PIN1, SCL_PIN1, GEOMETRY_128_64, I2C_TWO); // Bus 2

#define NEW_START_ENERGY_VALUE 0

#define CHECK_NIGHT 1

const char* shellyURL = "/rpc/Switch.GetStatus?id=0";
char shelly1_url[100];
char shelly2_url[100];

float power_1 = 0; // Aktuelle Leistung
float total_energy_ref_1 = 0; // Gesamtenergie Referenzwert für Shelly 1
unsigned long minute_ts_1 = 0; // Aktueller Minuten-Timestamp für Shelly 1

float power_2 = 0; // Aktuelle Leistung
float total_energy_ref_2 = 0; // Gesamtenergie Referenzwert für Shelly 2
unsigned long minute_ts_2 = 0; // Aktueller Minuten-Timestamp für Shelly 2

float energy_1_ref = -1; // Startwert für Totalenergie
float energy_1_ref_today = -1; // Startwert für Totalenergie
float energy_2_ref = -1; // Startwert für Totalenergie Shelly 2
float energy_2_ref_today = -1; // Startwert für Totalenergie Shelly
unsigned long min_1_ref_ts = 0; // Start-Minuten-Timestamp
unsigned long min_1_ref_ts_today = 0; // Start-Minuten-Timestamp heute
unsigned long min_2_ref_ts = 0; // Start-Minuten-Timestamp Shelly 2
unsigned long min_2_ref_ts_today = 0; // Start-Minuten-T

char nvs_time_str_total[20] = ""; // NVS Zeit als String
char nvs_time_str_today[20] = ""; // NVS Zeit als String

char start_time_str_1[20] = ""; // Startzeit als String
char start_time_str_1_today[20] = ""; // Startzeit als String

char start_time_str_2[20] = ""; // Startzeit als String
char start_time_str_2_today[20] = ""; // Startzeit als String

Shelly shelly1(1, shelly1_url);
Shelly shelly2(2, shelly2_url);

Nvs nvs;

// Funktionsprototypen
void drawOnDisplay(int display_num, float power, float energy_today, String start_time_str_today, float energy_total, String start_time_str);

void setup() {
    Serial.begin(115200);
    display1.init();
    display1.clear();
    display1.setFont(ArialMT_Plain_16);

    display2.init();
    display2.clear();
    display2.setFont(ArialMT_Plain_16);

    strcpy(shelly1_url, "http://");
    strcat(shelly1_url, id1);
    strcat(shelly1_url, shellyURL);

    strcpy(shelly2_url, "http://");
    strcat(shelly2_url, id2);
    strcat(shelly2_url, shellyURL);

    shelly1.setUrl(shelly1_url);
    shelly2.setUrl(shelly2_url);

    Serial.begin(115200);
    Serial.print("Shelly URLs: ");
    Serial.println(shelly1_url);
    Serial.println(shelly2_url);

    display1.drawString(0, 0, "Verbinde mit WLAN...");
    display1.display();

    display2.drawString(0, 0, "Verbinde mit WLAN...");
    display2.display();

    Serial.print("Verbinde WLAN...");
    Serial.println(ssid);

    WiFi.begin(ssid, password); 

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Verbunden!");
    
    display1.clear();

    display1.drawString(0, 0, "Verbunden!");
    display1.display();
    Serial.println("Verbunden!");

    #if NEW_START_ENERGY_VALUE
    shelly1.fetchData();
    shelly2.fetchData();

    // EEPROM/NVS Bereich komplett löschen und neue Startwerte schreiben
    nvs.clear();

    nvs.update_reference_energy_today(1, shelly1.getAllTimeEnergy(), shelly1.getTimestring());
    nvs.update_reference_energy_total(1, shelly1.getAllTimeEnergy(), shelly1.getTimestring());
    nvs.update_reference_energy_today(2, shelly2.getAllTimeEnergy(), shelly2.getTimestring());
    nvs.update_reference_energy_total(2, shelly2.getAllTimeEnergy(), shelly2.getTimestring());

    nvs.read_values();

    Serial.println("SHelly 1");
    Serial.println("EEPROM gelöscht und Startwert ins NVS geschrieben: " + String(shelly1.getTotalEnergy(), 1) + " Wh");
    Serial.println("Start-Minute-Timestamp ins NVS geschrieben: " + String(shelly1.getTimestring()));
    Serial.println("SHelly 2");
    Serial.println("EEPROM gelöscht und Startwert ins NVS geschrieben: " + String(shelly2.getTotalEnergy(), 1) + " Wh");
    Serial.println("Start-Minute-Timestamp ins NVS geschrieben: " + String(shelly2.getTimestring()));
    
    #else
    // Alle relevanten Werte aus NVS lesen
    nvs.read_values();
    
    // Debug-Ausgabe der Startwerte
    Serial.println("NVS Shelly 1");
    Serial.println("Startwert total NVS: " + String(nvs.get_reference_energy_total(1), 1) + " Wh");
    Serial.println("Start-Minute-Timestamp total NVS: " + String(nvs.get_reference_timestamp_total(1)));
    Serial.println("Startwert heute NVS: " + String(nvs.get_reference_energy_today(1), 1) + " Wh");
    Serial.println("EEPROM Start-Minute-Timestamp heute NVS: " + String(nvs.get_reference_timestamp_today(1)));

    Serial.println("NVS Shelly 2");
    Serial.println("Startwert total NVS: " + String(nvs.get_reference_energy_total(2), 1) + " Wh");
    Serial.println("Start-Minute-Timestamp total NVS: " + String(nvs.get_reference_timestamp_total(2)));
    Serial.println("Startwert heute NVS: " + String(nvs.get_reference_energy_today(2), 1) + " Wh");
    Serial.println("EEPROM Start-Minute-Timestamp heute NVS: " + String(nvs.get_reference_timestamp_today(2)));

    // Prüfe, ob gespeicherter Tag heute ist
    // hole aktuelle Zeit
    shelly1.fetchData();

    // Vergleiche eeprom Tag mit Tag aus shelly api
    if (strncmp(nvs.get_reference_timestamp_today(1).c_str(), shelly1.getTimestring(), 13) == 0) {
        Serial.println("Die Zeitstempel liegen im gleichen Tag.");
    } else {
        Serial.println("Neuer Tag erkannt. Werte werden in NVS aktualisiert.");
        nvs.update_reference_energy_today(1, shelly1.getAllTimeEnergy(), shelly1.getTimestring());
    }

    // Nochmal alle relevanten Werte aus NVS lesen
    nvs.read_values();
    
    #endif
}

void loop() {

    // Kontrolliere WLAN-Verbindung

    if (WiFi.status() != WL_CONNECTED) {
        int retryCount = 0;
        WiFi.begin(ssid, password);
        // Optional: Warten bis verbunden
        while (WiFi.status() != WL_CONNECTED && retryCount < 10) {
            delay(500);
            Serial.print(".");
            retryCount++;
            WiFi.begin(ssid, password);
            if (retryCount > 10) {
                Serial.println("Verbindung fehlgeschlagen.");
                esp_sleep_enable_timer_wakeup(20 * 1000000); // 20 Sekunden (Mikrosekunden)
                Serial.println("Gehe für 20 Sekunden in Light Sleep...");
                delay(1000); // Kurze Pause, um den Serial-Ausgang zu stabilisieren
                esp_light_sleep_start();
                Serial.println("Wieder wach!");
                delay(1000); // Kurze Pause, um den Serial-Ausgang zu stabilisieren
                retryCount = 0;
            }
        }
        Serial.println("Wieder verbunden!");
    }

    // Hole aktuelle Werte von Shelly 1
    shelly1.fetchData();
    //get_shelly_info(shelly1_url, power_1, total_energy_ref_1, minute_ts_1, 1);
    char time_str_1[20] = "";
    //convertTimestampToTime(minute_ts_1, time_str_1);

    // Vergleiche Datum-String (YYYY-MM-DD) für Tageswechsel
    nvs.read_values();
    Serial.print(nvs.get_reference_timestamp_today(1));
    Serial.print(" vs. ");
    Serial.print(shelly1.getTimestring());

    if (new_day_detected(nvs.get_reference_timestamp_today(1).c_str(), shelly1.getTimestring())) {
        Serial.println("Neuer Tag erkannt. Werte werden in NVS aktualisiert.");
        nvs.update_reference_energy_today(1, shelly1.getAllTimeEnergy(), shelly1.getTimestring());
    }
    else{
        Serial.println("Kein neuer Tag erkannt.");
    }

    // Energie seit heute berechnen
    shelly1.calculateEnergy(nvs.get_reference_energy_today(1), nvs.get_reference_energy_total(1));

    int day_today_1 = atoi(&nvs.get_reference_timestamp_today(1)[8]);
    int month_today_1 = atoi(&nvs.get_reference_timestamp_today(1)[5]);

    String date_today_1 = String(day_today_1) + "." + String(month_today_1);

    int day_total_1 = atoi(&nvs.get_reference_timestamp_total(1)[8]);
    int month_total_1 = atoi(&nvs.get_reference_timestamp_total(1)[5]);
    int year_total_1 = atoi(&nvs.get_reference_timestamp_total(1)[0]);

    String date_total_1 = String(day_total_1) + "." + String(month_total_1) + "." + String(year_total_1);

    drawOnDisplay(1, shelly1.getPower(), shelly1.getTodayEnergy(), date_today_1, shelly1.getTotalEnergy(), date_total_1);

    Serial.println("Shelly 1 API Output:");
    Serial.println("Leistung: " + String(shelly1.getPower(), 1) + " W");
    Serial.println("Energie heute: " + String(shelly1.getTodayEnergy(), 1) + " Wh");
    Serial.println("Seit: " + String(nvs.get_reference_timestamp_today(1)));
    Serial.println("Energie total: " + String(shelly1.getTotalEnergy(), 1) + " Wh");
    Serial.println("Seit: " + String(nvs.get_reference_timestamp_total(1)));

    //--------------------------------------
    // Hole aktuelle Werte von Shelly 2
    shelly2.fetchData();

    if (new_day_detected(nvs.get_reference_timestamp_today(2).c_str(), shelly2.getTimestring())) {
        Serial.println("Neuer Tag erkannt. Werte werden in NVS aktualisiert.");
        nvs.update_reference_energy_today(2, shelly2.getAllTimeEnergy(), shelly2.getTimestring());
    }
    else{
        Serial.println("Kein neuer Tag erkannt.");
    }

    // Energie seit heute berechnen
    shelly2.calculateEnergy(nvs.get_reference_energy_today(2), nvs.get_reference_energy_total(2));

    int day_today_2 = atoi(&nvs.get_reference_timestamp_today(2)[8]);
    int month_today_2 = atoi(&nvs.get_reference_timestamp_today(2)[5]);

    String date_today_2 = String(day_today_2) + "." + String(month_today_2);

    int day_total_2 = atoi(&nvs.get_reference_timestamp_total(2)[8]);
    int month_total_2 = atoi(&nvs.get_reference_timestamp_total(2)[5]);
    int year_total_2 = atoi(&nvs.get_reference_timestamp_total(2)[0]);

    String date_total_2 = String(day_total_2) + "." + String(month_total_2) + "." + String(year_total_2);

    drawOnDisplay(2, shelly2.getPower(), shelly2.getTodayEnergy(), date_today_2, shelly2.getTotalEnergy(), date_total_2);

    Serial.println("Shelly 2 API Output:");
    Serial.println("Leistung: " + String(shelly2.getPower(), 1) + " W");
    Serial.println("Energie heute: " + String(shelly2.getTodayEnergy(), 1) + " Wh");
    Serial.println("Seit: " + String(nvs.get_reference_timestamp_today(2)));
    Serial.println("Energie total: " + String(shelly2.getTotalEnergy(), 1) + " Wh");
    Serial.println("Seit: " + String(nvs.get_reference_timestamp_total(2)));

        // Uhrzeit direkt aus start_time_str_2_today    extrahieren
    int hour = atoi(&nvs.get_reference_timestamp_today(2)[11]);
    int minute = atoi(&nvs.get_reference_timestamp_today(2)[14]);
    

    Serial.println("Aktuelle Uhrzeit: " + String(hour) + ":" + String(minute));
    
    #if CHECK_NIGHT
    // Nachtzeit: 22:00 bis 6:00 Uhr
    if (hour >= 22 || hour < 6) {
        int sleep_seconds;
        if (hour >= 22) {
            // Bis 6 Uhr morgens schlafen
            sleep_seconds = ((24 - hour + 6) * 3600) - (minute * 60);
        } else {
            // Bis 6 Uhr morgens schlafen
            sleep_seconds = ((6 - hour) * 3600) - (minute * 60);
        }
        Serial.println("Nacht erkannt! Gehe in Deep Sleep bis 6 Uhr...");
        esp_sleep_enable_timer_wakeup(sleep_seconds * 1000000ULL);
        delay(1000); // Serial stabilisieren
        esp_deep_sleep_start();
    } else {
        // ESP32 für 20 Sekunden in Light Sleep schicken
        esp_sleep_enable_timer_wakeup(20 * 1000000); // 20 Sekunden (Mikrosekunden)
        Serial.println("Gehe für 20 Sekunden in Light Sleep...");
        delay(1000); // Kurze Pause, um den Serial-Ausgang zu stabilisieren
        esp_light_sleep_start();
        Serial.println("Wieder wach!");
        delay(1000); // Kurze Pause, um den Serial-Ausgang zu stabilisieren
    }
    #else
    delay(30000);
    #endif
}


void drawOnDisplay(int display_num, float power, float energy_today, String start_time_str_today, float energy_total, String start_time_str)
{
    if(display_num == 1){
        display1.clear();
        display1.setFont(ArialMT_Plain_10);
        display1.drawString(40, 0, "PV Kandel");
        display1.drawString(45, 15, String(power, 1) + " W");
        display1.drawString(0, 30, String("Heute (" +  start_time_str_today + "):"));
        if (energy_today > 1000)
        {
            display1.drawString(80, 30, String(energy_today / 1000, 1) + " kWh");
        }
        else{
            display1.drawString(80, 30, String(energy_today, 1) + " Wh");
        }
        display1.drawString(0, 45, String("Seit " + start_time_str + ":"));
        if (energy_total > 1000)
        {
            display1.drawString(80, 45, String(energy_total / 1000, 1) + " kWh");
        }
        else{
            display1.drawString(80, 45, String(energy_total, 1) + " Wh");
        }
        display1.display();
    }
    else{
        display2.clear();
        display2.setFont(ArialMT_Plain_10);
        display2.drawString(40, 0, "PV Blume");
        display2.drawString(45, 15, String(power, 1) + " W");
        display2.drawString(0, 30, String("Heute (" +  start_time_str_today + "):"));
        if (energy_today > 1000)
        {
            display2.drawString(80, 30, String(energy_today / 1000, 1) + " kWh");
        }
        else{
            display2.drawString(80, 30, String(energy_today, 1) + " Wh");
        }
        display2.drawString(0, 45, String("Seit " + start_time_str + ":"));
        if (energy_total > 1000)
        {
            display2.drawString(80, 45, String(energy_total / 1000, 1) + " kWh");
        }
        else{
            display2.drawString(80, 45, String(energy_total, 1) + " Wh");
        }
        display2.display();
    }
}


void get_shelly_info(char *shelly_url, float &power, float &total_energy_ref, unsigned long &minute_ts, int shelly_id)
{
    WiFiClient client;
    HTTPClient http;
    http.begin(client, shelly_url);
    int httpCode = http.GET();
    int retryCount = 0;
    // Retry-Logik für HTTP-Anfrage
    while (httpCode != 200 && retryCount < 5) {
        delay(1000);
        Serial.println("HTTP Fehler: " + String(httpCode) + ", versuche erneut...");
        httpCode = http.GET();
        retryCount++;
    }
    // Wenn nach 5 Versuchen immer noch ein Fehler auftritt, abbrechen
    if (httpCode == 200) {
        String payload = http.getString();
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            power = doc["apower"];
            total_energy_ref = doc["aenergy"]["total"];

            // Minuten-Timestamp holen
            minute_ts = doc["aenergy"]["minute_ts"] | 0;

            Serial.print("Start-Minuten-Timestamp: ");
            Serial.println(minute_ts);
            // Minuten-Timestamp in Zeit umwandeln
            char time_str[20];
            //convertTimestampToTime(minute_ts, time_str);
            Serial.print("Start-Minuten-Timestamp in Zeit: ");
            Serial.println(time_str);
        } else {
            if(shelly_id == 1){
                display1.drawString(0, 0, "JSON Error");
                display1.display();
            } else {
                display2.drawString(0, 0, "JSON Error");
                display2.display();
            }
            Serial.println("JSON Error: " + String(error.c_str()));
        }
    }
    else {
        if(shelly_id == 1){
            display1.drawString(0, 0, "HTTP Fehler: " + String(httpCode));
            display1.display();
        } else {
            display2.drawString(0, 0, "HTTP Fehler: " + String(httpCode));
            display2.display();
        }
        Serial.println("HTTP Fehler: " + String(httpCode));
    }
    http.end();
}