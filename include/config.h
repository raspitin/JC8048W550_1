#ifndef CONFIG_H
#define CONFIG_H

// --- HARDWARE PIN ---
// Pin del relè LOCALE (sulla scheda del display, se presente)
// Se usi solo quello remoto, questo non influisce
#define RELAY_PIN -1  // Metti -1 se non usi un relè collegato fisicamente al display

// --- CONFIGURAZIONE RELÈ REMOTO (SLAVE) ---
// IP di Default (usato solo se l'auto-discovery fallisce)
#define DEFAULT_REMOTE_RELAY_IP "192.168.1.33" 

// Porta UDP per la ricerca automatica (Discovery)
#define DISCOVERY_PORT 9876
#define DISCOVERY_PACKET_CONTENT "RELAY_HERE_V1"

// --- LOGGING ---
#define SYSLOG_SERVER "192.168.1.100" // IP del tuo PC per i log
#define SYSLOG_PORT 514
#define DEVICE_NAME "Termostato_Smart"

// --- PARAMETRI TERMOSTATO ---
#define TEMP_HYSTERESIS 0.2 // Isteresi (es. accende a 19.8, spegne a 20.2)
#define TARGET_HEAT_ON  22.0
#define TARGET_HEAT_OFF 16.0

// --- ALTRI ---
#define RELAY_ACTIVE_LOW true // true se il relè scatta con LOW, false con HIGH

#endif