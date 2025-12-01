#ifndef CONFIG_H
#define CONFIG_H

// --- HARDWARE LOCALE ---
// Pin del relè sulla scheda (verifica lo schematico della JC8048W550, spesso usano IO4 o IO5 per espansioni)
#define RELAY_PIN           4  
// Se il modulo relè scatta con segnale LOW (comune), metti true
#define RELAY_ACTIVE_LOW    false 

// --- HARDWARE REMOTO ---
// IP dell'ESP-01S slave (se presente). Se non lo usi, lascia stringa vuota o commenta.
// Assicurati di aver assegnato un IP statico allo slave o di usare mDNS.
#define REMOTE_RELAY_IP     "192.168.1.33" 

// --- PARAMETRI TERMOSTATO ---
#define TEMP_HYSTERESIS     0.5
#define TARGET_HEAT_ON      22.0  // Target impostato dal tasto "Brr che freddo"
#define TARGET_HEAT_OFF     16.0  // Target impostato dal tasto "Che caldo"

#endif