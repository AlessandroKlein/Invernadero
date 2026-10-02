#pragma once
// Interfaz web embebida (secciones 60-68). Página única servida desde flash.
// Usa fetch() para la API REST y WebSocket para actualizaciones en tiempo real.

namespace gh {
namespace WebAssets {

static const char INDEX_HTML[] = R"rawliteral(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Invernadero</title>
<style>
:root{--bg:#0f172a;--card:#1e293b;--fg:#e2e8f0;--accent:#22c55e;--off:#94a3b8;--warn:#f59e0b;--err:#ef4444}
[data-theme="light"]{--bg:#f1f5f9;--card:#ffffff;--fg:#0f172a;--accent:#16a34a;--off:#64748b;--warn:#d97706;--err:#dc2626}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:var(--bg);color:var(--fg);padding:16px;max-width:640px;margin:auto}
h1{font-size:1.2rem;margin-bottom:4px}
.sub{color:var(--off);font-size:.85rem;margin-bottom:16px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:12px}
.card{background:var(--card);border-radius:12px;padding:14px}
.card .k{color:var(--off);font-size:.75rem;text-transform:uppercase;letter-spacing:.05em}
.card .v{font-size:1.4rem;font-weight:700;margin-top:4px}
.card .s{font-size:.7rem;color:var(--off)}
.act{display:flex;align-items:center;justify-content:space-between;background:var(--card);border-radius:10px;padding:10px 14px;margin-bottom:8px}
.act .name{font-weight:600}
.dot{width:12px;height:12px;border-radius:50%;background:var(--off)}
.dot.on{background:var(--accent);box-shadow:0 0 8px var(--accent)}
.dot.err{background:var(--err)}
section{margin-top:20px}
h2{font-size:1rem;margin-bottom:10px;color:var(--accent)}
button{background:var(--card);border:1px solid var(--accent);color:var(--accent);border-radius:8px;padding:8px 12px;cursor:pointer}
input{background:var(--card);border:1px solid var(--off);color:var(--fg);border-radius:8px;padding:8px;margin:4px 0;width:100%}
label{font-size:.8rem;color:var(--off)}
</style>
<script>try{document.documentElement.setAttribute('data-theme',localStorage.getItem('gh_theme')||'dark')}catch(e){}</script>
</head>
<body>
<h1 id="title">Invernadero</h1>
<div class="sub" id="subtitle">—</div>
<button id="themeBtn" onclick="toggleTheme()" style="margin-bottom:12px;margin-right:8px">🌙</button>
<button id="loginBtn" style="margin-bottom:12px" onclick="login()">Iniciar sesión</button>

<section><h2>Sensores</h2><div class="grid" id="sensors"></div></section>
<section><h2>Actuadores</h2><div id="actuators"></div></section>
<section><h2>Alarmas</h2><div id="alarms">Sin alarmas</div></section>
<section><h2>API / Servidor central</h2>
<div class="card">
<label>Token de API (autoriza control y cambios desde el servidor central)</label>
<input id="apiToken" readonly placeholder="No generado" style="margin:8px 0">
<button onclick="rotateToken()" style="margin-right:8px">Generar token</button>
<button onclick="revokeToken()">Revocar</button>
</div></section>

<script>
const api = '/api/v1';
let token = localStorage.getItem('gh_token') || '';
async function j(path, opts){ const r = await fetch(path, opts); return r.json(); }

function updateLoginUi(){
  const b = document.getElementById('loginBtn');
  b.textContent = token ? 'Cerrar sesión' : 'Iniciar sesión';
}

async function login(){
  if(token){ token=''; localStorage.removeItem('gh_token'); updateLoginUi(); return; }
  const u = prompt('Usuario','admin'); if(u===null) return;
  const p = prompt('Contraseña',''); if(p===null) return;
  const r = await fetch(api+'/auth/login',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({user:u,pass:p})});
  const d = await r.json();
  if(d.ok){ token = d.token; localStorage.setItem('gh_token', token); updateLoginUi(); }
  else alert('Credenciales inválidas');
}
updateLoginUi();

document.getElementById('themeBtn').textContent = (document.documentElement.getAttribute('data-theme')==='dark') ? '🌙' : '☀️';
function toggleTheme(){
  const n = document.documentElement.getAttribute('data-theme')==='dark' ? 'light' : 'dark';
  document.documentElement.setAttribute('data-theme', n);
  try{ localStorage.setItem('gh_theme', n); }catch(e){}
  document.getElementById('themeBtn').textContent = n==='dark' ? '🌙' : '☀️';
}

function statusColor(s){ return s===1?'var(--accent)':(s===2?'var(--warn)':'var(--err)'); }

async function loadSensors(){
  const s = await j(api+'/sensors');
  document.getElementById('sensors').innerHTML = s.map(x=>
    `<div class="card"><div class="k">${x.name}</div>
     <div class="v">${x.value.toFixed(1)}<small> ${x.unit||''}</small></div>
     <div class="s" style="color:${statusColor(x.status)}">estado ${x.status}</div></div>`).join('');
}

async function loadActuators(){
  const a = await j(api+'/actuators');
  document.getElementById('actuators').innerHTML = a.map(x=>
    `<div class="act"><span class="name">${x.name}</span>
     <span class="dot ${x.output>0?'on':''} ${x.fault?'err':''}"></span>
     <button onclick="setAct('${x.name}',${x.output>0?0:100})">${x.output>0?'OFF':'ON'}</button></div>`).join('');
}

async function setAct(name, val){
  // Mapeo simple: se asume rol por prefijo.
  const map={Bomba:'pump','Válvula':'valve','Ventilador':'fan','Extractor':'extractor',
             'Calefacción':'heater','Humidificador':'humidifier','Iluminación':'light','Alarma':'alarm'};
  let role='pump'; for(const k in map){ if(name.startsWith(k)){role=map[k];break;} }
  await fetch(api+'/actuators',{method:'POST',headers:{'Content-Type':'application/json','X-Auth-Token':token},
    body:JSON.stringify({role,index:0,output:val})});
  loadActuators();
}

async function loadAlarms(){
  const a = await j(api+'/alarms');
  document.getElementById('alarms').innerHTML = a.length
    ? a.map(x=>`<div class="card" style="margin-bottom:6px">[${x.sev>=3?'ERR':'WARN'}] ${x.msg}</div>`).join('')
    : 'Sin alarmas';
}

async function loadStatus(){
  const s = await j(api+'/status');
  document.getElementById('title').textContent = s.name || 'Invernadero';
  document.getElementById('subtitle').textContent = 'ID: '+(s.id||'-')+' · Modo: AUTO';
}

async function tokenStatus(){
  if(!token) return;
  try{
    const r = await fetch(api+'/token/status',{headers:{'X-Auth-Token':token}});
    if(r.ok){ const d = await r.json(); if(!d.configured) document.getElementById('apiToken').placeholder='No generado'; }
  }catch(e){}
}
async function rotateToken(){
  if(!token){ alert('Inicie sesión primero'); return; }
  const r = await fetch(api+'/token/rotate',{method:'POST',headers:{'X-Auth-Token':token}});
  const d = await r.json();
  if(d.ok) document.getElementById('apiToken').value = d.token;
  else alert('No autorizado');
}
async function revokeToken(){
  if(!token){ alert('Inicie sesión primero'); return; }
  const r = await fetch(api+'/token/revoke',{method:'POST',headers:{'X-Auth-Token':token}});
  if(r.ok){ document.getElementById('apiToken').value=''; document.getElementById('apiToken').placeholder='No generado'; }
  else alert('No autorizado');
}

loadStatus(); loadSensors(); loadActuators(); loadAlarms(); tokenStatus();
setInterval(()=>{loadSensors();loadActuators();loadAlarms();}, 3000);
</script>
</body></html>)rawliteral";

// Página "Pines y hardware" (solo accesible desde la IP del dispositivo, no por
// API). Muestra el mapa de pines por defecto (PinMap.hpp) y las direcciones I²C.
static const char PINS_HTML[] = R"rawliteral(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Invernadero · Pines y hardware</title>
<style>
:root{--bg:#0f172a;--card:#1e293b;--fg:#e2e8f0;--accent:#22c55e;--off:#94a3b8;--line:rgba(148,163,184,.16)}
body{font-family:system-ui,sans-serif;background:var(--bg);color:var(--fg);padding:16px;max-width:760px;margin:auto}
h1{font-size:1.2rem;margin-bottom:4px}
.sub{color:var(--off);font-size:.85rem;margin-bottom:18px}
h2{font-size:1rem;color:var(--accent);margin:20px 0 10px}
.table-wrap{overflow-x:auto;-webkit-overflow-scrolling:touch;border-radius:10px}
table{width:100%;border-collapse:collapse;font-size:.9rem}
th,td{text-align:left;padding:8px 10px;border-bottom:1px solid var(--line)}
th{color:var(--off);font-weight:600}
td:first-child{font-family:ui-monospace,monospace;font-size:.8rem}
.note{color:var(--off);font-size:.78rem;margin-top:8px}
@media(max-width:640px){
  table thead{display:none}
  table,table tbody,table tr,table td{display:block;width:100%}
  table tr{border:1px solid var(--line);border-radius:10px;margin-bottom:10px;padding:4px 10px;background:var(--card)}
  table td{border-bottom:none;padding:6px 4px;display:flex;justify-content:space-between;gap:12px;text-align:right}
  table td::before{content:attr(data-label);color:var(--off);font-family:ui-monospace,monospace;font-size:.66rem;text-transform:uppercase;letter-spacing:.05em;text-align:left}
}
</style>
</head>
<body>
<h1>Pines y hardware</h1>
<div class="sub">Mapa de pines por defecto (PinMap.hpp). Los pines se asignan por instalación y varían según sensores/expansores conectados.</div>

<h2>Buses y E/S</h2>
<div class="table-wrap"><table>
<thead><tr><th>Función</th><th>GPIO</th><th>Tipo</th><th>Notas</th></tr></thead>
<tbody>
<tr><td data-label="Función">I²C SDA</td><td data-label="GPIO">21</td><td data-label="Tipo">bidireccional</td><td data-label="Notas">pull-up 4,7 kΩ a 3,3 V</td></tr>
<tr><td data-label="Función">I²C SCL</td><td data-label="GPIO">22</td><td data-label="Tipo">bidireccional</td><td data-label="Notas">pull-up 4,7 kΩ a 3,3 V</td></tr>
<tr><td data-label="Función">74HC595 DATA</td><td data-label="GPIO">23</td><td data-label="Tipo">salida</td><td data-label="Notas">SPI bit-banged (DS)</td></tr>
<tr><td data-label="Función">74HC595 CLOCK</td><td data-label="GPIO">18</td><td data-label="Tipo">salida</td><td data-label="Notas">SHCP</td></tr>
<tr><td data-label="Función">74HC595 LATCH</td><td data-label="GPIO">5</td><td data-label="Tipo">salida</td><td data-label="Notas">STCP</td></tr>
<tr><td data-label="Función">SPI SCK/MISO/MOSI</td><td data-label="GPIO">18/19/23</td><td data-label="Tipo">SPI nativo</td><td data-label="Notas">MCP23S17/ADC/SD/W5500</td></tr>
<tr><td data-label="Función">1-Wire</td><td data-label="GPIO">4</td><td data-label="Tipo">datos</td><td data-label="Notas">pull-up 4,7 kΩ a 3,3 V</td></tr>
<tr><td data-label="Función">Caudalímetro</td><td data-label="GPIO">34</td><td data-label="Tipo">entrada</td><td data-label="Notas">solo entrada</td></tr>
<tr><td data-label="Función">Pluviómetro</td><td data-label="GPIO">35</td><td data-label="Tipo">entrada</td><td data-label="Notas">solo entrada</td></tr>
<tr><td data-label="Función">Anemómetro</td><td data-label="GPIO">36</td><td data-label="Tipo">entrada</td><td data-label="Notas">solo entrada</td></tr>
<tr><td data-label="Función">Tanque TRIG/ECHO</td><td data-label="GPIO">25/26</td><td data-label="Tipo">salida/entrada</td><td data-label="Notas">ultrasónico</td></tr>
<tr><td data-label="Función">Flotador bajo/alto</td><td data-label="GPIO">32/33</td><td data-label="Tipo">entrada</td><td data-label="Notas">pull-up interno</td></tr>
<tr><td data-label="Función">Parada de emergencia</td><td data-label="GPIO">27</td><td data-label="Tipo">entrada</td><td data-label="Notas">pull-up interno</td></tr>
<tr><td data-label="Función">RS485 RX/TX/DE</td><td data-label="GPIO">16/17/14</td><td data-label="Tipo">UART</td><td data-label="Notas">RO/DI/control dirección</td></tr>
</tbody>
</table></div>

<h2>Direcciones I²C</h2>
<div class="table-wrap"><table>
<thead><tr><th>Dispositivo</th><th>Dirección</th></tr></thead>
<tbody>
<tr><td data-label="Dispositivo">SHT31</td><td data-label="Dirección">0x44</td></tr>
<tr><td data-label="Dispositivo">AHT20</td><td data-label="Dirección">0x38</td></tr>
<tr><td data-label="Dispositivo">ADS1115</td><td data-label="Dirección">0x48–0x4B</td></tr>
<tr><td data-label="Dispositivo">BH1750</td><td data-label="Dirección">0x23</td></tr>
<tr><td data-label="Dispositivo">SCD40/SCD41</td><td data-label="Dirección">0x62</td></tr>
<tr><td data-label="Dispositivo">MCP23017 #1/#2</td><td data-label="Dirección">0x20 / 0x21</td></tr>
</tbody>
</table></div>
<p class="note">Evitar GPIO de strapping (0, 2, 12, 15 en ESP32 clásico). El mapa real se define en <code>include/core/PinMap.hpp</code>.</p>
</body></html>)rawliteral";

} // namespace WebAssets
} // namespace gh
