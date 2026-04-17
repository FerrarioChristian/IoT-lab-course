import http.server
import socketserver
import json
import urllib.parse
import re
import time
import os
import threading

# Imposta la directory del file Dashboard.h
DASHBOARD_PATH = "SmartMuseumCase/Dashboard.h"

def extract_html(var_name: str) -> str:
    try:
        if not os.path.exists(DASHBOARD_PATH):
            return f"Errore: {DASHBOARD_PATH} non trovato. Lancia lo script dalla root del progetto."
        with open(DASHBOARD_PATH, 'r', encoding='utf-8') as f:
            content = f.read()
            
        pattern = rf'const char {var_name}\[\] PROGMEM = R"=====\((.*?)\)=====";'
        match = re.search(pattern, content, re.DOTALL)
        if match:
            return match.group(1)
        return f"Errore: variabile {var_name} non trovata in {DASHBOARD_PATH}."
    except Exception as e:
        return f"File error: {e}"

# Stato Globale (Mock dei Sensori)
state = {
    "temp": 24.5,
    "hum": 45.0,
    "light": 420,
    "dist": 12.0,
    "state": "ARMED"
}

# Impostazioni Globali
settings = {
    "dist": 10,
    "light": 930,
    "hum": 60.0,
    "temp": 25.0
}

# Logger
logs = []
start_time = time.time()

def add_log(msg):
    t = time.time() - start_time
    logs.append(f"[{t:.2f}s] {msg}")
    if len(logs) > 20:
        logs.pop(0)

add_log("==== SMART MUSEUM BOOT ====")
add_log("Hardware NodeMCU mock inizializzato.")
add_log("Pronto per le richieste HTTP.")

# Generatore di dati finti
def simulate_sensors():
    import random
    while True:
        state["temp"] += random.uniform(-0.5, 0.5)
        state["hum"] += random.uniform(-1.0, 1.0)
        state["dist"] += random.uniform(-2.0, 2.0)
        
        # Limita valori
        state["dist"] = max(2.0, min(100.0, state["dist"]))
        
        # Check allarmi
        if state["state"] == "ARMED" and state["dist"] < settings["dist"]:
            state["state"] = "ALARM_ACTIVE"
            add_log(f"INTRUSIONE RILEVATA! Distanza: {state['dist']:.1f}cm")
            
        if random.random() < 0.1: # 10% probabilita di nuovo log
            add_log(f"Aggiornamento telemetria: T:{state['temp']:.1f}C H:{state['hum']:.1f}%")
            
        time.sleep(3)

threading.Thread(target=simulate_sensors, daemon=True).start()

class ESPHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            html = extract_html("DASHBOARD_HTML")
            self.wfile.write(html.encode('utf-8'))
            
        elif self.path == "/settings":
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            html = extract_html("SETTINGS_HTML")
            self.wfile.write(html.encode('utf-8'))
            
        elif self.path == "/debug":
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            html = extract_html("DEBUG_HTML")
            self.wfile.write(html.encode('utf-8'))
            
        elif self.path == "/api/data":
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            
            resp_data = state.copy()
            resp_data["th_temp"] = settings["temp"]
            resp_data["th_hum"] = settings["hum"]
            resp_data["th_light"] = settings["light"]
            resp_data["th_dist"] = settings["dist"]
            
            self.wfile.write(json.dumps(resp_data).encode('utf-8'))
            
        elif self.path == "/api/settings":
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(settings).encode('utf-8'))
            
        elif self.path == "/api/debug_data":
            self.send_response(200)
            self.send_header("Content-type", "text/plain")
            self.end_headers()
            out = "\n".join(logs) + "\n"
            self.wfile.write(out.encode('utf-8'))
            
        else:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found")

    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        post_data = self.rfile.read(content_length).decode('utf-8')
        params = urllib.parse.parse_qs(post_data)

        if self.path == "/api/action":
            cmd = params.get('cmd', [''])[0]
            if cmd == "ARM":
                state["state"] = "ARMED"
                add_log("Sistema ARMATO via Web API")
            elif cmd == "DISARM":
                state["state"] = "DISARMED"
                add_log("Sistema DISARMATO via Web API")
            elif cmd == "MUTE":
                state["state"] = "ARMED"
                add_log("Allarme SILENZIATO via Web API")
                
            self.send_response(200)
            self.send_header("Content-type", "text/plain")
            self.end_headers()
            self.wfile.write(b"OK")
            
        elif self.path == "/api/settings":
            if 'dist' in params: settings['dist'] = float(params['dist'][0])
            if 'light' in params: settings['light'] = float(params['light'][0])
            if 'hum' in params: settings['hum'] = float(params['hum'][0])
            if 'temp' in params: settings['temp'] = float(params['temp'][0])
            
            add_log("Impostazioni salvate con successo")
            
            self.send_response(200)
            self.send_header("Content-type", "text/plain")
            self.end_headers()
            self.wfile.write(b"OK")
            
        else:
            self.send_response(404)
            self.end_headers()

PORT = 8080
with socketserver.TCPServer(("", PORT), ESPHandler) as httpd:
    print(f"Mock server in esecuzione su http://localhost:{PORT}")
    print("Premi Ctrl+C per fermarlo.")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nArresto server.")
