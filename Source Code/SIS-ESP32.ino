// Code for ESP32 to manage WiFi AP, web server, and interface with Arduino for rocket launch control

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

//AP Configuration
const char* ssid = "Launch-Control-AP";
const char* password = "sis@2025";

const int triggerPin = 4;    // Output pin to Arduino (connect to Arduino pin 7)
const int statusLED = 16;    // Status LED (built-in or external)
const int systemLED = 17;    // System status LED

WebServer server(80);

// System variables
bool systemArmed = false;
bool launchInProgress = false;
unsigned long lastLaunchTime = 0;
int totalLaunches = 0;
unsigned long systemUptime = 0;
unsigned long lastHeartbeat = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialized pins
  pinMode(triggerPin, OUTPUT);
  pinMode(statusLED, OUTPUT);
  pinMode(systemLED, OUTPUT);
  
  digitalWrite(triggerPin, HIGH); // Default HIGH (Arduino uses INPUT_PULLUP)
  digitalWrite(statusLED, LOW);
  digitalWrite(systemLED, LOW);
  
  // Initialize SPIFFS for web files
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
  }
  
  // Setup WiFi AP
  setupWiFiAP();
  
  // Setup web server routes
  setupWebServer();
  
  // Start web server
  server.begin();
  
  Serial.println("========================================");
  Serial.println("    ESP32 ROCKET LAUNCH CONTROLLER    ");
  Serial.println("========================================");
  Serial.println("WiFi AP: " + String(ssid));
  Serial.println("Password: " + String(password));
  Serial.println("Web Interface: http://192.168.4.1");
  Serial.println("System Status: READY");
  Serial.println("========================================");
  
  systemArmed = true;
  digitalWrite(systemLED, HIGH);
}

void loop() {
  server.handleClient();
  updateSystemStatus();
  
  
  if (millis() - lastHeartbeat > 500) {
    lastHeartbeat = millis();
    digitalWrite(statusLED, !digitalRead(statusLED));
  }
  
  delay(10);
}

void setupWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

