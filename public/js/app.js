// Dashboard del servidor central con RBAC (roles + permisos + scope).
const $ = (s) => document.querySelector(s);
let token = localStorage.getItem('gh_token') || '';
let user = JSON.parse(localStorage.getItem('gh_user') || 'null');
let allRoles = [];
let editingUserId = null;

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

function can(perm) { return user && (user.perms || []).includes(perm); }
function show(el) { $(el).classList.remove('hidden'); }
function hide(el) { $(el).classList.add('hidden'); }

function showLogin() { show('#login'); hide('#app'); }
function logout() {
  token = ''; user = null;
  localStorage.removeItem('gh_token'); localStorage.removeItem('gh_user');
  showLogin();
}

async function login() {
  try {
    const r = await fetch('/api/v1/auth/login', {
      method: 'POST', headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ username: $('#login-user').value, password: $('#login-pass').value }),
    });
    const data = await r.json();
    if (!r.ok) throw new Error(data.error || 'Error de login');
    token = data.token; user = data.user;
    localStorage.setItem('gh_token', token); localStorage.setItem('gh_user', JSON.stringify(user));
    $('#whoami').textContent = `${user.full_name || user.username} · ${(user.roles || []).join(', ')}`;
    hide('#login'); show('#app');
    buildNav(); openView('devices');
  } catch (e) { $('#login-err').textContent = e.message; }
}

function buildNav() {
  const items = [];
  if (can('device.read')) items.push(['devices', 'Dispositivos']);
  if (can('user.read')) items.push(['users', 'Usuarios']);
  if (can('user.read')) items.push(['roles', 'Roles y permisos']);
  $('#nav').innerHTML = items.map(([k, label]) => `<a href="#" data-view="${k}">${label}</a>`).join('');
  $('#nav').querySelectorAll('a').forEach((a) => (a.onclick = (e) => { e.preventDefault(); openView(a.dataset.view); }));
}

function openView(name) {
  document.querySelectorAll('.view').forEach((v) => hide('#' + v.id));
  const m = { devices: '#view-devices', users: '#view-users', roles: '#view-roles' };
  show(m[name] || '#view-devices');
  if (name === 'devices') loadDevices();
  if (name === 'users') loadUsers();
  if (name === 'roles') loadRoles();
}

// ---------- Dispositivos ----------
async function loadDevices() {
  try {
    const devices = await api('/devices');
    const box = $('#device-list');
    box.innerHTML = '';
    if (!devices.length) { $('#device-meta').textContent = 'Sin dispositivos registrados.'; return; }
    devices.forEach((d) => {
      const b = document.createElement('button');
      b.className = 'dev';
      b.textContent = `${d.device_id} · ${d.name || ''}`;
      b.onclick = () => selectDevice(d);
      box.appendChild(b);
    });
  } catch (e) { $('#device-meta').textContent = e.message; }
}

async function selectDevice(d) {
  $('#device-meta').textContent = `FW ${d.firmware_version} · HW ${d.hardware_profile} ${d.hardware_version} · ${d.status}`;
  const [sensors, actuators, alarms] = await Promise.all([
    api(`/devices/${d.device_id}/sensors`),
    api(`/devices/${d.device_id}/actuators`),
    api(`/devices/${d.device_id}/alarms`),
  ]);
  $('#sensors').innerHTML = sensors.map((s) =>
    `<div class="tile"><div class="label">${s.name || s.sensor_id}</div><div class="value">--</div><div class="unit">${s.unit || ''}</div></div>`
  ).join('') || '<p>Sin sensores</p>';
  $('#actuators').innerHTML = actuators.map((a) =>
    `<div class="tile"><div class="label">${a.name || a.actuator_id}</div><div class="value">--</div><div class="unit">${a.role || ''}</div></div>`
  ).join('') || '<p>Sin actuadores</p>';
  $('#alarms').innerHTML = alarms.map((a) =>
    `<li class="${a.severity === 'CRITICAL' ? 'critical' : ''}">${a.type}: ${a.message || ''} <small>(${a.severity})</small></li>`
  ).join('') || '<li>Sin alarmas</li>';
}

