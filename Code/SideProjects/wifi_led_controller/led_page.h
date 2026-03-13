#pragma once
// RGB LED controller page — pure neobrutalist dark theme
const char LED_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>RGB LED Controller</title>
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
      text-align: center;
    }
    .container { max-width: 420px; margin: 0 auto; }

    h1 {
      font-size: 22px;
      font-weight: 900;
      letter-spacing: 3px;
      text-transform: uppercase;
      color: var(--accent);
      margin-bottom: 16px;
    }

    /* ============ BUTTONS ============ */
    .btn-row { display: flex; gap: 10px; justify-content: center; margin-bottom: 16px; }
    .btn {
      flex: 1;
      font-family: var(--font);
      font-size: 14px;
      font-weight: 900;
      padding: 12px 8px;
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
    .btn-on {
      background: var(--accent);
      color: var(--bg);
      box-shadow: 5px 5px 0 var(--accent-muted);
    }
    .btn-on:hover { box-shadow: 3px 3px 0 var(--accent-muted); }
    .btn-off {
      background: var(--danger);
      color: var(--bg);
      box-shadow: 5px 5px 0 #8a1a1a;
    }
    .btn-off:hover { box-shadow: 3px 3px 0 #8a1a1a; }
    .btn-blink {
      background: var(--purple);
      color: var(--bg);
      box-shadow: 5px 5px 0 #6a4a9e;
    }
    .btn-blink:hover { box-shadow: 3px 3px 0 #6a4a9e; }
    .btn.active {
      outline: 3px solid #fff;
      outline-offset: 2px;
    }

    /* ============ STATUS ============ */
    .status {
      font-size: 14px;
      font-weight: 900;
      letter-spacing: 1px;
      text-transform: uppercase;
      padding: 14px;
      border: 3px solid var(--accent);
      background: var(--surface);
      margin-bottom: 16px;
      box-shadow: 4px 4px 0 var(--accent-muted);
      color: var(--text);
    }

    /* ============ COLOR PREVIEW ============ */
    .preview {
      width: 90px;
      height: 90px;
      border-radius: 50%;
      margin: 16px auto;
      border: 3px solid var(--text);
      box-shadow: 4px 4px 0 #333;
      transition: background 0.15s;
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

    /* ============ PRESETS ============ */
    .presets { display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; margin: 12px 0; }
    .preset {
      width: 48px;
      height: 48px;
      border: 3px solid #333;
      cursor: pointer;
      box-shadow: 3px 3px 0 #111;
      transition: transform 0.1s, box-shadow 0.1s;
    }
    .preset:hover {
      transform: translate(1px, 1px);
      box-shadow: 2px 2px 0 #111;
    }
    .preset:active {
      transform: translate(3px, 3px);
      box-shadow: none;
    }
    .preset.sel {
      border-color: var(--accent);
      outline: 3px solid var(--accent);
      outline-offset: 2px;
    }

    /* ============ SLIDERS ============ */
    .slider-group { margin: 10px auto; max-width: 300px; text-align: left; }
    .slider-label {
      display: flex;
      justify-content: space-between;
      font-size: 11px;
      font-weight: 900;
      color: var(--text-muted);
      text-transform: uppercase;
      letter-spacing: 1px;
      margin-bottom: 4px;
    }
    .slider-label span {
      color: var(--accent);
      font-size: 14px;
    }
    input[type=range] {
      width: 100%;
      height: 6px;
      -webkit-appearance: none;
      background: #222;
      border: 2px solid #333;
      outline: none;
      margin-bottom: 8px;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 22px;
      height: 22px;
      border: 3px solid #111;
      cursor: pointer;
    }
    #sliderR::-webkit-slider-thumb { background: #ff4444; }
    #sliderG::-webkit-slider-thumb { background: #44ff44; }
    #sliderB::-webkit-slider-thumb { background: #4444ff; }
    #sliderBr::-webkit-slider-thumb { background: var(--accent); }

    /* ============ RESET LINK ============ */
    .reset-link {
      display: block;
      margin-top: 20px;
      color: var(--danger);
      font-size: 11px;
      font-weight: 900;
      text-transform: uppercase;
      letter-spacing: 1px;
      text-decoration: none;
    }
    .reset-link:hover { text-decoration: underline; }

    /* ============ RESPONSIVE ============ */
    @media (max-width: 480px) {
      body { padding: 10px; }
      h1 { font-size: 18px; }
      .btn { padding: 12px 4px; font-size: 13px; }
      .preset { width: 40px; height: 40px; }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>RGB LED Controller</h1>

    <!-- Status -->
    <div id="status" class="status">LED: OFF</div>

    <!-- Control buttons -->
    <div class="btn-row">
      <button id="btnOn" class="btn btn-on" onclick="sendCmd('/on')">ON</button>
      <button id="btnOff" class="btn btn-off" onclick="sendCmd('/off')">OFF</button>
      <button id="btnBlink" class="btn btn-blink" onclick="sendCmd('/blink')">BLINK</button>
    </div>

    <!-- Color preview -->
    <div id="preview" class="preview"></div>

    <!-- Color presets -->
    <div class="card">
      <div class="card-title">Presets</div>
      <div class="presets" id="presets">
        <div class="preset" style="background:#FF0000" onclick="setPreset(255,0,0)"></div>
        <div class="preset" style="background:#00FF00" onclick="setPreset(0,255,0)"></div>
        <div class="preset" style="background:#0000FF" onclick="setPreset(0,0,255)"></div>
        <div class="preset" style="background:#FFFF00" onclick="setPreset(255,255,0)"></div>
        <div class="preset" style="background:#FF00FF" onclick="setPreset(255,0,255)"></div>
        <div class="preset" style="background:#00FFFF" onclick="setPreset(0,255,255)"></div>
        <div class="preset" style="background:#FF6600" onclick="setPreset(255,102,0)"></div>
        <div class="preset" style="background:#FF1493" onclick="setPreset(255,20,147)"></div>
        <div class="preset" style="background:#FFFFFF" onclick="setPreset(255,255,255)"></div>
      </div>
    </div>

    <!-- RGB sliders -->
    <div class="card">
      <div class="card-title">RGB Channels</div>
      <div class="slider-group">
        <div class="slider-label">Red <span id="valR">255</span></div>
        <input type="range" id="sliderR" min="0" max="255" value="255"
          oninput="sliderMoved()" onchange="sliderDone()">
      </div>
      <div class="slider-group">
        <div class="slider-label">Green <span id="valG">255</span></div>
        <input type="range" id="sliderG" min="0" max="255" value="255"
          oninput="sliderMoved()" onchange="sliderDone()">
      </div>
      <div class="slider-group">
        <div class="slider-label">Blue <span id="valB">255</span></div>
        <input type="range" id="sliderB" min="0" max="255" value="255"
          oninput="sliderMoved()" onchange="sliderDone()">
      </div>
    </div>

    <!-- Brightness slider -->
    <div class="card">
      <div class="card-title">Brightness</div>
      <div class="slider-group">
        <div class="slider-label">Level <span id="valBr">255</span></div>
        <input type="range" id="sliderBr" min="0" max="255" value="255"
          oninput="document.getElementById('valBr').textContent=this.value"
          onchange="sendCmd('/brightness?value='+this.value)">
      </div>
    </div>

    <a href="/reset-wifi" class="reset-link"
       onclick="return confirm('Clear WiFi credentials and reboot?')">
      Reset WiFi Settings
    </a>
  </div>

  <script>
    /* ============ STATE ============ */
    var curR = 255, curG = 255, curB = 255;
    var dragging = false;

    /* ============ SEND COMMAND ============ */
    function sendCmd(path) {
      var xhr = new XMLHttpRequest();
      xhr.open('GET', path, true);
      xhr.send();
    }

    /* ============ PRESETS ============ */
    function setPreset(r, g, b) {
      sendCmd('/color?r=' + r + '&g=' + g + '&b=' + b);
    }

    /* ============ RGB SLIDERS ============ */
    function sliderMoved() {
      dragging = true;
      var r = document.getElementById('sliderR').value;
      var g = document.getElementById('sliderG').value;
      var b = document.getElementById('sliderB').value;
      document.getElementById('valR').textContent = r;
      document.getElementById('valG').textContent = g;
      document.getElementById('valB').textContent = b;
      updatePreview(r, g, b);
    }

    function sliderDone() {
      var r = document.getElementById('sliderR').value;
      var g = document.getElementById('sliderG').value;
      var b = document.getElementById('sliderB').value;
      sendCmd('/color?r=' + r + '&g=' + g + '&b=' + b);
      dragging = false;
    }

    /* ============ UPDATE PREVIEW ============ */
    function updatePreview(r, g, b) {
      var hex = '#' + toHex(r) + toHex(g) + toHex(b);
      var el = document.getElementById('preview');
      el.style.background = hex;
      el.style.boxShadow = '4px 4px 0 ' + hex;
    }

    function toHex(n) {
      var h = parseInt(n).toString(16);
      return h.length < 2 ? '0' + h : h;
    }

    /* ============ UPDATE UI FROM STATUS ============ */
    function updateUI(d) {
      /* Status text */
      var statusEl = document.getElementById('status');
      if (d.blink) statusEl.textContent = 'Mode: BLINKING';
      else if (d.on) statusEl.textContent = 'LED: ON';
      else statusEl.textContent = 'LED: OFF';

      /* Button highlights */
      document.getElementById('btnOn').className = 'btn btn-on' + (d.on && !d.blink ? ' active' : '');
      document.getElementById('btnOff').className = 'btn btn-off' + (!d.on && !d.blink ? ' active' : '');
      document.getElementById('btnBlink').className = 'btn btn-blink' + (d.blink ? ' active' : '');

      /* Only update sliders and picker if user is not dragging */
      if (!dragging) {
        curR = d.r; curG = d.g; curB = d.b;

        document.getElementById('sliderR').value = d.r;
        document.getElementById('sliderG').value = d.g;
        document.getElementById('sliderB').value = d.b;
        document.getElementById('valR').textContent = d.r;
        document.getElementById('valG').textContent = d.g;
        document.getElementById('valB').textContent = d.b;

        document.getElementById('sliderBr').value = d.br;
        document.getElementById('valBr').textContent = d.br;

        updatePreview(d.r, d.g, d.b);
      }

      /* Highlight matching preset */
      var presets = document.getElementById('presets').children;
      for (var i = 0; i < presets.length; i++) {
        presets[i].classList.remove('sel');
      }
      var presetData = [
        [255,0,0],[0,255,0],[0,0,255],[255,255,0],
        [255,0,255],[0,255,255],[255,102,0],[255,20,147],[255,255,255]
      ];
      for (var i = 0; i < presetData.length; i++) {
        if (d.r == presetData[i][0] && d.g == presetData[i][1] && d.b == presetData[i][2]) {
          presets[i].classList.add('sel');
        }
      }
    }

    /* ============ POLL STATUS ============ */
    function fetchStatus() {
      var xhr = new XMLHttpRequest();
      xhr.open('GET', '/status', true);
      xhr.timeout = 3000;
      xhr.onload = function() {
        if (xhr.status === 200) {
          try {
            var d = JSON.parse(xhr.responseText);
            updateUI(d);
          } catch(e) {}
        }
      };
      xhr.onloadend = function() {
        setTimeout(fetchStatus, 500);
      };
      xhr.send();
    }

    /* Start polling */
    fetchStatus();
  </script>
</body>
</html>
)rawliteral";
