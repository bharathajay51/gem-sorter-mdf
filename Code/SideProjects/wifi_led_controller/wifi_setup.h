#pragma once
// WiFi setup captive portal page — pure neobrutalist dark theme
const char SETUP_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>WiFi Setup</title>
  <style>
    /* ============ RESET ============ */
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

    /* ============ TOKENS ============ */
    :root {
      --bg: #0d0d0d;
      --surface: #1a1a1a;
      --accent: #39FF14;
      --accent-muted: #2a8a14;
      --text: #E0E0E0;
      --text-muted: #888888;
      --danger: #FF3B3B;
      --purple: #B388FF;
      --font: ui-monospace, 'SFMono-Regular', 'Menlo', 'Monaco', 'Consolas', 'Liberation Mono', 'Courier New', monospace;
    }

    body {
      font-family: var(--font);
      background: var(--bg);
      color: var(--text);
      min-height: 100vh;
      padding: 16px;
    }
    .container { max-width: 420px; margin: 0 auto; }

    h1 {
      font-size: 22px;
      font-weight: 900;
      letter-spacing: 3px;
      text-transform: uppercase;
      color: var(--accent);
      text-align: center;
      margin-bottom: 8px;
    }
    .subtitle {
      text-align: center;
      color: var(--text-muted);
      font-size: 12px;
      letter-spacing: 1px;
      margin-bottom: 20px;
    }

    /* ============ CARD ============ */
    .card {
      border: 3px solid var(--accent-muted);
      background: var(--surface);
      padding: 16px;
      margin-bottom: 14px;
      box-shadow: 5px 5px 0 #111;
    }
    .card-title {
      font-size: 12px;
      font-weight: 900;
      text-transform: uppercase;
      letter-spacing: 2px;
      margin-bottom: 12px;
      padding-bottom: 8px;
      border-bottom: 2px solid #333;
      color: var(--accent);
    }

    /* ============ NETWORK LIST ============ */
    .network-list {
      max-height: 240px;
      overflow-y: auto;
    }
    .network-item {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 10px 12px;
      margin-bottom: 4px;
      background: #111;
      border: 2px solid #222;
      cursor: pointer;
      transition: all 0.1s;
    }
    .network-item:hover {
      border-color: var(--accent-muted);
      box-shadow: 3px 3px 0 #111;
      transform: translate(-1px, -1px);
    }
    .network-item.selected {
      background: var(--surface);
      border-color: var(--accent);
      box-shadow: 3px 3px 0 var(--accent-muted);
    }
    .network-name {
      font-size: 13px;
      font-weight: 900;
      flex: 1;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      color: var(--text);
    }
    .network-item.selected .network-name {
      color: var(--accent);
    }
    .network-info {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 11px;
      color: var(--text-muted);
      flex-shrink: 0;
      margin-left: 10px;
    }
    .lock { color: var(--danger); }

    /* ============ SIGNAL BARS ============ */
    .signal-bars {
      display: inline-flex;
      align-items: flex-end;
      gap: 1px;
      height: 14px;
    }
    .signal-bars .bar {
      width: 3px;
      background: #333;
      border-radius: 1px;
    }
    .signal-bars .bar.active { background: var(--accent); }
    .signal-bars .bar:nth-child(1) { height: 4px; }
    .signal-bars .bar:nth-child(2) { height: 7px; }
    .signal-bars .bar:nth-child(3) { height: 10px; }
    .signal-bars .bar:nth-child(4) { height: 14px; }

    /* ============ SCANNING ============ */
    .scanning {
      text-align: center;
      padding: 30px;
      color: var(--text-muted);
      font-size: 13px;
    }
    .spinner {
      display: inline-block;
      width: 18px;
      height: 18px;
      border: 3px solid #333;
      border-top-color: var(--accent);
      border-radius: 50%;
      animation: spin 0.8s linear infinite;
      margin-right: 8px;
      vertical-align: middle;
    }
    @keyframes spin { to { transform: rotate(360deg); } }
    .no-networks {
      text-align: center;
      padding: 20px;
      color: var(--text-muted);
      font-size: 12px;
    }

    /* ============ FORM INPUTS ============ */
    label {
      display: block;
      font-size: 11px;
      font-weight: 900;
      color: var(--text-muted);
      margin-bottom: 6px;
      text-transform: uppercase;
      letter-spacing: 1px;
    }
    input[type="text"],
    input[type="password"] {
      width: 100%;
      padding: 10px 12px;
      font-size: 14px;
      font-family: var(--font);
      background: #111;
      border: 3px solid #333;
      color: var(--text);
      outline: none;
      margin-bottom: 12px;
    }
    input[type="text"]:focus,
    input[type="password"]:focus {
      border-color: var(--accent);
      box-shadow: 3px 3px 0 var(--accent-muted);
    }
    input::placeholder { color: #444; }

    /* ============ BUTTONS ============ */
    .btn {
      display: block;
      width: 100%;
      padding: 12px;
      font-family: var(--font);
      font-size: 14px;
      font-weight: 900;
      border: 3px solid var(--bg);
      cursor: pointer;
      text-transform: uppercase;
      letter-spacing: 1px;
      transition: transform 0.1s, box-shadow 0.1s;
    }
    .btn:hover {
      transform: translate(2px, 2px);
    }
    .btn:active {
      transform: translate(5px, 5px);
      box-shadow: none !important;
    }
    .btn:disabled { opacity: 0.3; cursor: not-allowed; transform: none !important; }
    .btn-connect {
      background: var(--accent);
      color: var(--bg);
      box-shadow: 5px 5px 0 var(--accent-muted);
      margin-bottom: 10px;
    }
    .btn-connect:hover:not(:disabled) { box-shadow: 3px 3px 0 var(--accent-muted); }
    .btn-scan {
      background: var(--purple);
      color: var(--bg);
      box-shadow: 5px 5px 0 #6a4a9e;
    }
    .btn-scan:hover:not(:disabled) { box-shadow: 3px 3px 0 #6a4a9e; }

    /* ============ STATUS MESSAGE ============ */
    .status-msg {
      text-align: center;
      padding: 12px;
      font-size: 12px;
      font-weight: 900;
      letter-spacing: 1px;
      margin-bottom: 14px;
      display: none;
      border: 3px solid;
    }
    .status-msg.success {
      display: block;
      background: rgba(57,255,20,0.08);
      color: var(--accent);
      border-color: var(--accent);
      box-shadow: 4px 4px 0 var(--accent-muted);
    }
    .status-msg.error {
      display: block;
      background: rgba(255,59,59,0.08);
      color: var(--danger);
      border-color: var(--danger);
      box-shadow: 4px 4px 0 #8a1a1a;
    }

    /* ============ SHOW PASSWORD ============ */
    .show-pass {
      display: flex;
      align-items: center;
      gap: 6px;
      font-size: 11px;
      color: var(--text-muted);
      margin-bottom: 12px;
      cursor: pointer;
      user-select: none;
    }
    .show-pass input { width: auto; margin: 0; accent-color: var(--accent); }

    /* ============ RESPONSIVE ============ */
    @media (max-width: 480px) {
      body { padding: 10px; }
      h1 { font-size: 18px; }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>WiFi LED Controller</h1>
    <p class="subtitle">Connect your device to a WiFi network</p>

    <div id="statusMsg" class="status-msg"></div>

    <div class="card">
      <div class="card-title">Available Networks</div>
      <div id="networkList" class="network-list">
        <div class="scanning"><span class="spinner"></span>Scanning...</div>
      </div>
      <div style="margin-top:12px;">
        <button id="btnScan" class="btn btn-scan" onclick="doScan()">Scan Again</button>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Connect</div>
      <label for="ssid">Network Name (SSID)</label>
      <input type="text" id="ssid" maxlength="32" placeholder="Select above or type manually">
      <label for="pass">Password</label>
      <input type="password" id="pass" maxlength="64" placeholder="Leave empty for open networks">
      <label class="show-pass"><input type="checkbox" onchange="togglePass(this)"> Show password</label>
      <button id="btnConnect" class="btn btn-connect" onclick="doConnect()">Connect</button>
    </div>
  </div>

  <script>
    var selectedEl = null;

    function doScan() {
      var list = document.getElementById('networkList');
      list.innerHTML = '<div class="scanning"><span class="spinner"></span>Scanning...</div>';
      document.getElementById('btnScan').disabled = true;

      var xhr = new XMLHttpRequest();
      xhr.open('GET', '/scan', true);
      xhr.timeout = 15000;
      xhr.onload = function() {
        document.getElementById('btnScan').disabled = false;
        if (xhr.status === 200) {
          var networks = JSON.parse(xhr.responseText);
          renderNetworks(networks);
        } else {
          list.innerHTML = '<div class="no-networks">Scan failed. Try again.</div>';
        }
      };
      xhr.onerror = function() {
        document.getElementById('btnScan').disabled = false;
        list.innerHTML = '<div class="no-networks">Connection error. Try again.</div>';
      };
      xhr.ontimeout = function() {
        document.getElementById('btnScan').disabled = false;
        list.innerHTML = '<div class="no-networks">Scan timed out. Try again.</div>';
      };
      xhr.send();
    }

    function renderNetworks(networks) {
      var list = document.getElementById('networkList');
      if (networks.length === 0) {
        list.innerHTML = '<div class="no-networks">No networks found. Try scanning again.</div>';
        return;
      }
      var html = '';
      for (var i = 0; i < networks.length; i++) {
        var n = networks[i];
        var bars = signalBars(n.rssi);
        var lockIcon = n.secure ? '<span class="lock">&#128274;</span>' : '';
        html += '<div class="network-item" onclick="selectNetwork(this,\'' + escAttr(n.ssid) + '\',' + (n.secure ? 'true' : 'false') + ')">';
        html += '<span class="network-name">' + escHtml(n.ssid) + '</span>';
        html += '<span class="network-info">' + bars + lockIcon + '</span>';
        html += '</div>';
      }
      list.innerHTML = html;
    }

    function signalBars(rssi) {
      var strength = 0;
      if (rssi >= -50) strength = 4;
      else if (rssi >= -65) strength = 3;
      else if (rssi >= -75) strength = 2;
      else strength = 1;
      var html = '<span class="signal-bars">';
      for (var i = 1; i <= 4; i++) {
        html += '<span class="bar' + (i <= strength ? ' active' : '') + '"></span>';
      }
      html += '</span>';
      return html;
    }

    function selectNetwork(el, ssid, secure) {
      if (selectedEl) selectedEl.classList.remove('selected');
      el.classList.add('selected');
      selectedEl = el;
      document.getElementById('ssid').value = ssid;
      if (secure) {
        document.getElementById('pass').focus();
      }
    }

    function doConnect() {
      var ssid = document.getElementById('ssid').value.trim();
      var pass = document.getElementById('pass').value;
      if (!ssid) {
        showStatus('Please enter or select a network name.', 'error');
        return;
      }
      document.getElementById('btnConnect').disabled = true;
      showStatus('Saving credentials and rebooting...', 'success');

      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/connect', true);
      xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
      xhr.onload = function() {
        showStatus('Credentials saved! The device is rebooting. Connect to your WiFi network and check your router for the device IP, or open the serial monitor.', 'success');
      };
      xhr.onerror = function() {
        showStatus('Credentials saved! The device is rebooting.', 'success');
      };
      xhr.send('ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(pass));
    }

    function showStatus(msg, type) {
      var el = document.getElementById('statusMsg');
      el.textContent = msg;
      el.className = 'status-msg ' + type;
    }

    function togglePass(cb) {
      document.getElementById('pass').type = cb.checked ? 'text' : 'password';
    }

    function escHtml(s) {
      return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
    }

    function escAttr(s) {
      return s.replace(/\\/g,'\\\\').replace(/'/g,"\\'").replace(/"/g,'\\"');
    }

    // Auto-scan on page load
    doScan();
  </script>
</body>
</html>
)rawliteral";
