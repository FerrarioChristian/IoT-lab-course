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
        @import url('https://fonts.googleapis.com/css2?family=Outfit:wght@300;500;800&display=swap');
        
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
            transition: transform 0.3s cubic-bezier(0.175, 0.885, 0.32, 1.275);
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

        .card:hover {
            transform: translateY(-8px);
            border-color: rgba(255,255,255,0.1);
            background: rgba(255, 255, 255, 0.04);
        }

        .card:hover::before { opacity: 1; }

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
            transform: scale(1.05);
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
                    Silenzia Allarme
                </button>
            </div>
        </div>
    </div>

    <script>
        const UPDATE_INTERVAL = 500; // Aggiornamento ogni 500ms

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
                    badge.innerText = '⚠️ INTRUSIONE RILEVATA ⚠️';
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

#endif // DASHBOARD_H
