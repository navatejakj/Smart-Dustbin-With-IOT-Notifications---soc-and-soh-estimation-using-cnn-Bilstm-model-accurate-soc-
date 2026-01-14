#include <ESP8266WiFi.h>  
#include <ESP8266WebServer.h>  
#include <Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimeLib.h>

// Wi-Fi credentials
const char* ssid = "vivo 1938";
const char* password = "123456789";

// Relay pins
const int relayA = D1; // Inlet Pump
const int relayB = D2; // Outlet Pump
const int relayPin = D7; // Filter Motor

// Servo for feeding
const int servoPin = D4;
Servo feedServo;

// DS18B20 Temperature Sensor
const int oneWireBus = D4;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

// Ultrasonic Sensor Pins
const int trigPin = D5;
const int echoPin = D6;

// Web server
ESP8266WebServer server(80);

// Status Variables
String inletStatus = "OFF";
String outletStatus = "OFF";
float waterTemperature = 0.0;
String temperatureAlert = "Normal";
float waterLevel = 0.0;
String waterLevelAlert = "Normal";
bool filterState = false;

// Scheduling Variables
int pumpDay = -1, pumpHour = -1, pumpMinute = -1;
bool pumpScheduleEnabled = false;

int feedDay = -1, feedHour = -1, feedMinute = -1;
bool feedScheduleEnabled = false;

// Function to update webpage status via AJAX
void sendStatus() {
  String json = "{";
  json += "\"inlet\":\"" + inletStatus + "\",";
  json += "\"outlet\":\"" + outletStatus + "\",";
  json += "\"temperature\":\"" + String(waterTemperature) + "°C\",";
  json += "\"temperatureStatus\":\"" + temperatureAlert + "\",";
  json += "\"waterLevel\":\"" + String(waterLevel) + " cm\",";
  json += "\"waterLevelStatus\":\"" + waterLevelAlert + "\",";
  json += "\"filter\":\"" + String(filterState ? "ON" : "OFF") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// Handle Web Page
void handleRoot() {
  String html = "<html>\n"
                "<head><title>Smart Aquarium</title>\n"
                "<meta http-equiv='refresh' content='3'>\n"
                "<script>\n"
                "function updateStatus() {\n"
                "  fetch('/status')\n"
                "  .then(response => response.json())\n"
                "  .then(data => {\n"
                "    document.getElementById('inlet').innerText = data.inlet;\n"
                "    document.getElementById('outlet').innerText = data.outlet;\n"
                "    document.getElementById('temperature').innerText = data.temperature;\n"
                "    document.getElementById('temperatureStatus').innerText = data.temperatureStatus;\n"
                "    document.getElementById('waterLevel').innerText = data.waterLevel;\n"
                "    document.getElementById('waterLevelStatus').innerText = data.waterLevelStatus;\n"
                "    document.getElementById('filter').innerText = data.filter;\n"
                "  });\n"
                "}\n"
                "setInterval(updateStatus, 3000);\n"
                "</script>\n"
                "</head>\n"
                "<body onload='updateStatus()'>\n"
                "<h1>Smart Aquarium Control</h1>\n"
                "<p>Inlet Pump: <span id='inlet'>OFF</span></p>\n"
                "<p>Outlet Pump: <span id='outlet'>OFF</span></p>\n"
                "<p>Water Temperature: <span id='temperature'>0°C</span></p>\n"
                "<p>Temperature Status: <span id='temperatureStatus'>Normal</span></p>\n"
                "<p>Water Level: <span id='waterLevel'>0 cm</span></p>\n"
                "<p>Water Level Status: <span id='waterLevelStatus'>Normal</span></p>\n"
                "<p>Filter Motor: <span id='filter'>OFF</span></p>\n"
                "<h3>Schedule Pump Operation</h3>\n"
                "<button onclick=\"location.href='/start'\">Start Pumps</button>\n"
                "<button onclick=\"location.href='/stop'\">Stop Pumps</button>\n"
                "<button onclick=\"location.href='/feed'\">Feed Fish</button>\n"
                "<button onclick=\"location.href='/on'\">Turn ON Filter</button>\n"
                "<button onclick=\"location.href='/off'\">Turn OFF Filter</button>\n"
                "</body>\n"
                "</html>";
  server.send(200, "text/html", html);
}

// Start Pumps
void startPumps() {
  digitalWrite(relayA, LOW);
  inletStatus = "ON";
  delay(5000);
  digitalWrite(relayA, HIGH);
  inletStatus = "OFF";
  
  digitalWrite(relayB, LOW);
  outletStatus = "ON";
  delay(5000);
  digitalWrite(relayB, HIGH);
  outletStatus = "OFF";
  
  sendStatus();
}

// Stop Pumps
void stopPumps() {
  digitalWrite(relayA, HIGH);
  digitalWrite(relayB, HIGH);
  inletStatus = "OFF";
  outletStatus = "OFF";
  sendStatus();
}

// Feed Fish
void feedFish() {
  feedServo.write(120);
  delay(250);
  feedServo.write(0);
  
  server.sendHeader("Location", "/");
  server.send(303);  // Redirects back to the main page
}

// Turn Filter ON/OFF
void handleOn() { filterState = true; digitalWrite(relayPin, LOW); sendStatus(); }
void handleOff() { filterState = false; digitalWrite(relayPin, HIGH); sendStatus(); }

// Schedule Pump Operation
void handleSchedulePump() {
  pumpDay = server.arg("day").toInt();
  pumpHour = server.arg("hour").toInt();
  pumpMinute = server.arg("minute").toInt();
  pumpScheduleEnabled = true;
  sendStatus();
}

// Schedule Fish Feeding
void handleScheduleFeed() {
  feedDay = server.arg("day").toInt();
  feedHour = server.arg("hour").toInt();
  feedMinute = server.arg("minute").toInt();
  feedScheduleEnabled = true;
  sendStatus();
}

// Setup
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  Serial.println("\nConnecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  pinMode(relayA, OUTPUT);
  pinMode(relayB, OUTPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayA, HIGH);
  digitalWrite(relayB, HIGH);
  digitalWrite(relayPin, HIGH);

  feedServo.attach(servoPin);
  feedServo.write(0);

  sensors.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Web Server Routes
  server.on("/", handleRoot);
  server.on("/start", startPumps);
  server.on("/stop", stopPumps);
  server.on("/feed", feedFish);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/schedulePump", handleSchedulePump);
  server.on("/scheduleFeed", handleScheduleFeed);
  server.on("/status", sendStatus);

  server.begin();
}

// Loop
void loop() {
  server.handleClient();
  sensors.requestTemperatures();
  waterTemperature = sensors.getTempCByIndex(0);
  waterLevel = pulseIn(echoPin, HIGH) * 0.034 / 2;
}