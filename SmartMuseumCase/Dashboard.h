#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <Arduino.h>

// Utilizziamo un raw literal C++ con direttiva PROGMEM per salvare il malloppo
// stringa nella Flash e non nella RAM (preziosissima)
const char DASHBOARD_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="it">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Museum Dashboard</title>
    <style>
        :root {
            --bg: #09090b;
            --panel: rgba(24, 24, 27, 0.8);
            --border: rgba(255, 255, 255, 0.08);
            --accent: #6366f1;
            --accent-glow: rgba(99, 102, 241, 0.4);
            --text: #f8fafc;
            --text-muted: #94a3b8;
            --red: #ef4444;
            --red-glow: rgba(239, 68, 68, 0.4);
            --green: #22c55e;
            --green-glow: rgba(34, 197, 94, 0.4);
        }

        body {
            background-color: var(--bg);
            background-image: radial-gradient(circle at 50% 0%, rgba(99,102,241,0.15) 0%, rgba(9,9,11,1) 50%);
            color: var(--text);
            font-family: 'Outfit', sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            transition: all 0.3s ease;
        }

        .container {
            width: 95%;
            max-width: 1200px;
            padding: 2rem 0;
            z-index: 10;
        }

        .glass-panel {
            background: var(--panel);
            backdrop-filter: blur(16px);
            -webkit-backdrop-filter: blur(16px);
            border: 1px solid var(--border);
            border-radius: 24px;
            padding: 3rem;
            box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);
            text-align: center;
        }

        h1 {
            margin: 0 0 0.5rem 0;
            font-weight: 800;
            font-size: 2.5rem;
            background: linear-gradient(135deg, #a5b4fc, #6366f1);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            letter-spacing: -1px;
        }

        p.subtitle {
            color: var(--text-muted);
            margin-bottom: 2.5rem;
            font-size: 1.1rem;
        }

        .status-badge {
            display: inline-block;
            padding: 0.7rem 2rem;
            border-radius: 999px;
            font-weight: 800;
            font-size: 1.2rem;
            letter-spacing: 1px;
            margin-bottom: 3rem;
            transition: all 0.4s;
            text-transform: uppercase;
        }

        .status-armed {
            background: rgba(34, 197, 94, 0.1);
            color: var(--green);
            border: 1px solid var(--green);
            box-shadow: 0 0 20px var(--green-glow);
        }

        .status-disarmed {
            background: rgba(148, 163, 184, 0.1);
            color: var(--text-muted);
            border: 1px solid var(--text-muted);
        }

        .status-alarm {
            background: rgba(239, 68, 68, 0.15);
            color: var(--red);
            border: 1px solid var(--red);
            animation: pulse-red 1.5s infinite;
        }

        @keyframes pulse-red {
            0% { box-shadow: 0 0 0 0 var(--red-glow); }
            70% { box-shadow: 0 0 0 20px rgba(239, 68, 68, 0); }
            100% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0); }
        }

        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
            gap: 1.5rem;
        }

        .card {
            background: rgba(255, 255, 255, 0.02);
            border: 1px solid var(--border);
            border-radius: 16px;
            padding: 2rem;
            position: relative;
            overflow: hidden;
        }

        .card::before {
            content: '';
            position: absolute;
            top: 0; left: 0; right: 0; height: 2px;
            background: linear-gradient(90deg, transparent, var(--accent), transparent);
            opacity: 0;
            transition: opacity 0.3s;
        }

        .card h3 {
            margin: 0;
            color: var(--text-muted);
            font-size: 1rem;
            text-transform: uppercase;
            letter-spacing: 1px;
        }

        .value {
            font-size: 3rem;
            font-weight: 300;
            margin: 1rem 0 0 0;
            color: var(--text);
            text-shadow: 0 0 15px rgba(255,255,255,0.1);
        }

        .unit {
            font-size: 1.2rem;
            color: var(--accent);
            font-weight: 500;
        }

        .controls {
            margin-top: 3rem;
            display: flex;
            gap: 1rem;
            justify-content: center;
            flex-wrap: wrap;
        }

        button {
            background: rgba(255,255,255,0.05);
            color: var(--text);
            border: 1px solid var(--border);
            padding: 1rem 2rem;
            border-radius: 12px;
            font-size: 1.1rem;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.2s;
            font-family: 'Outfit', sans-serif;
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        button:hover {
            background: rgba(255,255,255,0.1);
        }

        button.primary {
            background: var(--accent);
            border-color: var(--accent);
        }
        
        button.primary:hover {
            background: #4f46e5;
            box-shadow: 0 0 20px var(--accent-glow);
        }

        button.danger {
            background: rgba(239, 68, 68, 0.1);
            color: var(--red);
            border-color: var(--red);
        }

        button.danger:hover {
            background: var(--red);
            color: white;
            box-shadow: 0 0 20px var(--red-glow);
        }

        .danger-bg {
            background-color: #450a0a !important;
            background-image: none !important;
        }
    </style>
</head>
<body>

    <div class="container">
        <div class="glass-panel">
            <h1>Smart Museum</h1>
            <p class="subtitle">Live Telemetry & Control Panel</p>

            <div id="statusBadge" class="status-badge status-disarmed">
                CONNESSIONE INCORSO...
            </div>

            <div class="grid">
                <div class="card">
                    <h3>Distanza</h3>
                    <div class="value" id="val-dist">--<span class="unit">cm</span></div>
                </div>
                <div class="card">
                    <h3>Temperatura</h3>
                    <div class="value" id="val-temp">--<span class="unit">°C</span></div>
                </div>
                <div class="card">
                    <h3>Umidità</h3>
                    <div class="value" id="val-hum">--<span class="unit">%</span></div>
                </div>
                <div class="card">
                    <h3>Luce</h3>
                    <div class="value" id="val-light">--<span class="unit">lux</span></div>
                </div>
            </div>

            <div class="controls">
                <button class="primary" onclick="sendCommand('ARM')">
                    <svg width="20" height="20" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"></path></svg>
                    Attiva (Arm)
                </button>
                <button onclick="sendCommand('DISARM')">
                    <svg width="20" height="20" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0110 0v4"></path></svg>
                    Disattiva
                </button>
                <button class="danger" onclick="sendCommand('MUTE')">
                    <svg width="20" height="20" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M11 5L6 9H2v6h4l5 4V5z"></path><path d="M19.07 4.93a10 10 0 010 14.14M15.54 8.46a5 5 0 010 7.07"></path></svg>
                    Silenzia
                </button>
                <button onclick="window.location.href='/settings'">
                    <svg width="20" height="20" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4"></path></svg>
                    Impostazioni
                </button>
                <button onclick="window.location.href='/debug'">
                    <svg width="20" height="20" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path d="M4 6h16M4 12h16m-7 6h7"></path></svg>
                    Debug
                </button>
            </div>
        </div>
    </div>

    <script>
        const UPDATE_INTERVAL = 3000;

        async function fetchTelemetry() {
            try {
                const response = await fetch('/api/data');
                if(!response.ok) throw new Error('Network response was not ok');
                const data = await response.json();
                
                // Aggiorna valori
                document.getElementById('val-temp').innerHTML = data.temp.toFixed(1) + '<span class="unit">°C</span>';
                document.getElementById('val-hum').innerHTML = data.hum.toFixed(0) + '<span class="unit">%</span>';
                document.getElementById('val-dist').innerHTML = data.dist.toFixed(1) + '<span class="unit">cm</span>';
                document.getElementById('val-light').innerHTML = data.light + '<span class="unit"> val</span>';

                // Gestione Visiva dello Stato
                const badge = document.getElementById('statusBadge');
                if (data.state === 'ALARM_ACTIVE') {
                    badge.className = 'status-badge status-alarm';
                    badge.innerText = 'INTRUSIONE RILEVATA';
                    document.body.classList.add('danger-bg');
                } else if (data.state === 'ARMED') {
                    badge.className = 'status-badge status-armed';
                    badge.innerText = 'SISTEMA ARMATO E SICURO';
                    document.body.classList.remove('danger-bg');
                } else {
                    badge.className = 'status-badge status-disarmed';
                    badge.innerText = 'SISTEMA DISARMATO (MANUTENZIONE)';
                    document.body.classList.remove('danger-bg');
                }

            } catch (error) {
                console.error("Errore fetch telemetria:", error);
            }
        }

        async function sendCommand(command) {
            try {
                const res = await fetch('/api/action', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: 'cmd=' + command
                });
                if(res.ok) {
                    // Forza fetch immediato per snappiness
                    fetchTelemetry();
                } else {
                    alert("Errore invio comando");
                }
            } catch(e) {
                alert("Errore di rete");
            }
        }

        // Avvio ciclo infinito asincrono
        setInterval(fetchTelemetry, UPDATE_INTERVAL);
        fetchTelemetry(); // Init immediato
    </script>
