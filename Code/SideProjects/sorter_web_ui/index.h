#pragma once
// Full neobrutalist HTML/CSS/JS dashboard
const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Candy Sorter</title>
  <style>
    /* ============ RESET ============ */
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

    /* ============ TOKENS — DARK RETRO 80s ============ */
    :root {
      --bg: #0a0a0a;
      --card-bg: #0f0f0f;
      --glow: #39FF14;
      --glow-dim: rgba(57,255,20,0.15);
      --border-c: #39FF14;
      --border: 2px solid var(--border-c);
      --font: ui-monospace, 'SFMono-Regular', 'Menlo', 'Monaco', 'Consolas', 'Liberation Mono', 'Courier New', monospace;
      --red: #FF2D2D;
      --green: #39FF14;
      --blue: #00E5FF;
      --yellow: #FFD600;
      --pink: #FF4081;
      --orange: #FF6D00;
      --purple: #B388FF;
      --text: #39FF14;
      --text-dim: #1a8a0e;
      --tube-h: 200px;
    }

    /* ============ SCANLINE OVERLAY ============ */
    body::after {
      content: '';
      position: fixed;
      top: 0; left: 0; right: 0; bottom: 0;
      background: repeating-linear-gradient(
        0deg,
        transparent,
        transparent 2px,
        rgba(0,0,0,0.15) 2px,
        rgba(0,0,0,0.15) 4px
      );
      pointer-events: none;
      z-index: 9999;
    }

    body {
      font-family: var(--font);
      background: var(--bg);
      color: var(--text);
      min-height: 100vh;
      padding: 16px;
    }

    /* ============ HEADER ============ */
    .header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 14px 20px;
      border: var(--border);
      background: var(--card-bg);
      margin-bottom: 16px;
      box-shadow: 0 0 12px var(--glow-dim), inset 0 0 12px rgba(57,255,20,0.05);
      flex-wrap: wrap;
      gap: 10px;
    }
    .header h1 {
      font-size: 20px;
      font-weight: 700;
      letter-spacing: 2px;
      text-transform: uppercase;
      color: var(--glow);
      text-shadow: 0 0 8px var(--glow-dim);
    }
    .header-right {
      display: flex;
      gap: 12px;
      align-items: center;
    }
    .total-count {
      font-size: 13px;
      font-weight: 700;
      color: var(--text-dim);
      letter-spacing: 1px;
      text-transform: uppercase;
      margin-bottom: 12px;
    }
    .total-count span {
      color: var(--glow);
      font-size: 18px;
      text-shadow: 0 0 8px var(--glow-dim);
    }
    .btn-group { display: flex; gap: 10px; align-items: center; }

    /* ============ BUTTONS ============ */
    .btn {
      font-family: var(--font);
      font-size: 13px;
      font-weight: 700;
      padding: 10px 20px;
      border: 2px solid;
      cursor: pointer;
      text-transform: uppercase;
      letter-spacing: 1px;
      background: transparent;
      transition: all 0.1s;
    }
    .btn:active {
      transform: translate(1px, 1px);
    }
    .btn-start {
      color: var(--green);
      border-color: var(--green);
      text-shadow: 0 0 6px var(--glow-dim);
      box-shadow: 0 0 8px var(--glow-dim);
    }
    .btn-start:hover { background: rgba(57,255,20,0.1); }
    .btn-stop {
      color: var(--red);
      border-color: var(--red);
      text-shadow: 0 0 6px rgba(255,45,45,0.3);
      box-shadow: 0 0 8px rgba(255,45,45,0.15);
    }
    .btn-stop:hover { background: rgba(255,45,45,0.1); }
    .btn:disabled { opacity: 0.3; cursor: not-allowed; }

    /* ============ GRID ============ */
    .grid {
      display: grid;
      grid-template-columns: 1fr 2fr;
      gap: 16px;
      margin-bottom: 16px;
    }

    /* ============ CARD ============ */
    .card {
      border: var(--border);
      background: var(--card-bg);
      padding: 20px;
      box-shadow: 0 0 12px var(--glow-dim), inset 0 0 12px rgba(57,255,20,0.05);
    }
    .card-title {
      font-size: 14px;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 2px;
      margin-bottom: 16px;
      padding-bottom: 8px;
      border-bottom: 1px solid var(--text-dim);
      color: var(--glow);
      text-shadow: 0 0 6px var(--glow-dim);
    }

    /* ============ SERVO GRAPHIC ============ */
    .servo-wrap {
      display: flex;
      gap: 24px;
      justify-content: space-around;
      padding: 10px 0;
    }
    .servo-unit {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 8px;
    }
    .servo-visual {
      position: relative;
      width: 120px;
      height: 150px;
    }
    /* Servo body — rectangle */
    .servo-body {
      position: absolute;
      bottom: 0;
      left: 50%;
      transform: translateX(-50%);
      width: 56px;
      height: 72px;
      border: 2px solid var(--glow);
      background: rgba(57,255,20,0.05);
      border-radius: 3px;
    }
    /* Servo hub — circle on top of body */
    .servo-hub {
      position: absolute;
      bottom: 62px;
      left: 50%;
      transform: translateX(-50%);
      width: 30px;
      height: 30px;
      border-radius: 50%;
      border: 2px solid var(--glow);
      background: var(--card-bg);
      z-index: 2;
    }
    /* Servo arm — rotating line */
    .servo-arm {
      position: absolute;
      bottom: 76px;
      left: 50%;
      width: 3px;
      height: 48px;
      background: var(--glow);
      transform-origin: bottom center;
      transform: rotate(0deg);
      transition: transform 0.4s ease-out;
      box-shadow: 0 0 6px var(--glow);
      z-index: 1;
    }
    /* Arm tip dot */
    .servo-arm::after {
      content: '';
      position: absolute;
      top: -4px;
      left: -3px;
      width: 9px;
      height: 9px;
      border-radius: 50%;
      background: var(--glow);
      box-shadow: 0 0 8px var(--glow);
    }
    .servo-info {
      text-align: center;
    }
    .servo-label {
      font-weight: 700;
      font-size: 11px;
      color: var(--text-dim);
      text-transform: uppercase;
      letter-spacing: 1px;
      margin-bottom: 4px;
    }
    .servo-angle {
      font-size: 22px;
      font-weight: 700;
      color: var(--glow);
      text-shadow: 0 0 8px var(--glow-dim);
    }

    /* ============ TEST TUBES ============ */
    .tubes-wrap {
      display: flex;
      justify-content: space-around;
      gap: 8px;
      align-items: flex-end;
    }
    .tube-col {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 6px;
    }
    .tube {
      width: 44px;
      height: var(--tube-h);
      border: 2px solid;
      border-radius: 0 0 4px 4px;
      position: relative;
      display: flex;
      flex-direction: column;
      justify-content: flex-end;
      overflow: hidden;
      background: rgba(0,0,0,0.6);
    }
    .candy-block {
      width: 100%;
      height: calc(var(--tube-h) / 10 - 2px);
      margin-bottom: 3px;
      border-radius: 2px;
      opacity: 0.85;
      animation: dropIn 0.3s ease-out;
    }
    @keyframes dropIn {
      0% { transform: translateY(-10px); opacity: 0; }
      100% { transform: translateY(0); opacity: 0.85; }
    }
    .tube-count {
      font-size: 16px;
      font-weight: 700;
      color: var(--glow);
      text-shadow: 0 0 6px var(--glow-dim);
    }
    .tube-label {
      font-size: 10px;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.5px;
      color: var(--text-dim);
    }

    /* Tube border + glow + label colours */
    .tube-Red    { border-color: var(--red); box-shadow: 0 0 8px rgba(255,45,45,0.2); }
    .tube-Green  { border-color: var(--green); box-shadow: 0 0 8px var(--glow-dim); }
    .tube-Blue   { border-color: var(--blue); box-shadow: 0 0 8px rgba(0,229,255,0.2); }
    .tube-Yellow { border-color: var(--yellow); box-shadow: 0 0 8px rgba(255,214,0,0.2); }
    .tube-Pink   { border-color: var(--pink); box-shadow: 0 0 8px rgba(255,64,129,0.2); }
    .tube-Orange { border-color: var(--orange); box-shadow: 0 0 8px rgba(255,109,0,0.2); }
    .tube-Purple { border-color: var(--purple); box-shadow: 0 0 8px rgba(179,136,255,0.2); }

    /* ============ TERMINAL ============ */
    .terminal-wrap {
      border: var(--border);
      background: var(--card-bg);
      box-shadow: 0 0 12px var(--glow-dim), inset 0 0 12px rgba(57,255,20,0.05);
      padding: 14px 16px;
    }
    .terminal-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      cursor: pointer;
      margin-bottom: 10px;
      padding-bottom: 8px;
      border-bottom: 1px solid var(--text-dim);
    }
    .terminal-title {
      font-size: 14px;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 2px;
      color: var(--glow);
      text-shadow: 0 0 6px var(--glow-dim);
    }
    .terminal-toggle {
      font-size: 14px;
      color: var(--text-dim);
      transition: transform 0.2s;
    }
    .terminal-toggle.collapsed { transform: rotate(-90deg); }
    .terminal {
      background: #050505;
      border: 1px solid #1a3a0e;
      padding: 12px 14px;
      max-height: 180px;
      overflow-y: auto;
      font-size: 12px;
      line-height: 1.7;
      color: var(--glow);
      transition: max-height 0.3s ease, padding 0.3s ease;
    }
    .terminal.collapsed {
      max-height: 0;
      padding: 0 14px;
      overflow: hidden;
      border: none;
    }
    .terminal-line {
      white-space: pre-wrap;
      text-shadow: 0 0 4px var(--glow-dim);
    }
    .terminal-cursor {
      display: inline-block;
      width: 8px;
      height: 13px;
      background: var(--glow);
      box-shadow: 0 0 6px var(--glow);
      animation: blink 1s step-end infinite;
      vertical-align: text-bottom;
      margin-left: 2px;
    }
    @keyframes blink {
      50% { opacity: 0; }
    }

    /* ============ RESPONSIVE — TABLET ============ */
    @media (max-width: 900px) {
      .grid { grid-template-columns: 1fr 1fr; }
      :root { --tube-h: 170px; }
    }

    /* ============ RESPONSIVE — PHONE ============ */
    @media (max-width: 550px) {
      body { padding: 10px; }
      .header {
        flex-direction: column;
        align-items: stretch;
        text-align: center;
        padding: 12px 14px;
      }
      .header h1 { font-size: 16px; }
      .header-right {
        flex-direction: column;
        gap: 8px;
      }
      .btn-group { justify-content: center; }
      .btn { padding: 12px 20px; flex: 1; }
      .grid { grid-template-columns: 1fr; gap: 12px; }
      :root { --tube-h: 140px; }
      .tube { width: 32px; }
      .tube-label { font-size: 8px; }
      .tube-count { font-size: 13px; }
      .servo-wrap { gap: 16px; }
      .servo-visual { width: 80px; height: 100px; }
      .servo-body { width: 40px; height: 50px; }
      .servo-hub { width: 22px; height: 22px; bottom: 42px; }
      .servo-arm { height: 32px; bottom: 52px; width: 2px; }
      .servo-arm::after { width: 6px; height: 6px; top: -3px; left: -2px; }
      .card { padding: 14px; }
    }
  </style>
