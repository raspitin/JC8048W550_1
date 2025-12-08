#include "web_interface.h"
#include "config_manager.h"
#include "thermostat.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>

extern Thermostat thermo;
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <title>Termostato Smart</title>
  <style>
    body { font-family: 'Segoe UI', Arial, sans-serif; margin:0; padding:0; background-color: #121212; color: #e0e0e0; }
    .header { background-color: #2c3e50; padding: 15px; text-align: center; font-size: 22px; font-weight: bold; color: #ecf0f1; border-bottom: 3px solid #e67e22; }
    .container { padding: 15px; max-width: 600px; margin: auto; }
    
    .card { background-color: #1e1e1e; border-radius: 12px; padding: 20px; margin-bottom: 20px; box-shadow: 0 4px 10px rgba(0,0,0,0.5); border: 1px solid #333; position: relative; }
    h2 { border-bottom: 1px solid #444; padding-bottom: 10px; margin-top: 0; color: #f39c12; font-size: 18px; text-transform: uppercase; letter-spacing: 1px; }
    
    /* STATUS BOX */
    .status-box { display: flex; justify-content: space-between; align-items: center; }
    .temp-wrapper { text-align: left; }
    .temp-label { font-size: 12px; color: #aaa; text-transform: uppercase; }
    .temp-large { font-size: 48px; font-weight: bold; color: #fff; line-height: 1; }
    .status-right { text-align: right; }
    .badge { padding: 6px 12px; border-radius: 6px; font-weight: bold; font-size: 14px; display: inline-block; }
    .badge-on { background-color: #e74c3c; color: white; box-shadow: 0 0 10px rgba(231, 76, 60, 0.4); }
    .badge-off { background-color: #7f8c8d; color: white; }
    .relay-msg { font-size: 11px; margin-top: 5px; display: block; }
    
    /* BUTTONS & INPUTS */
    button { border: none; padding: 12px; border-radius: 6px; cursor: pointer; font-size: 16px; font-weight: bold; width: 100%; transition: background 0.2s; }
    button:active { transform: scale(0.98); }
    button:disabled { background-color: #555 !important; color: #aaa !important; cursor: not-allowed; }
    
    input, select { width: 100%; padding: 12px; margin-bottom: 10px; border-radius: 6px; border: 1px solid #444; background: #2c2c2c; color: white; box-sizing: border-box; font-size: 16px; }
    input:focus, select:focus { border-color: #e67e22; outline: none; }

    /* BOOST SECTION */
    .boost-controls { display: flex; align-items: center; gap: 10px; margin-bottom: 15px; background: #252525; padding: 10px; border-radius: 8px; }
    .btn-circle { width: 50px; height: 50px; border-radius: 50%; font-size: 24px; padding: 0; display: flex; align-items: center; justify-content: center; background: #34495e; color: white; }
    .boost-display { flex-grow: 1; text-align: center; font-size: 24px; font-weight: bold; color: #ecf0f1; }
    .btn-start { background-color: #e67e22; color: white; }
    .btn-stop { background-color: #c0392b; color: white; animation: pulse 2s infinite; }
    
    @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.8; } 100% { opacity: 1; } }

    /* IMPEGNI SECTION */
    .imp-row { display: flex; gap: 5px; margin-bottom: 10px; }
    .imp-date { flex: 2; }
    .imp-time { flex: 1; }
    .imp-desc { flex: 3; }
    
    ul { list-style-type: none; padding: 0; margin: 0; }
    li { background: #333; padding: 12px; margin-bottom: 8px; border-radius: 6px; display: flex; justify-content: space-between; align-items: center; border-left: 4px solid #e67e22; }
    .btn-del { background-color: transparent; color: #e74c3c; width: auto; padding: 5px; font-size: 18px; margin: 0; }

    /* SCHEDULE SECTION */
    .schedule-container { user-select: none; }
    .slot-grid { display: grid; grid-template-columns: repeat(48, 1fr); gap: 1px; height: 50px; background: #333; padding: 2px; border-radius: 4px; margin-bottom: 5px; touch-action: none; }
    .slot { background-color: #222; cursor: pointer; }
    .slot.active { background-color: #27ae60; }
    
    .timeline-labels { display: flex; justify-content: space-between; padding: 0 2px; margin-bottom: 5px; }
    .tl-label { font-size: 10px; color: #888; width: 20px; text-align: center; }
    
    /* NUOVO: Label selezione e riepilogo orari */
    #selectionLabel { text-align: center; font-size: 14px; color: #e67e22; height: 20px; font-weight: bold; margin-bottom: 5px; }
    #scheduleSummary { font-size: 13px; color: #ccc; background: #252525; padding: 10px; border-radius: 5px; min-height: 20px; margin-bottom: 10px; }

    .btn-save { background-color: #27ae60; margin-top: 10px; }

    /* MODAL COPY */
    .modal { display: none; position: fixed; z-index: 1000; left: 0; top: 0; width: 100%; height: 100%; background-color: rgba(0,0,0,0.8); }
    .modal-content { background-color: #2d2d2d; margin: 15% auto; padding: 20px; border: 1px solid #888; width: 80%; max-width: 400px; border-radius: 10px; text-align: center; }
    .copy-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; text-align: left; margin: 20px 0; }
    /* Fix allineamento checkbox */
    .copy-grid label { display: flex; align-items: center; gap: 8px; cursor: pointer; }
    .copy-checkbox { transform: scale(1.3); margin: 0; width: auto; }
  </style>
</head>
<body>
  <div class="header">Termostato Smart</div>
  <div class="container">
    
    <div class="card">
      <div class="status-box">
        <div class="temp-wrapper">
            <div class="temp-label">Temperatura</div>
            <div class="temp-large"><span id="currTemp">--</span>&deg;</div>
            <div style="font-size: 12px; color: #888;">Target: <span id="targetTemp">--</span>&deg;</div>
            <div style="font-size: 16px; color: #27ae60; font-weight: bold; margin-bottom: 5px;">
        💧 <span id="currHum">--</span>%
    </div>
        </div>
        <div class="status-right">
             <span id="relayStatus" class="badge badge-off">OFF</span>
             <span id="relayMsg" class="relay-msg" style="color: #555;">--</span>
        </div>
      </div>

      <hr style="border: 0; border-top: 1px solid #444; margin: 20px 0;">

      <div id="boostPanel">
        <div style="margin-bottom: 10px; font-size: 14px; color: #aaa; text-transform: uppercase;">Start Temporizzato</div>
        
        <div id="boostSelector" class="boost-controls">
            <button class="btn-circle" onclick="adjBoost(-30)">-</button>
            <div class="boost-display"><span id="boostMin">30</span> <span style="font-size: 14px; color:#aaa">min</span></div>
            <button class="btn-circle" onclick="adjBoost(30)">+</button>
        </div>
        
        <button id="btnAction" class="btn-start" onclick="toggleBoost()">AVVIA BOOST</button>
        <div id="boostError" style="display:none; color: #e74c3c; font-size: 12px; text-align: center; margin-top: 5px;">⚠️ Relè Offline: Impossibile avviare</div>
      </div>
    </div>

    <div class="card">
      <h2>🕒 Programmazione</h2>
      <div class="schedule-container">
          <select id="daySelect" onchange="loadDaySchedule()">
            <option value="1">Lunedì</option>
            <option value="2">Martedì</option>
            <option value="3">Mercoledì</option>
            <option value="4">Giovedì</option>
            <option value="5">Venerdì</option>
            <option value="6">Sabato</option>
            <option value="0">Domenica</option>
          </select>
          
          <div id="selectionLabel">Trascina per modificare</div>
          <div id="scheduleGrid" class="slot-grid"></div>
          
          <div class="timeline-labels">
            <span class="tl-label">00</span>
            <span class="tl-label">04</span>
            <span class="tl-label">08</span>
            <span class="tl-label">12</span>
            <span class="tl-label">16</span>
            <span class="tl-label">20</span>
            <span class="tl-label">24</span>
          </div>

          <div id="scheduleSummary">Caricamento orari...</div>

          <button class="btn-save" onclick="openCopyModal()">SALVA PROGRAMMA</button>
      </div>
    </div>

    <div class="card">
      <h2>📅 Impegni Famiglia</h2>
      <div class="imp-row">
        <input type="date" id="impDate" class="imp-date">
        <input type="time" id="impTime" class="imp-time">
      </div>
      <div class="imp-row">
        <input type="text" id="impDesc" class="imp-desc" placeholder="Descrizione (es. Cena fuori)">
      </div>
      <button class="btn-boost" onclick="addImpegno()">AGGIUNGI IMPEGNO</button>
      
      <ul id="listImpegni" style="margin-top: 15px;">Caricamento...</ul>
    </div>

  </div>

  <div id="copyModal" class="modal">
    <div class="modal-content">
      <h3>Salvataggio Completato</h3>
      <p>Vuoi copiare questo orario anche su altri giorni?</p>
      <div class="copy-grid">
        <label><input type="checkbox" class="copy-checkbox" value="1"> Lunedì</label>
        <label><input type="checkbox" class="copy-checkbox" value="2"> Martedì</label>
        <label><input type="checkbox" class="copy-checkbox" value="3"> Mercoledì</label>
        <label><input type="checkbox" class="copy-checkbox" value="4"> Giovedì</label>
        <label><input type="checkbox" class="copy-checkbox" value="5"> Venerdì</label>
        <label><input type="checkbox" class="copy-checkbox" value="6"> Sabato</label>
        <label><input type="checkbox" class="copy-checkbox" value="0"> Domenica</label>
      </div>
      <div style="display:flex; gap:10px;">
        <button class="btn-start" onclick="confirmCopy()">COPIA & CHIUDI</button>
        <button class="btn-stop" onclick="closeModal()">SOLO CORRENTE</button>
      </div>
    </div>
  </div>

<script>
let weekData = [];
let impegniList = [];
let boostValue = 30;
let relayOnline = false;
let isBoostRunning = false;
let currentScheduleBuffer = {h:0, l:0}; 

// Variabili per il drag
let isDrawing = false;
let startSlot = -1;
let currentMode = false; // true = accendi, false = spegni

// --- CORE FUNCTIONS ---
function fetchStatus() {
  fetch('/api/status').then(r => r.json()).then(d => {
    document.getElementById('currTemp').innerText = d.temp.toFixed(1);
    document.getElementById('currHum').innerText = d.humidity.toFixed(0);
    document.getElementById('targetTemp').innerText = d.target.toFixed(1);
    const rStat = document.getElementById('relayStatus');
    const rMsg = document.getElementById('relayMsg');
    relayOnline = d.relayOnline;
    isBoostRunning = d.boostActive;

    if(d.heating) { rStat.className = 'badge badge-on'; rStat.innerText = 'ON'; } 
    else { rStat.className = 'badge badge-off'; rStat.innerText = 'OFF'; }

    if (d.relayOnline) {
        rMsg.innerText = "Relè Connesso"; rMsg.style.color = "#27ae60";
        document.getElementById('boostError').style.display = 'none';
    } else {
        rMsg.innerText = "⚠️ RELÈ OFFLINE"; rMsg.style.color = "#e74c3c";
    }
    updateBoostUI(d);
  });
}

function updateBoostUI(data) {
    const btn = document.getElementById('btnAction');
    const selector = document.getElementById('boostSelector');
    if (!relayOnline) {
        btn.disabled = true; btn.innerText = "RELÈ OFFLINE"; btn.className = "btn-stop"; btn.style.opacity = "0.5";
        document.getElementById('boostError').style.display = 'block';
        return;
    }
    btn.disabled = false; btn.style.opacity = "1";
    if (data.boostActive) {
        let minRem = Math.round(data.boostRem / 60);
        btn.innerText = "STOP BOOST (" + minRem + "m)"; btn.className = "btn-stop";
        selector.style.opacity = "0.3"; selector.style.pointerEvents = "none";
    } else {
        btn.innerText = "AVVIA BOOST (" + boostValue + "m)"; btn.className = "btn-start";
        selector.style.opacity = "1"; selector.style.pointerEvents = "auto";
    }
}

function adjBoost(delta) {
    boostValue += delta;
    if (boostValue < 30) boostValue = 30;
    if (boostValue > 480) boostValue = 480; 
    document.getElementById('boostMin').innerText = boostValue;
    if (!isBoostRunning) document.getElementById('btnAction').innerText = "AVVIA BOOST (" + boostValue + "m)";
}

function toggleBoost() {
    if (!relayOnline) return;
    if (isBoostRunning) fetch('/api/boost?stop=true', {method:'POST'}).then(fetchStatus);
    else fetch('/api/boost?min='+boostValue, {method:'POST'}).then(fetchStatus);
}

// --- PROGRAMMAZIONE ---
function initGrid() {
    const grid = document.getElementById('scheduleGrid');
    grid.innerHTML = '';
    
    // Gestione Drag & Drop globale sulla griglia
    grid.onmouseleave = function() { isDrawing = false; updateSelectionLabel(-1, -1); };
    grid.onmouseup = function() { isDrawing = false; updateSelectionLabel(-1, -1); updateScheduleText(); };

    for(let i=0; i<48; i++) {
        let d = document.createElement('div');
        d.className = 'slot';
        d.id = 'slot-'+i;
        d.dataset.idx = i;
        
        // Logica Mouse/Touch
        d.onmousedown = function(e) {
            e.preventDefault();
            isDrawing = true;
            startSlot = i;
            // Inverti lo stato del primo slot cliccato e usa quello come target per il trascinamento
            let isActive = this.classList.contains('active');
            currentMode = !isActive;
            setSlot(i, currentMode);
            updateSelectionLabel(i, i);
        };
        
        d.onmouseenter = function() {
            if(isDrawing) {
                // Riempi dal startSlot a questo
                let start = Math.min(startSlot, i);
                let end = Math.max(startSlot, i);
                for(let k=start; k<=end; k++) {
                    setSlot(k, currentMode);
                }
                updateSelectionLabel(start, end);
            }
        };

        // Supporto Touch base
        d.ontouchstart = d.onmousedown;
        // (Il touch move richiede calcolo coordinate, semplificato qui con click)

        grid.appendChild(d);
    }
}

function setSlot(idx, state) {
    let el = document.getElementById('slot-'+idx);
    if(state) el.classList.add('active');
    else el.classList.remove('active');
}

function updateSelectionLabel(start, end) {
    let lbl = document.getElementById('selectionLabel');
    if(start === -1) {
        lbl.innerText = "Trascina per modificare";
        return;
    }
    let t1 = slotToTime(start);
    let t2 = slotToTime(end + 1);
    lbl.innerText = "Selezione: " + t1 + " - " + t2;
}

function slotToTime(slot) {
    let h = Math.floor(slot/2);
    let m = (slot % 2) * 30;
    return (h<10?'0':'')+h + ':' + (m<10?'0':'')+m;
}

function loadSchedule() {
    fetch('/api/schedule').then(r => r.json()).then(d => {
        weekData = d;
        loadDaySchedule();
    });
}

function loadDaySchedule() {
    let day = document.getElementById('daySelect').value;
    let dayIdx = parseInt(day);
    let slots = weekData[dayIdx]; 
    if (!slots) return;
    let high = slots.h; let low = slots.l; 
    for(let i=0; i<32; i++) {
        let active = (low & (1 << i)) !== 0;
        setSlot(i, active);
    }
    for(let i=0; i<16; i++) {
        let active = (high & (1 << i)) !== 0;
        setSlot(32+i, active);
    }
    updateScheduleText();
}

function updateScheduleText() {
    // Genera riepilogo testuale dalle classi 'active'
    let summary = "";
    let start = -1;
    let hasIntervals = false;
    
    for(let i=0; i<=48; i++) {
        let active = false;
        if (i < 48) {
            let el = document.getElementById('slot-'+i);
            if (el && el.classList.contains('active')) active = true;
        }

        if (active && start === -1) {
            start = i;
        } else if (!active && start !== -1) {
            if (hasIntervals) summary += ", ";
            summary += slotToTime(start) + "-" + slotToTime(i);
            hasIntervals = true;
            start = -1;
        }
    }
    
    if (!hasIntervals) summary = "Nessuna fascia oraria impostata";
    document.getElementById('scheduleSummary').innerText = summary;
}

function openCopyModal() {
    let low = 0; let high = 0;
    for(let i=0; i<32; i++) if(document.getElementById('slot-'+i).classList.contains('active')) low |= (1 << i);
    for(let i=0; i<16; i++) if(document.getElementById('slot-'+(32+i)).classList.contains('active')) high |= (1 << i);
    
    currentScheduleBuffer = {h: high, l: low};

    let currentDay = parseInt(document.getElementById('daySelect').value);
    saveDayAPI(currentDay, high, low);
    weekData[currentDay] = {h: high, l: low}; 

    document.querySelectorAll('.copy-checkbox').forEach(cb => {
        cb.checked = false;
        if(parseInt(cb.value) === currentDay) cb.disabled = true; 
        else cb.disabled = false;
    });

    document.getElementById('copyModal').style.display = 'block';
}

function confirmCopy() {
    let checkboxes = document.querySelectorAll('.copy-checkbox:checked');
    checkboxes.forEach(cb => {
        let day = parseInt(cb.value);
        saveDayAPI(day, currentScheduleBuffer.h, currentScheduleBuffer.l);
        weekData[day] = currentScheduleBuffer; 
    });
    closeModal();
}

function closeModal() {
    document.getElementById('copyModal').style.display = 'none';
}

function saveDayAPI(day, h, l) {
    fetch('/api/schedule', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({day: day, h: h, l: l})
    });
}

// --- IMPEGNI ---
function loadImpegni() {
    fetch('/api/impegni').then(r => r.json()).then(d => {
        impegniList = d;
        renderImpegni();
    });
}

function renderImpegni() {
    const ul = document.getElementById('listImpegni');
    ul.innerHTML = '';
    if (impegniList.length === 0) {
        ul.innerHTML = '<li style="color:#777; background:none; border:none;">Nessun impegno.</li>';
        return;
    }
    impegniList.forEach((imp, idx) => {
        let li = document.createElement('li');
        // Visualizza solo la stringa salvata (che ora conterrà l'anno per il backend ma qui è solo testo)
        // La formattazione la fa chi salva
        li.innerHTML = `<span>${imp}</span> <button class="btn-del" onclick="delImpegno(${idx})">&times;</button>`;
        ul.appendChild(li);
    });
}

function addImpegno() {
    let d = document.getElementById('impDate').value; // yyyy-mm-dd
    let t = document.getElementById('impTime').value;
    let desc = document.getElementById('impDesc').value;
    if(!d || !t || !desc) { alert("Compila tutti i campi!"); return; }
    
    // Converti yyyy-mm-dd in formato gestibile
    // Salviamo in formato: "YYYY/MM/DD HH:MM - Descrizione"
    // Questo formato è facile da parsare in C++ per la cancellazione e l'ordinamento
    // Nota: sostituire '-' con '/' nella data per evitare conflitti con il separatore ' - '
    let dateStr = d.replace(/-/g, '/');
    let fullStr = `${dateStr} ${t} - ${desc}`;
    
    impegniList.push(fullStr);
    // Ordina per data (semplice sort stringa funziona perché YYYY/MM/DD è ordinabile)
    impegniList.sort();
    
    saveImpegni();
    document.getElementById('impDesc').value = '';
}

function delImpegno(idx) {
    if(confirm("Eliminare impegno?")) {
        impegniList.splice(idx, 1);
        saveImpegni();
    }
}

function saveImpegni() {
    renderImpegni();
    fetch('/api/impegni', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(impegniList)
    });
}

// INIT
initGrid();
setInterval(fetchStatus, 2000);
fetchStatus();
loadSchedule();
loadImpegni();
document.getElementById('impDate').valueAsDate = new Date();

</script>
</body></html>
)rawliteral";

void handleRoot(AsyncWebServerRequest *request) { request->send(200, "text/html", index_html); }

void handleGetStatus(AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"temp\":" + String(thermo.getCurrentTemp()) + ",";
    json += "\"humidity\":" + String(thermo.getHumidity()) + ",";
    json += "\"target\":" + String(thermo.getTarget()) + ",";
    json += "\"heating\":" + String(thermo.isHeatingState() ? "true" : "false") + ",";
    json += "\"boostActive\":" + String(thermo.isBoostActive() ? "true" : "false") + ",";
    json += "\"boostRem\":" + String(thermo.getBoostRemainingSeconds()) + ",";
    json += "\"relayOnline\":" + String(thermo.isRelayOnline() ? "true" : "false");
    json += "}";
    request->send(200, "application/json", json);
}

void handleSetBoost(AsyncWebServerRequest *request) {
    if (request->hasParam("stop")) thermo.stopBoost();
    else if (request->hasParam("min")) thermo.startBoost(request->getParam("min")->value().toInt());
    request->send(200, "text/plain", "OK");
}

void handleGetSchedule(AsyncWebServerRequest *request) {
    String json = "[";
    for(int i=0; i<7; i++) {
        uint64_t slots = configManager.data.weekSchedule[i].timeSlots;
        uint32_t h = (uint32_t)(slots >> 32);
        uint32_t l = (uint32_t)(slots & 0xFFFFFFFF);
        json += "{\"h\":" + String(h) + ",\"l\":" + String(l) + "}";
        if(i < 6) json += ",";
    }
    json += "]";
    request->send(200, "application/json", json);
}

void handleSetSchedule(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    static char jsonBuffer[512];
    if(len >= sizeof(jsonBuffer)) return;
    memcpy(jsonBuffer, data, len);
    jsonBuffer[len] = 0;
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, jsonBuffer);
    int day = doc["day"];
    if (day >= 0 && day < 7) {
        uint32_t h = doc["h"];
        uint32_t l = doc["l"];
        configManager.data.weekSchedule[day].timeSlots = ((uint64_t)h << 32) | l;
        configManager.saveConfig();
    }
    request->send(200, "text/plain", "OK");
}

void handleGetImpegni(AsyncWebServerRequest *request) {
    if (LittleFS.exists("/impegni.json")) request->send(LittleFS, "/impegni.json", "application/json");
    else request->send(200, "application/json", "[]");
}

void handleSetImpegni(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    File file = LittleFS.open("/impegni.json", "w");
    if(file) {
        file.write(data, len);
        file.close();
    }
    request->send(200, "text/plain", "OK");
}

void setup_web_server() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleGetStatus);
    server.on("/api/boost", HTTP_POST, handleSetBoost);
    server.on("/api/schedule", HTTP_GET, handleGetSchedule);
    server.on("/api/impegni", HTTP_GET, handleGetImpegni);
    server.on("/api/schedule", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, handleSetSchedule);
    server.on("/api/impegni", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, handleSetImpegni);
    server.begin();
    Serial.println("Web Server Avviato");
}