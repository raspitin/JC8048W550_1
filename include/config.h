#ifndef CONFIG_H
#define CONFIG_H

// --- HARDWARE PIN ---
// Metti -1 se non hai un relè collegato fisicamente al display
#define RELAY_PIN -1  

// --- CONFIGURAZIONE RELÈ REMOTO ---
// IP di fallback se l'auto-discovery fallisce
#define DEFAULT_REMOTE_RELAY_IP "192.168.1.33" 

// Configurazione Discovery UDP
#define DISCOVERY_PORT 9876
#define DISCOVERY_PACKET_CONTENT "RELAY_HERE_V1"

// --- LOGGING ---
#define SYSLOG_SERVER "192.168.1.100" 
#define SYSLOG_PORT 514
#define DEVICE_NAME "Termostato_Smart"

// --- PARAMETRI TERMOSTATO ---
#define TEMP_HYSTERESIS 0.2
#define TARGET_HEAT_ON  22.0
#define TARGET_HEAT_OFF 16.0

// --- ALTRI ---
#define RELAY_ACTIVE_LOW true 

#endif