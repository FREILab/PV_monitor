#include <Arduino.h>

// Shelly.h
// Definition der Shelly-Klasse zur Kommunikation mit Shelly-Geräten
class Shelly {
  public:
    Shelly(int id, const String& url);
    void fetchData();
    void setUrl(const String& url);
    float getPower() const;
    float getAllTimeEnergy() const;
    float getTotalEnergy() const;
    float getTodayEnergy() const;
    void calculateEnergy(float reference_energy_today, float reference_energy_total);
    const char* getTimestring() const;

  private:
    int id;
    String url;
    float power;
    float all_time_energy;
    float total_energy;
    float total_energy_today;
    char timestring[20];
};