</body>
</html>
)=====";

const char SETTINGS_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="it">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Impostazioni Soglie</title>
    <style>
        :root {
            --bg: #09090b;
            --panel: rgba(24, 24, 27, 0.8);
            --border: rgba(255, 255, 255, 0.08);
            --accent: #6366f1;
            --accent-glow: rgba(99, 102, 241, 0.4);
            --text: #f8fafc;
            --text-muted: #94a3b8;
        }
        body {
            background-color: var(--bg);
            background-image: radial-gradient(circle at 50% 0%, rgba(99,102,241,0.15) 0%, rgba(9,9,11,1) 50%);
            color: var(--text);
            font-family: 'Outfit', sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
        }
        .container {
            width: 95%;
            max-width: 600px;
            padding: 2rem 0;
        }
        .glass-panel {
            background: var(--panel);
            backdrop-filter: blur(16px);
            -webkit-backdrop-filter: blur(16px);
            border: 1px solid var(--border);
            border-radius: 24px;
            padding: 3rem;
            box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);
        }
        h1 {
            text-align: center;
            margin: 0 0 2rem 0;
            font-weight: 800;
            font-size: 2.5rem;
            background: linear-gradient(135deg, #a5b4fc, #6366f1);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .form-group {
            margin-bottom: 1.5rem;
        }
        label {
            display: block;
            margin-bottom: 0.5rem;
            color: var(--text-muted);
        }
        input {
            width: 100%;
            padding: 0.8rem;
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid var(--border);
            border-radius: 8px;
            color: var(--text);
            font-family: 'Outfit', sans-serif;
            font-size: 1rem;
            box-sizing: border-box;
        }
        input:focus {
            outline: none;
            border-color: var(--accent);
        }
        .controls {
            display: flex;
            gap: 1rem;
            margin-top: 2rem;
        }
        button {
            flex: 1;
            background: rgba(255,255,255,0.05);
            color: var(--text);
            border: 1px solid var(--border);
            padding: 1rem;
            border-radius: 12px;
            font-size: 1.1rem;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.2s;
            font-family: 'Outfit', sans-serif;
        }
        button:hover {
            background: rgba(255,255,255,0.1);
        }
        button.primary {
            background: var(--accent);
            border-color: var(--accent);
        }
        button.primary:hover {
            background: #4f46e5;
            box-shadow: 0 0 20px var(--accent-glow);
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="glass-panel">
            <h1>Impostazioni Soglie</h1>
            <div class="form-group">
                <label>Distanza Minima Allarme (cm)</label>
                <input type="number" id="dist" step="1">
            </div>
            <div class="form-group">
                <label>Luminosità Massima (lux / val)</label>
                <input type="number" id="light" step="1">
            </div>
            <div class="form-group">
                <label>Umidità Massima (%)</label>
                <input type="number" id="hum" step="0.1">
            </div>
            <div class="form-group">
                <label>Temperatura Massima (°C)</label>
                <input type="number" id="temp" step="0.1">
            </div>
            <div class="controls">
                <button onclick="window.location.href='/'">Indietro</button>
                <button class="primary" onclick="salvaImpostazioni()">Salva Valori</button>
            </div>
        </div>
    </div>
    <script>
        async function fetchSettings() {
            try {
                const response = await fetch('/api/settings');
                const data = await response.json();
                document.getElementById('dist').value = data.dist;
                document.getElementById('light').value = data.light;
                document.getElementById('hum').value = data.hum;
                document.getElementById('temp').value = data.temp;
            } catch (error) {
                console.error("Errore fetch impostazioni:", error);
            }
        }
        async function salvaImpostazioni() {
            const formData = new URLSearchParams();
            formData.append('dist', document.getElementById('dist').value);
            formData.append('light', document.getElementById('light').value);
            formData.append('hum', document.getElementById('hum').value);
            formData.append('temp', document.getElementById('temp').value);

            try {
                const res = await fetch('/api/settings', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: formData.toString()
                });
                if(res.ok) {
                    alert("Impostazioni salvate con successo!");
                } else {
                    alert("Errore salvataggio!");
                }
            } catch(e) {
                alert("Errore di rete");
            }
        }
        fetchSettings();
    </script>
</body>
</html>
)=====";

