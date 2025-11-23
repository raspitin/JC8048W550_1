#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>

// Struttura per una singola fascia oraria
struct ScheduleSlot {
    uint8_t hour;
    uint8_t minute;
    float temp;
};

// Struttura per un giorno intero (4 fasce)
struct DailySchedule {
    ScheduleSlot slots[4];
};

struct AppConfig {
    char weatherKey[64] = "3737538a12e713a7bd87f314c9086bc2"; 
    char weatherCity[32] = "Rome";
    char weatherCountry[4] = "IT";
    char timezone[64] = "CET-1CEST,M3.5.0,M10.5.0/3";
    
    // Programma Settimanale (7 giorni)
    DailySchedule weekSchedule[7];
};

class ConfigManager {
public:
    AppConfig data;
    
    bool begin();
    bool loadConfig();
    bool saveConfig();
    void resetToDefault();
};

extern ConfigManager configManager;

#endif