void setupWebServer() {
  // API endpoints
  server.on("/", handleRoot);
  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/api/launch", HTTP_POST, handleLaunch);
  server.on("/api/arm", HTTP_POST, handleArmSystem);
  server.on("/api/disarm", HTTP_POST, handleDisarmSystem);
  server.onNotFound(handleNotFound);
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Launch Control Dashboard</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Courier New', monospace;
            background: linear-gradient(135deg, #0c0c0c 0%, #1a1a2e 50%, #16213e 100%);
            color: #00ff41;
            min-height: 100vh;
            overflow-x: hidden;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
            position: relative;
        }
        
        .header h1 {
            font-size: 2.5rem;
            text-shadow: 0 0 20px #00ff41;
            margin-bottom: 10px;
            animation: glow 2s ease-in-out infinite alternate;
        }
        
        @keyframes glow {
            from { text-shadow: 0 0 20px #00ff41, 0 0 30px #00ff41; }
            to { text-shadow: 0 0 30px #00ff41, 0 0 40px #00ff41; }
        }
        
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .status-card {
            background: rgba(0, 255, 65, 0.1);
            border: 1px solid #00ff41;
            border-radius: 10px;
            padding: 20px;
            backdrop-filter: blur(10px);
            transition: all 0.3s ease;
        }
        
        .status-card:hover {
            background: rgba(0, 255, 65, 0.15);
            box-shadow: 0 0 20px rgba(0, 255, 65, 0.3);
        }
        
        .status-card h3 {
            color: #ff6b00;
            margin-bottom: 10px;
            font-size: 1.2rem;
        }
        
        .status-value {
            font-size: 1.5rem;
            font-weight: bold;
            color: #00ff41;
        }
        
        .control-panel {
            background: rgba(0, 0, 0, 0.7);
            border: 2px solid #ff6b00;
            border-radius: 15px;
            padding: 30px;
            text-align: center;
            margin-bottom: 20px;
        }
        
        .launch-button {
            background: linear-gradient(45deg, #ff1744, #ff6b00);
            border: none;
            color: white;
            padding: 20px 40px;
            font-size: 1.5rem;
            border-radius: 50px;
            cursor: pointer;
            margin: 10px;
            transition: all 0.3s ease;
            text-transform: uppercase;
            font-weight: bold;
            box-shadow: 0 4px 15px rgba(255, 23, 68, 0.4);
        }
        
        .launch-button:hover:not(:disabled) {
            transform: translateY(-3px);
            box-shadow: 0 6px 20px rgba(255, 23, 68, 0.6);
        }
        
        .launch-button:disabled {
            background: #666;
            cursor: not-allowed;
            box-shadow: none;
        }
        
        .arm-button {
            background: linear-gradient(45deg, #4caf50, #8bc34a);
            border: none;
            color: white;
            padding: 12px 25px;
            font-size: 1rem;
            border-radius: 25px;
            cursor: pointer;
            margin: 5px;
            transition: all 0.3s ease;
        }
        
        .disarm-button {
            background: linear-gradient(45deg, #f44336, #ff5722);
        }
        
        .system-log {
            background: rgba(0, 0, 0, 0.8);
            border: 1px solid #00ff41;
            border-radius: 10px;
            padding: 20px;
            height: 200px;
            overflow-y: auto;
            font-family: 'Courier New', monospace;
            font-size: 0.9rem;
        }
        
        .log-entry {
            margin: 5px 0;
            padding: 2px 0;
        }
        
        .log-info { color: #00ff41; }
        .log-warning { color: #ff9800; }
        .log-error { color: #f44336; }
        .log-success { color: #4caf50; }
        
        .armed { color: #4caf50; }
        .disarmed { color: #f44336; }
        .launching { color: #ff9800; animation: blink 0.5s infinite; }
        
        @keyframes blink {
            0%, 50% { opacity: 1; }
            51%, 100% { opacity: 0.3; }
        }
        
        @media (max-width: 768px) {
            .header h1 { font-size: 2rem; }
            .launch-button { 
                padding: 15px 30px; 
                font-size: 1.2rem; 
                width: 100%;
                max-width: 300px;
            }
            .container { padding: 10px; }
        }
        
        .footer {
            text-align: center;
            margin-top: 30px;
            color: #666;
            font-size: 0.8rem;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Stellar Ignition System</h1>
            <p>Launch Control Dashboard</p>
        </div>
        
        <div class="status-grid">
            <div class="status-card">
                <h3>System Status</h3>
                <div class="status-value" id="systemStatus">Loading...</div>
            </div>
            <div class="status-card">
                <h3>Connected Clients</h3>
                <div class="status-value" id="clientCount">0</div>
            </div>
            <div class="status-card">
                <h3>Total Launches</h3>
                <div class="status-value" id="totalLaunches">0</div>
            </div>
            <div class="status-card">
                <h3>System Uptime</h3>
                <div class="status-value" id="uptime">00:00:00</div>
            </div>
            <div class="status-card">
                <h3>Last Launch</h3>
                <div class="status-value" id="lastLaunch">Never</div>
            </div>
            <div class="status-card">
                <h3>Signal Strength</h3>
                <div class="status-value" id="signalStrength">Good</div>
            </div>
        </div>
        
        <div class="control-panel">
            <h2>Control Panel</h2>
            <div style="margin: 20px 0;">
                <button class="arm-button" onclick="armSystem()">Arm System</button>
                <button class="arm-button disarm-button" onclick="disarmSystem()">Disarm System</button>
            </div>
            <button class="launch-button" id="launchBtn" onclick="initiateLaunch()">
                ⚠️ INITIATE LAUNCH SEQUENCE
            </button>
            <p style="margin-top: 15px; color: #ff9800;">
                 Warning: This will trigger the Arduino's countdown sequence
            </p>
        </div>
        
        <div class="status-card">
            <h3>System Log</h3>
            <div class="system-log" id="systemLog"></div>
        </div>
        
        <div class="footer">
            <p>Made with ❤️ by K Rajtilak | https://github.com/rajtilak-2020</p>
        </div>
    </div>

    <script>
        let logEntries = [];
        
        function addLog(message, type = 'info') {
            const timestamp = new Date().toLocaleTimeString();
            logEntries.unshift(`[${timestamp}] ${message}`);
            if (logEntries.length > 50) logEntries.pop();
            
            const logDiv = document.getElementById('systemLog');
            logDiv.innerHTML = logEntries.map(entry => 
                `<div class="log-entry log-${type}">${entry}</div>`
            ).join('');
            logDiv.scrollTop = 0;
        }
        
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('systemStatus').textContent = data.armed ? 'ARMED' : 'DISARMED';
                    document.getElementById('systemStatus').className = data.armed ? 'status-value armed' : 'status-value disarmed';
                    
                    document.getElementById('clientCount').textContent = data.clients;
                    document.getElementById('totalLaunches').textContent = data.totalLaunches;
                    document.getElementById('uptime').textContent = data.uptime;
                    document.getElementById('lastLaunch').textContent = data.lastLaunch;
                    
                    const launchBtn = document.getElementById('launchBtn');
                    if (data.launching) {
                        launchBtn.textContent = '🔥 LAUNCH IN PROGRESS 🔥';
                        launchBtn.className = 'launch-button launching';
                        launchBtn.disabled = true;
                    } else {
                        launchBtn.textContent = '🚀 INITIATE LAUNCH SEQUENCE 🚀';
                        launchBtn.className = 'launch-button';
                        launchBtn.disabled = !data.armed;
                    }
                })
                .catch(error => {
                    addLog('Communication error with control system', 'error');
                });
        }
        
        function initiateLaunch() {
            if (confirm('Are you sure you want to initiate the launch sequence?')) {
                addLog('Launch sequence initiated by operator', 'warning');
                fetch('/api/launch', { method: 'POST' })
                    .then(response => response.json())
                    .then(data => {
                        if (data.success) {
                            addLog('🚀 LAUNCH COMMAND SENT TO ARDUINO! 🚀', 'success');
                        } else {
                            addLog('Launch failed: ' + data.message, 'error');
                        }
                    })
                    .catch(error => {
                        addLog('Launch command failed: Network error', 'error');
                    });
            }
        }
        
        function armSystem() {
            fetch('/api/arm', { method: 'POST' })
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        addLog('System ARMED - Launch control enabled', 'success');
                    }
                });
        }
        
        function disarmSystem() {
            if (confirm('Disarm the launch system?')) {
                fetch('/api/disarm', { method: 'POST' })
                    .then(response => response.json())
                    .then(data => {
                        if (data.success) {
                            addLog('System DISARMED - Launch control disabled', 'warning');
                        }
                    });
            }
        }
        
        // Initialize
        addLog('Control Interface initialized.', 'info');
        addLog('Establishing connection to ignition system...', 'info');
        addLog('System status is nominal - Stand by for launch.', 'success');
        
        // Update status every second
        setInterval(updateStatus, 1000);
        updateStatus();
    </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleGetStatus() {
  DynamicJsonDocument doc(1024);
  
  doc["armed"] = systemArmed;
  doc["launching"] = launchInProgress;
  doc["clients"] = WiFi.softAPgetStationNum();
  doc["totalLaunches"] = totalLaunches;
  doc["uptime"] = formatUptime(millis());
  doc["lastLaunch"] = lastLaunchTime > 0 ? formatTime(lastLaunchTime) : "Never";
  doc["signalStrength"] = "Excellent";
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleLaunch() {
  DynamicJsonDocument doc(256);
  
  if (!systemArmed) {
    doc["success"] = false;
    doc["message"] = "System not armed";
  } else if (launchInProgress) {
    doc["success"] = false;
    doc["message"] = "Launch already in progress";
  } else {
    // Trigger the launch
    triggerLaunch();
    doc["success"] = true;
    doc["message"] = "Launch sequence initiated";
    totalLaunches++;
    lastLaunchTime = millis();
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleArmSystem() {
  systemArmed = true;
  digitalWrite(systemLED, HIGH);
  
  DynamicJsonDocument doc(128);
  doc["success"] = true;
  doc["message"] = "System armed";
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
  
  Serial.println("System ARMED via web interface");
}

void handleDisarmSystem() {
  systemArmed = false;
  digitalWrite(systemLED, LOW);
  
  DynamicJsonDocument doc(128);
  doc["success"] = true;
  doc["message"] = "System disarmed";
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
  
  Serial.println("System DISARMED via web interface");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void triggerLaunch() {
  Serial.println("========================================");
  Serial.println("⚠️ LAUNCH TRIGGER ACTIVATED!");
  Serial.println("Sending signal to Arduino...");
  
  launchInProgress = true;
  
  // Send LOW pulse to Arduino (trigger pin 7)
  digitalWrite(triggerPin, LOW);
  delay(500);  // Hold for 500ms
  digitalWrite(triggerPin, HIGH);
  
  Serial.println("Signal sent to Arduino pin 7");
  Serial.println("Arduino countdown should begin now!");
  Serial.println("========================================");
  
  // Reset launch status after 30 seconds (enough for full countdown)
  delay(30000);
  launchInProgress = false;
}

void updateSystemStatus() {
  // Update system uptime and other status indicators
  systemUptime = millis();
}

String formatUptime(unsigned long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  seconds %= 60;
  minutes %= 60;
  
  char buffer[32];
  sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(buffer);
}

String formatTime(unsigned long ms) {
  unsigned long seconds = (millis() - ms) / 1000;
  if (seconds < 60) return String(seconds) + "s ago";
  if (seconds < 3600) return String(seconds/60) + "m ago";
  return String(seconds/3600) + "h ago";
}