const char DEBUG_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="it">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Console di Debug</title>
    <style>
        :root {
            --bg: #09090b;
            --panel: rgba(24, 24, 27, 0.9);
            --border: rgba(255, 255, 255, 0.1);
            --text: #22c55e;
            --accent: #6366f1;
        }
        body {
            background-color: var(--bg);
            background-image: radial-gradient(circle at 50% 0%, rgba(99,102,241,0.1) 0%, rgba(9,9,11,1) 50%);
            color: var(--text);
            font-family: 'Fira Code', monospace;
            margin: 0;
            padding: 2rem;
            display: flex;
            flex-direction: column;
            height: 100vh;
            box-sizing: border-box;
        }
        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1rem;
            font-family: 'Outfit', sans-serif;
        }
        h1 {
            margin: 0;
            color: #f8fafc;
            font-size: 1.8rem;
        }
        button {
            background: rgba(255, 255, 255, 0.05);
            color: white;
            border: 1px solid var(--border);
            padding: 0.8rem 1.5rem;
            border-radius: 8px;
            font-family: 'Outfit', sans-serif;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.2s;
        }
        button:hover {
            background: rgba(255,255,255,0.1);
        }
        .terminal {
            flex: 1;
            background: var(--panel);
            backdrop-filter: blur(10px);
            border: 1px solid var(--border);
            border-radius: 12px;
            padding: 1.5rem;
            overflow-y: auto;
            position: relative;
            box-shadow: 0 10px 30px rgba(0,0,0,0.5);
            font-size: 0.95rem;
            line-height: 1.5;
        }
        .line {
            margin-bottom: 0.3rem;
            word-wrap: break-word;
        }
        .line .time {
            color: #94a3b8;
            margin-right: 10px;
        }
        
    </style>
</head>
<body>
    <div class="header">
        <h1>Console di Debug</h1>
        <button onclick="window.location.href='/'">Torna alla Dashboard</button>
    </div>
    <div class="terminal" id="terminal">
        <div class="line">=== SMART MUSEUM SERIAL CONSOLE ===</div>
        <div class="line">In attesa di log...</div>
    </div>

    <script>
        const terminal = document.getElementById('terminal');
        let lastLogs = "";

        async function fetchLogs() {
            try {
                const response = await fetch('/api/debug_data');
                const text = await response.text();
                
                if (text !== lastLogs) {
                    lastLogs = text;
                    const lines = text.split('\n').filter(l => l.trim() !== '');
                    let html = '';
                    lines.forEach(line => {
                        html += `<div class="line">${line}</div>`;
                    });
                    terminal.innerHTML = html;
                    terminal.scrollTop = terminal.scrollHeight; // tail -f
                }
            } catch (error) {
                console.error("Errore fetch log:", error);
            }
        }

        setInterval(fetchLogs, 1000);
        fetchLogs();
    </script>
</body>
</html>
)=====";

#endif // DASHBOARD_H
