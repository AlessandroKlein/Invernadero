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
</head>
<body>
<h1 id="title">Invernadero</h1>
<div class="sub" id="subtitle">—</div>
<button id="loginBtn" style="margin-bottom:12px" onclick="login()">Iniciar sesión</button>

<section><h2>Sensores</h2><div class="grid" id="sensors"></div></section>
<section><h2>Actuadores</h2><div id="actuators"></div></section>
<section><h2>Alarmas</h2><div id="alarms">Sin alarmas</div></section>

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

loadStatus(); loadSensors(); loadActuators(); loadAlarms();
setInterval(()=>{loadSensors();loadActuators();loadAlarms();}, 3000);
</script>
</body></html>)rawliteral";

} // namespace WebAssets
} // namespace gh
