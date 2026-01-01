#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "web_interface.h"
#include "config_manager.h"
#include "thermostat.h"
#include "influx_manager.h" 

extern Thermostat thermo;

AsyncWebServer server(80);

const char* index_html = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Termostato Smart</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; margin:0; padding:0; background-color: #f4f4f9; color: #333; }
    .header { background-color: #007bff; color: white; padding: 15px; text-align: center; }
    .tabs { display: flex; background: #ddd; }
    .tab { flex: 1; padding: 15px; text-align: center; cursor: pointer; border-bottom: 3px solid transparent; }
    .tab.active { border-bottom: 3px solid #007bff; background: #fff; font-weight: bold; }
    .content { padding: 20px; display: none; }
    .content.active { display: block; }
    .card { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-bottom: 20px; }
    h2 { margin-top: 0; }
    label { display: block; margin-top: 10px; font-weight: bold; }
    input, select { width: 100%; padding: 8px; margin-top: 5px; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; }
    button { background-color: #007bff; color: white; border: none; padding: 10px 20px; cursor: pointer; border-radius: 4px; margin-top: 15px; }
    button:hover { background-color: #0056b3; }
    .schedule-grid { display: grid; grid-template-columns: 50px repeat(24, 1fr); gap: 2px; overflow-x: auto; }
    .sched-cell { height: 30px; border: 1px solid #eee; text-align: center; font-size: 10px; line-height: 30px; cursor: pointer; }
    .sched-header { font-weight: bold; background: #eee; }
    .sched-day { font-weight: bold; background: #eee; line-height: 30px; padding-left: 5px; }
    .mode-heat { background-color: #ffcccc; } 
    .mode-off { background-color: #f0f0f0; }
  </style>
</head>
<body>
  <div class="header"><h1>Termostato Control</h1></div>
  <div class="tabs">
    <div class="tab active" onclick="openTab('dashboard')">Dashboard</div>
    <div class="tab" onclick="openTab('schedule')">Impegni</div>
    <div class="tab" onclick="openTab('setup')">Setup</div>
  </div>

  <div id="dashboard" class="content active">
    <div class="card">
      <h2>Stato Attuale</h2>
      <p>Temperatura: <span id="val_temp">--</span> &deg;C</p>
      <p>Target: <span id="val_target">--</span> &deg;C</p>
      <p>Umidit&agrave;: <span id="val_hum">--</span> %</p>
      <p>Pressione: <span id="val_press">--</span> hPa</p>
      <p>Stato Rel&egrave;: <span id="val_relay">--</span></p>
    </div>
  </div>

  <div id="schedule" class="content">
    <div class="card">
      <h2>Programmazione Settimanale</h2>
      <p>Clicca per attivare/disattivare (Rosso = Comfort)</p>
      <div id="grid_container" class="schedule-grid"></div>
      <button onclick="saveSchedule()">Salva Programmazione</button>
    </div>
  </div>

  <div id="setup" class="content">
    <div class="card">
      <h2>InfluxDB Config</h2>
      <label>Server URL:</label> <input type="text" id="inf_url">
      <label>Organization:</label> <input type="text" id="inf_org">
      <label>Bucket:</label> <input type="text" id="inf_bucket">
      <label>Token:</label> <input type="password" id="inf_token">
    </div>
    <div class="card">
      <h2>API & Calibrazione</h2>
      <label>OpenWeather API Key:</label> <input type="text" id="api_key">
      <label>Temp Offset:</label> <input type="number" step="0.1" id="temp_offset">
      <button onclick="saveConfig()">Salva Configurazione</button>
    </div>
  </div>

<script>
  function openTab(t) {
    var x=document.getElementsByClassName("content"); for(var i=0;i<x.length;i++)x[i].style.display="none";
    var l=document.getElementsByClassName("tab"); for(var i=0;i<l.length;i++)l[i].className=l[i].className.replace(" active","");
    document.getElementById(t).style.display="block"; event.currentTarget.className+=" active";
  }

  function fetchStatus() {
    fetch('/api/status').then(r=>r.json()).then(d=>{
      document.getElementById('val_temp').innerText=d.temp;
      document.getElementById('val_target').innerText=d.target;
      document.getElementById('val_hum').innerText=d.hum;
      document.getElementById('val_press').innerText=d.press;
      document.getElementById('val_relay').innerText=d.relay?"ON":"OFF";
    });
  }

  function fetchConfig() {
    fetch('/api/config').then(r=>r.json()).then(d=>{
      document.getElementById('inf_url').value=d.influxUrl;
      document.getElementById('inf_org').value=d.influxOrg;
      document.getElementById('inf_bucket').value=d.influxBucket;
      document.getElementById('inf_token').value=d.influxToken;
      document.getElementById('api_key').value=d.weatherKey;
      document.getElementById('temp_offset').value=d.tempOffset;
    });
  }

  function saveConfig() {
    var d={
      influxUrl:document.getElementById('inf_url').value,
      influxOrg:document.getElementById('inf_org').value,
      influxBucket:document.getElementById('inf_bucket').value,
      influxToken:document.getElementById('inf_token').value,
      weatherKey:document.getElementById('api_key').value,
      tempOffset:parseFloat(document.getElementById('temp_offset').value)
    };
    fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(d)})
    .then(r=>{alert("Salvato! Riavvio...");});
  }
  // ... (Codice JS Schedule rimasto invariato per brevità) ...
  // Per l'inizializzazione:
  function initGrid() { /* ... codice esistente ... */ fetchSched(); }
  function fetchSched() { /* ... codice esistente ... */ }
  function saveSchedule() { /* ... codice esistente ... */ }

  initGrid(); fetchStatus(); fetchConfig(); setInterval(fetchStatus,5000);
</script>
</body></html>
)rawliteral";

void setup_web_server() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        DynamicJsonDocument doc(512);
        doc["temp"] = thermo.getCurrentTemp();
        doc["hum"] = thermo.getHumidity();
        doc["press"] = thermo.getPressure(); // NUOVO
        doc["target"] = thermo.getTarget();
        doc["relay"] = thermo.isHeatingState();
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request){
        DynamicJsonDocument doc(1024);
        doc["ssid"] = WiFi.SSID();
        doc["ip"] = WiFi.localIP().toString();
        doc["weatherKey"] = configManager.data.weatherKey;
        doc["influxUrl"] = configManager.data.influxUrl;
        doc["influxOrg"] = configManager.data.influxOrg;
        doc["influxBucket"] = configManager.data.influxBucket;
        doc["influxToken"] = configManager.data.influxToken;
        doc["tempOffset"] = configManager.data.tempOffset;
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        if(request->url() == "/api/config") {
            StaticJsonDocument<1024> doc;
            deserializeJson(doc, data);
            strlcpy(configManager.data.weatherKey, doc["weatherKey"] | "", sizeof(configManager.data.weatherKey));
            strlcpy(configManager.data.influxUrl, doc["influxUrl"] | "", sizeof(configManager.data.influxUrl));
            strlcpy(configManager.data.influxOrg, doc["influxOrg"] | "", sizeof(configManager.data.influxOrg));
            strlcpy(configManager.data.influxBucket, doc["influxBucket"] | "", sizeof(configManager.data.influxBucket));
            strlcpy(configManager.data.influxToken, doc["influxToken"] | "", sizeof(configManager.data.influxToken));
            configManager.data.tempOffset = doc["tempOffset"];

            if(configManager.saveConfig()) {
                request->send(200, "text/plain", "OK");
                delay(1000);
                ESP.restart(); 
            } else {
                request->send(500, "text/plain", "Save Failed");
            }
        }
        if(request->url() == "/api/schedule") {
             // ... Logica salvataggio schedule (invariata) ...
            request->send(200, "text/plain", "Schedule Saved");
        }
    });

    server.on("/api/schedule", HTTP_GET, [](AsyncWebServerRequest *request){
         DynamicJsonDocument doc(4096);
         JsonArray sched = doc.createNestedArray("schedule");
         for(int d=0; d<7; d++) {
             sched.add(configManager.data.weekSchedule[d].timeSlots);
         }
         String response;
         serializeJson(doc, response);
         request->send(200, "application/json", response);
    });

    server.begin();
}