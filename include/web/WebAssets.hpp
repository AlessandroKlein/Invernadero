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
<title>Invernadero · Pines</title>
<style>
:root{--bg:#0f172a;--card:#1e293b;--fg:#e2e8f0;--accent:#22c55e;--off:#94a3b8;--line:rgba(148,163,184,.16)}
body{font-family:system-ui,sans-serif;background:var(--bg);color:var(--fg);padding:16px;max-width:760px;margin:auto}
h1{font-size:1.2rem;margin-bottom:4px}
.sub{color:var(--off);font-size:.85rem;margin-bottom:18px}
h2{font-size:1rem;color:var(--accent);margin:20px 0 10px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(180px,1fr));gap:10px}
label{display:flex;flex-direction:column;font-size:.8rem;color:var(--off);gap:4px}
input{background:var(--card);border:1px solid var(--line);color:var(--fg);border-radius:8px;padding:8px;font-size:.9rem;font-family:ui-monospace,monospace}
button{background:var(--accent);color:#06250f;border:0;border-radius:8px;padding:12px 16px;font-weight:700;font-size:.9rem;margin-top:18px;cursor:pointer}
.note{color:var(--off);font-size:.78rem;margin-top:12px}
#st{margin-top:10px;font-size:.85rem}
.ok{color:var(--accent)}.err{color:#ef4444}
</style>
</head>
<body>
<h1>Configuración de pines</h1>
<div class="sub">Editable sin recompilar. Se guarda en NVS y requiere reinicio para aplicar.</div>
<form id="f">
  <h2>Bus I²C</h2>
  <div class="grid">
    <label>SDA<input name="i2c_sda" type="number"></label>
    <label>SCL<input name="i2c_scl" type="number"></label>
    <label>Frecuencia (Hz)<input name="i2c_freq" type="number"></label>
  </div>
  <h2>74HC595 (SPI bit-banged)</h2>
  <div class="grid">
    <label>MOSI (DATA)<input name="hc595_mosi" type="number"></label>
    <label>SCLK (CLOCK)<input name="hc595_sclk" type="number"></label>
    <label>LATCH<input name="hc595_latch" type="number"></label>
    <label>Registros<input name="hc595_count" type="number"></label>
  </div>
  <h2>1-Wire</h2>
  <div class="grid"><label>Pin DS18B20<input name="onewire" type="number"></label></div>
  <h2>Entradas de pulsos</h2>
  <div class="grid">
    <label>Caudalímetro<input name="flow_pin" type="number"></label>
    <label>Pluviómetro<input name="rain_pin" type="number"></label>
    <label>Anemómetro<input name="wind_pin" type="number"></label>
  </div>
  <h2>Tanque</h2>
  <div class="grid">
    <label>Ultrasónico TRIG<input name="tank_trig" type="number"></label>
    <label>Ultrasónico ECHO<input name="tank_echo" type="number"></label>
    <label>Flotador bajo<input name="float_low" type="number"></label>
    <label>Flotador alto<input name="float_high" type="number"></label>
  </div>
  <h2>Seguridad</h2>
  <div class="grid"><label>Parada emergencia<input name="emergency_stop" type="number"></label></div>
  <h2>RS485 / Modbus</h2>
  <div class="grid">
    <label>RX<input name="rs485_rx" type="number"></label>
    <label>TX<input name="rs485_tx" type="number"></label>
    <label>DE/RE<input name="rs485_de" type="number"></label>
  </div>
  <h2>SPI nativo</h2>
  <div class="grid">
    <label>SCK<input name="spi_sck" type="number"></label>
    <label>MISO<input name="spi_miso" type="number"></label>
    <label>MOSI<input name="spi_mosi" type="number"></label>
  </div>
  <h2>Direcciones I²C (decimal)</h2>
  <div class="grid">
    <label>SHT31<input name="i2c_addr_sht31" type="number"></label>
    <label>AHT20<input name="i2c_addr_aht20" type="number"></label>
    <label>ADS1115<input name="i2c_addr_ads1115" type="number"></label>
    <label>BH1750<input name="i2c_addr_bh1750" type="number"></label>
    <label>SCD41<input name="i2c_addr_scd41" type="number"></label>
    <label>MCP23017<input name="i2c_addr_mcp23017" type="number"></label>
  </div>
  <button type="submit">Guardar y reiniciar</button>
</form>
<div id="st"></div>
<p class="note">Evitar GPIO de strapping (0, 2, 12, 15 en ESP32 clásico).</p>
<script>
var f=document.getElementById('f');
fetch('/api/v1/pins').then(function(r){return r.json()}).then(function(p){
  var pins=(p.pins||p);
  f.querySelectorAll('input[name]').forEach(function(i){ if(pins[i.name]!==undefined) i.value=pins[i.name]; });
  if(p.locked){
    f.querySelectorAll('input').forEach(function(i){i.disabled=true;});
    f.querySelector('button').disabled=true;
    var st=document.getElementById('st');st.className='err';st.textContent='Pines bloqueados (PCB fija).';
  }
}).catch(function(){});
f.addEventListener('submit',function(e){
  e.preventDefault();
  var o={};
  f.querySelectorAll('input[name]').forEach(function(i){ o[i.name]=parseInt(i.value,10)||0; });
  var st=document.getElementById('st');
  fetch('/api/v1/pins',{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify(o)})
    .then(function(r){ return r.json().then(function(j){ return {ok:r.ok,j:j}; }); })
    .then(function(x){ if(x.ok){ st.className='ok'; st.textContent='Guardado. Reiniciando...'; setTimeout(function(){location.reload();},1500);} else { st.className='err'; st.textContent='Error: '+(x.j.error||'no autorizado'); } })
    .catch(function(){ st.className='err'; st.textContent='Error de red'; });
});
</script>
</body></html>)rawliteral";

} // namespace WebAssets
} // namespace gh
