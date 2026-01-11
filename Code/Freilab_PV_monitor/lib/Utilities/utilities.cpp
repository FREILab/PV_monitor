// utilities.cpp
// Hilfsfunktionen für Zeitumwandlung und Energieberechnung

#include "Arduino.h"
#include "utilities.h"
#include <time.h>
#include <string.h>

// Konvertiert einen Unix-Timestamp in einen lesbaren Zeitstring
void convertTimestampToTime(unsigned long timestamp, char *time_str) 
 {
    // Shelly liefert Unix-Timestamp in Sekunden
    time_t ts = timestamp + 60*120;
    // Lokalen Offset holen (Sommer/Winterzeit automatisch)
    struct tm *tm_info = localtime(&ts);
    if (tm_info) {
        strftime(time_str, 20, "%Y-%m-%d %H:%M", tm_info);
    } else {
        strcpy(time_str, "unbekannt");
    }
    Serial.print("Timestamp: ");
    Serial.println(timestamp);
    Serial.print("Zeit: ");
    Serial.println(time_str);
}

// Berechnet die Energie basierend auf dem Gesamtenergiezähler und einem Referenzwert
void calculate_energy(float &energy, float total_energy, float energy_1_ref)
{
    if (energy_1_ref >= 0) {
        energy = total_energy - energy_1_ref;
    } else {
        energy = 0; // Falls kein Startwert gesetzt, Energie auf 0 setzen
    }
}

// Prüft, ob ein neuer Tag begonnen hat basierend auf den Zeitstrings
bool new_day_detected(const char* nvs_timestamp, const char* shelly_timestamp)
{
    return strncmp(nvs_timestamp, shelly_timestamp, 13) != 0;
}