// ---------- Usuarios ----------
async function loadUsers() {
  const users = await api('/users');
  $('#users-table tbody').innerHTML = users.map((u) => `
    <tr>
      <td>${u.username}</td>
      <td>${u.full_name || ''}</td>
      <td>${(u.roles || []).join(', ')}</td>
      <td>${u.active ? '✅' : '⛔'}</td>
      <td>${u.last_login ? new Date(u.last_login).toLocaleString() : '—'}</td>
      <td class="row-actions">
        <button class="secondary" data-edit="${u.id}">Editar</button>
        <button class="danger" data-del="${u.id}">Eliminar</button>
      </td>
    </tr>`).join('');
  $('#users-table').querySelectorAll('[data-edit]').forEach((b) => (b.onclick = () => editUser(b.dataset.edit)));
  $('#users-table').querySelectorAll('[data-del]').forEach((b) => (b.onclick = () => delUser(b.dataset.del)));
}

async function loadRoles() {
  const [roles, perms] = await Promise.all([api('/roles'), api('/permissions')]);
  const byRole = perms.role_permissions || {};
  $('#roles-list').innerHTML = roles.map((r) => `
    <div class="role-card">
      <h3>${r.name}</h3>
      <p class="meta">${r.description || ''}</p>
      <p>${(byRole[r.id] || []).join(', ')}</p>
    </div>`).join('');
}

async function editUser(id) {
  const users = await api('/users');
  const u = users.find((x) => x.id === id);
  if (u) openUserModal(u);
}

async function openUserModal(u = null) {
  editingUserId = u ? u.id : null;
  $('#user-modal-title').textContent = u ? 'Editar usuario' : 'Nuevo usuario';
  $('#u-username').value = u ? (u.username || '') : '';
  $('#u-email').value = u ? (u.email || '') : '';
  $('#u-fullname').value = u ? (u.full_name || '') : '';
  $('#u-password').value = '';
  $('#u-active').checked = u ? !!u.active : true;
  $('#u-scope').value = (u && u.scope && u.scope.greenhouses ? u.scope.greenhouses : ['*']).join(', ');
  $('#u-err').textContent = '';

  if (!allRoles.length) allRoles = await api('/roles');
  const selected = new Set(u ? (u.roles || []) : []);
  $('#u-roles').innerHTML = allRoles.map((r) =>
    `<label class="chip"><input type="checkbox" value="${r.name}" ${selected.has(r.name) ? 'checked' : ''}> ${r.name}</label>`
  ).join('');
  show('#user-modal');
}

function closeUserModal() { hide('#user-modal'); }

function collectUser() {
  const roles = [...document.querySelectorAll('#u-roles input:checked')].map((i) => i.value);
  const scopeRaw = $('#u-scope').value.split(',').map((s) => s.trim()).filter(Boolean);
  return {
    username: $('#u-username').value,
    email: $('#u-email').value || null,
    full_name: $('#u-fullname').value || null,
    active: $('#u-active').checked,
    roles,
    scope: { greenhouses: scopeRaw.length ? scopeRaw : ['*'] },
    password: $('#u-password').value || undefined,
  };
}

async function saveUser() {
  try {
    const payload = collectUser();
    if (editingUserId) {
      if (!payload.password) delete payload.password;
      await api('/users/' + editingUserId, { method: 'PUT', body: JSON.stringify(payload) });
    } else {
      if (!payload.password) throw new Error('La contraseña es requerida');
      await api('/users', { method: 'POST', body: JSON.stringify(payload) });
    }
    closeUserModal(); loadUsers();
  } catch (e) { $('#u-err').textContent = e.message; }
}

async function delUser(id) {
  if (!confirm('¿Eliminar este usuario?')) return;
  try { await api('/users/' + id, { method: 'DELETE' }); loadUsers(); }
  catch (e) { alert(e.message); }
}

$('#login-btn').onclick = login;
$('#logout').onclick = logout;
$('#new-user').onclick = () => openUserModal(null);
$('#u-cancel').onclick = closeUserModal;
$('#u-save').onclick = saveUser;

if (token && user) {
  $('#whoami').textContent = `${user.full_name || user.username} · ${(user.roles || []).join(', ')}`;
  hide('#login'); show('#app');
  buildNav(); openView('devices');
} else { showLogin(); }
