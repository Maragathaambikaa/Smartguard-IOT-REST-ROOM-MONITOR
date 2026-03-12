#include <WiFi.h>
#include <WebServer.h>

#define MQ135_PIN 32
#define LDR_PIN 33
#define TRIG_PIN 14
#define ECHO_PIN 12
#define LIGHT_PIN 26
#define BUZZER_PIN 27

const char* ssid = "xxxxx";
const char* password = "xxxxx";

WebServer server(80);

// Thresholds
#define GAS_THRESHOLD 300
#define CO_THRESHOLD 100
#define LDR_THRESHOLD 2000

float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

String interpretLDR(int analogValue) {
  if (analogValue < 40) return "Dark";
  else if (analogValue < 500) return "Dim";
  else if (analogValue < 1000) return "Light";
  else if (analogValue < 2000) return "Bright";
  else return "Very Bright";
}

String createHTML() {
  int gasRaw = analogRead(MQ135_PIN);
  int ldrValue = analogRead(LDR_PIN);
  float distance = readDistanceCM();

  float voltage = gasRaw / 4095.0 * 3.3;
  float resistance = 0.0;
  float ppm = 400.0;

  if (voltage > 0.01 && voltage < 3.2) {
    resistance = (3.3 - voltage) / voltage;
    ppm = 1.0 / (0.03588 * pow(resistance, 1.336));
  }

  float co = ppm / 2.2;
  float methane = ppm / 2.7;
  float ammonia = ppm / 3.6;
  String lightLevel = interpretLDR(ldrValue);

  String html = R"rawliteral(
  <!DOCTYPE html><html><head><meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>Smart Restroom Monitor</title>
  <style>
    * {margin: 0; padding: 0; box-sizing: border-box;}
    body {
      font-family: 'Segoe UI', sans-serif;
      background: linear-gradient(to right, #e6f0ff, #ffffff);
      color: #003366;
      display: flex;
      flex-direction: column;
      min-height: 100vh;
    }
    header {
      background-color: #00264d;
      color: white;
      padding: 20px 0;
      text-align: center;
      font-size: 2rem;
    }
    main {
      flex: 1;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }
    .grid-container {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 20px;
      width: 100%;
      max-width: 900px;
    }
    .card {
      background-color: #007acc;
      color: white;
      padding: 20px;
      border-radius: 15px;
      box-shadow: 0 8px 16px rgba(0, 0, 0, 0.2);
      font-size: 1.1rem;
      text-align: center;
      transition: transform 0.3s ease;
    }
    .card:hover {
      transform: scale(1.05);
    }
    footer {
      background-color: #00264d;
      color: #ccc;
      text-align: center;
      padding: 10px 0;
      font-size: 0.9rem;
    }

    @media screen and (max-width: 600px) {
      .grid-container {
        grid-template-columns: 1fr;
      }
    }
  </style></head><body>
  <header>Smart Restroom IoT Dashboard</header>
  <main>
    <div class='grid-container'>
      <div class='card'>Raw Gas: )rawliteral" + String(gasRaw) + R"rawliteral(</div>
      <div class='card'>CO₂ (ppm): )rawliteral" + String(ppm, 2) + R"rawliteral(</div>
      <div class='card'>CO (ppm): )rawliteral" + String(co, 2) + R"rawliteral(</div>
      <div class='card'>CH₄ (ppm): )rawliteral" + String(methane, 2) + R"rawliteral(</div>
      <div class='card'>NH₃ (ppm): )rawliteral" + String(ammonia, 2) + R"rawliteral(</div>
      <div class='card'>LDR Value: )rawliteral" + String(ldrValue) + R"rawliteral(</div>
      <div class='card'>Light Level: )rawliteral" + lightLevel + R"rawliteral(</div>
      <div class='card'>Distance (cm): )rawliteral" + String(distance, 2) + R"rawliteral(</div>
    </div>
  </main>
  <footer>Smart Monitoring System © 2025</footer>
  </body></html>
  )rawliteral";

  return html;
}

void handleRoot() {
  server.send(200, "text/html", createHTML());
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
  delay(500);
  Serial.print(".");
}

if (WiFi.status() != WL_CONNECTED) {
  Serial.println("\nFailed to connect to WiFi. Check credentials or signal strength.");
} else {
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();

  int gasRaw = analogRead(MQ135_PIN);
  int ldrValue = analogRead(LDR_PIN);
  float distance = readDistanceCM();

  float voltage = gasRaw / 4095.0 * 3.3;
  float resistance = 0.0;
  float ppm = 400.0;
  if (voltage > 0.01 && voltage < 3.2) {
    resistance = (3.3 - voltage) / voltage;
    ppm = 1.0 / (0.03588 * pow(resistance, 1.336));
  }

  float co = ppm / 2.2;

  bool personNearby = distance < 100;
  bool dark = ldrValue > LDR_THRESHOLD;
  bool gasHigh = (gasRaw > GAS_THRESHOLD || co > CO_THRESHOLD);

  digitalWrite(LIGHT_PIN, (personNearby && dark) ? HIGH : LOW);
  digitalWrite(BUZZER_PIN, (personNearby && gasHigh) ? HIGH : LOW);

  

  delay(500);
}

