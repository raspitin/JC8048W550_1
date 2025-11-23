#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>

struct AppConfig {
    char weatherKey[64] = "3737538a12e713a7bd87f314c9086bc2"; // Valore di default
    char weatherCity[32] = "Rome";
    char weatherCountry[4] = "IT";
    char timezone[64] = "CET-1CEST,M3.5.0,M10.5.0/3"; // Default Italia
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