</head>
<body>

  <!-- HEADER -->
  <div class="header">
    <h1>&#9670; Kriya Candy Sorter</h1>
    <div class="btn-group">
      <button id="btnStart" class="btn btn-start" onclick="startSort()">&#9654; Start</button>
      <button id="btnStop" class="btn btn-stop" onclick="stopSort()" disabled>&#9632; Stop</button>
    </div>
  </div>

  <!-- MAIN GRID -->
  <div class="grid">

    <!-- SERVO STATUS CARD -->
    <div class="card">
      <div class="card-title">Servo Status</div>
      <div class="servo-wrap">
        <!-- Top Servo -->
        <div class="servo-unit">
          <div class="servo-visual">
            <div class="servo-body"></div>
            <div class="servo-hub"></div>
            <div id="armTop" class="servo-arm"></div>
          </div>
          <div class="servo-info">
            <div class="servo-label">Top</div>
            <div id="angleTop" class="servo-angle">0&deg;</div>
          </div>
        </div>
        <!-- Slide Servo -->
        <div class="servo-unit">
          <div class="servo-visual">
            <div class="servo-body"></div>
            <div class="servo-hub"></div>
            <div id="armSlide" class="servo-arm"></div>
          </div>
          <div class="servo-info">
            <div class="servo-label">Slide</div>
            <div id="angleSlide" class="servo-angle">0&deg;</div>
          </div>
        </div>
      </div>
    </div>

    <!-- SORT STATUS CARD -->
    <div class="card">
      <div class="card-title">Sort Status</div>
      <div class="total-count">Total Sorted: <span id="totalCount">0</span></div>
      <div id="tubesWrap" class="tubes-wrap">
        <!-- Rendered by JS -->
      </div>
    </div>

  </div>

  <!-- TERMINAL -->
  <div class="terminal-wrap">
    <div class="terminal-header" onclick="toggleTerminal()">
      <div class="terminal-title">Status Terminal</div>
      <div id="termToggle" class="terminal-toggle">&#9660;</div>
    </div>
    <div id="terminal" class="terminal">
      <span class="terminal-line">> Waiting for connection...</span>
      <span class="terminal-cursor"></span>
    </div>
  </div>

  <script>
    /* ============ COLOUR MAP ============ */
    var COLOR_MAP = {
      Orange: '#FF6D00',
      Purple: '#B388FF',
      Red:    '#FF2D2D',
      Yellow: '#FFD600',
      Green:  '#39FF14',
      Blue:   '#00E5FF',
      Pink:   '#FF4081'
    };
    var MAX_CANDY = 8;

    /* ============ TUBE ORDER (by slide angle) ============ */
    var TUBE_ORDER = ['Orange','Purple','Red','Yellow','Green','Blue','Pink'];

    /* ============ INIT TUBES ============ */
    function initTubes() {
      var wrap = document.getElementById('tubesWrap');
      wrap.innerHTML = '';
      for (var i = 0; i < TUBE_ORDER.length; i++) {
        var name = TUBE_ORDER[i];
        var col = document.createElement('div');
        col.className = 'tube-col';

        var label = document.createElement('div');
        label.className = 'tube-label';
        label.textContent = name;
        label.style.color = COLOR_MAP[name];

        var tube = document.createElement('div');
        tube.className = 'tube tube-' + name;
        tube.id = 'tube-' + name;

        var count = document.createElement('div');
        count.className = 'tube-count';
        count.id = 'count-' + name;
        count.textContent = '0';
        count.style.color = COLOR_MAP[name];
        count.style.textShadow = '0 0 6px ' + COLOR_MAP[name] + '40';

        col.appendChild(label);
        col.appendChild(tube);
        col.appendChild(count);
        wrap.appendChild(col);
      }
    }
    initTubes();

    /* ============ UPDATE TUBES ============ */
    function updateTubes(colors) {
      for (var i = 0; i < colors.length; i++) {
        var tube = document.getElementById('tube-' + colors[i].name);
        var countEl = document.getElementById('count-' + colors[i].name);
        if (!tube || !countEl) continue;

        countEl.textContent = colors[i].count;

        /* Clear existing candy blocks */
        tube.innerHTML = '';

        /* Fill from bottom */
        var n = Math.min(colors[i].count, MAX_CANDY);
        for (var j = 0; j < n; j++) {
          var block = document.createElement('div');
          block.className = 'candy-block';
          block.style.background = COLOR_MAP[colors[i].name] || '#ccc';
          tube.appendChild(block);
        }
      }
    }

    /* ============ UPDATE SERVOS ============ */
    function updateServos(topAngle, slideAngle) {
      document.getElementById('angleTop').innerHTML = topAngle + '&deg;';
      document.getElementById('angleSlide').innerHTML = slideAngle + '&deg;';
      /* Rotate arms: map 0-180 to -90 to +90 deg */
      var topRot = (topAngle - 90);
      var slideRot = (slideAngle - 90);
      document.getElementById('armTop').style.transform = 'rotate(' + topRot + 'deg)';
      document.getElementById('armSlide').style.transform = 'rotate(' + slideRot + 'deg)';
    }

    /* ============ TERMINAL TOGGLE ============ */
    function toggleTerminal() {
      var el = document.getElementById('terminal');
      var chevron = document.getElementById('termToggle');
      el.classList.toggle('collapsed');
      chevron.classList.toggle('collapsed');
    }

    /* ============ UPDATE TERMINAL ============ */
    function updateTerminal(log) {
      var el = document.getElementById('terminal');
      if (el.classList.contains('collapsed')) return;
      if (!log || log.length === 0) return;
      var lines = log.split('\\n');
      var html = '';
      for (var i = 0; i < lines.length; i++) {
        html += '<div class="terminal-line">' + escapeHtml(lines[i]) + '</div>';
      }
      html += '<span class="terminal-cursor"></span>';
      el.innerHTML = html;
      el.scrollTop = el.scrollHeight;
    }

    function escapeHtml(str) {
      return str.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
    }

    /* ============ FETCH STATUS ============ */
    function fetchStatus() {
      return fetch('/status')
        .then(function(r) { return r.json(); })
        .then(function(d) {
          /* Servos */
          updateServos(d.topAngle, d.slideAngle);

          /* Buttons */
          document.getElementById('btnStart').disabled = d.sorting;
          document.getElementById('btnStop').disabled = !d.sorting;

          /* Total count */
          document.getElementById('totalCount').textContent = d.count;

          /* Tubes */
          updateTubes(d.colors);

          /* Terminal */
          updateTerminal(d.log);
        })
        .catch(function(e) {
          console.log('fetch error', e);
        });
    }

    /* ============ START / STOP ============ */
    function startSort() {
      fetch('/start', { method: 'POST' });
    }
    function stopSort() {
      fetch('/stop', { method: 'POST' });
    }

    /* Use sequential setTimeout instead of setInterval */
    function pollLoop() {
      fetchStatus().finally(function() {
        setTimeout(pollLoop, 500);
      });
    }

    /* Start the loop */
    pollLoop();
  </script>

</body>
</html>
)rawliteral";
