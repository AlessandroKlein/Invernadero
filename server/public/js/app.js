// Dashboard del servidor central.
const $ = (s) => document.querySelector(s);
let token = localStorage.getItem('gh_token') || '';
let chart = null;

function api(path, opts = {}) {
  const headers = { 'Content-Type': 'application/json' };
  if (token) headers['Authorization'] = 'Bearer ' + token;
  return fetch('/api/v1' + path, { headers, ...opts }).then(async (r) => {
    if (r.status === 401) { logout(); throw new Error('Sesión expirada'); }
    const data = await r.json().catch(() => ({}));
    if (!r.ok) throw new Error(data.error || r.statusText);
    return data;
  });
}

function showLogin() { $('#login').classList.remove('hidden'); $('#app').classList.add('hidden'); }
function showApp()   { $('#login').classList.add('hidden'); $('#app').classList.remove('hidden'); }
function logout()    { token = ''; localStorage.removeItem('gh_token'); showLogin(); }

async function login() {
  try {
    const r = await fetch('/api/v1/auth/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ username: $('#login-user').value, password: $('#login-pass').value }),
    });
    const data = await r.json();
    if (!r.ok) throw new Error(data.error || 'Error de login');
    token = data.token;
    localStorage.setItem('gh_token', token);
    showApp();
    loadDevices();
  } catch (e) {
    $('#login-err').textContent = e.message;
  }
}

async function loadDevices() {
  const devices = await api('/devices');
  const ul = $('#device-list');
  ul.innerHTML = '';
  devices.forEach((d) => {
    const li = document.createElement('li');
    const cls = d.status === 'ONLINE' ? 'online' : 'offline';
    li.innerHTML = `<span class="dot ${cls}"></span>${d.device_id} <span style="color:var(--muted)">· ${d.name || ''}</span>`;
    li.onclick = () => selectDevice(d);
    ul.appendChild(li);
  });
}

async function selectDevice(d) {
  $('#placeholder').classList.add('hidden');
  $('#detail').classList.remove('hidden');
  $('#detail-title').textContent = `${d.device_id} · ${d.name || 'Sin nombre'}`;
  $('#device-meta').innerHTML = `FW ${d.firmware_version} · HW ${d.hardware_profile} ${d.hardware_version} · ${d.status}`;

  const [sensors, actuators, alarms] = await Promise.all([
    api(`/devices/${d.device_id}/sensors`),
    api(`/devices/${d.device_id}/actuators`),
    api(`/devices/${d.device_id}/alarms`),
  ]);

  $('#sensors').innerHTML = sensors.map((s) =>
    `<div class="tile"><div class="label">${s.name || s.sensor_id}</div><div class="value">--</div><div class="unit">${s.unit || ''}</div></div>`
  ).join('') || '<p>Sin sensores registrados</p>';

  $('#actuators').innerHTML = actuators.map((a) =>
    `<div class="tile"><div class="label">${a.name || a.actuator_id}</div><div class="value">--</div><div class="unit">${a.role || ''}</div></div>`
  ).join('') || '<p>Sin actuadores registrados</p>';

  $('#alarms').innerHTML = alarms.map((a) =>
    `<li class="${a.severity === 'CRITICAL' ? 'critical' : ''}">${a.type}: ${a.message || ''} <small>(${a.severity})</small></li>`
  ).join('') || '<li>Sin alarmas</li>';

  loadReadings(d.device_id);
  connectMqtt(d.device_id);
}

async function loadReadings(deviceId) {
  const readings = await api(`/devices/${deviceId}/readings?limit=200`);
  const bySensor = {};
  readings.forEach((r) => { (bySensor[r.sensor_id] ||= []).push(r); });
  const datasets = Object.entries(bySensor).map(([sid, rows]) => ({
    label: sid,
    data: rows.map((r) => ({ x: r.ts, y: r.value })),
    borderWidth: 1.5,
    pointRadius: 0,
  }));
  if (chart) chart.destroy();
  chart = new Chart($('#chart'), {
    type: 'line',
    data: { datasets },
    options: { parsing: false, scales: { x: { type: 'time', display: true } } },
  });
}

function connectMqtt(deviceId) {
  if (!window.mqtt) return;
  try {
    const c = mqtt.connect(`ws://${location.hostname}:9001`);
    c.on('connect', () => c.subscribe(`greenhouse/${deviceId}/#`));
    c.on('message', (t, m) => console.log('[mqtt]', t, m.toString()));
  } catch (e) { /* tiempo real opcional */ }
}

$('#login-btn').onclick = login;
$('#logout').onclick = logout;

if (token) { showApp(); loadDevices(); }
else { showLogin(); }
