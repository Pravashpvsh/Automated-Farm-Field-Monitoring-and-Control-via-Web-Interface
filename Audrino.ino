#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ESP32Servo.h> // Use ESP32Servo library

// Replace with your mobile hotspot credentials
const char *ssid = "project1";         
const char *password = "abc12345";   
// Ultrasonic Sensor Pins
const int trigPin = 12;
const int echoPin = 14;

// Soil Moisture Sensor Pin
const int soilMoisturePin = 34;

// DHT11 Sensor
#define DHTPIN 27
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ESP32-CAM IP Address
const char* esp32CamIP = "http://192.168.1.97/"; 

// Servo Motor Pin
const int servoPin = 25;
Servo myServo;

// Web Server
AsyncWebServer server(80);

// Function to measure water level as percentage
long getWaterLevelPercent() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  long distance = duration * 0.034 / 2; // Convert to cm

  // Calculate percentage (0 cm = empty, 11 cm = full)
  long waterPercent = map(distance, 11, 0, 0, 100); // 11 cm = full, 0 cm = empty
  if (waterPercent > 100) waterPercent = 100; // Cap at 100%
  if (waterPercent < 0) waterPercent = 0;     // Floor at 0%
  return waterPercent;
}

void setup() {
  // Serial Monitor
  Serial.begin(115200);

  // Connect to Wi-Fi (Mobile Hotspot)
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Hotspot...");
  }
  Serial.println("Connected to Hotspot");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize Ultrasonic Sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initialize DHT Sensor
  dht.begin();

  // Initialize Servo Motor
  myServo.attach(servoPin);
  myServo.write(90); // Start at 90 degrees

  // Serve JSON data for AJAX
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    long waterLevelPercent = getWaterLevelPercent();
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
    int soilMoisture = analogRead(soilMoisturePin);
    int soilPercent = map(soilMoisture, 4095, 0, 0, 100); // Corrected mapping

    String json = "{";
    json += "\"waterLevel\":" + String(waterLevelPercent) + ",";
    json += "\"temperature\":" + String(temp) + ",";
    json += "\"humidity\":" + String(humidity) + ",";
    json += "\"soilMoisture\":" + String(soilPercent);
    json += "}";

    request->send(200, "application/json", json);
  });

  // Endpoint to turn servo "on" (move to 40 degrees)
  server.on("/servo/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    myServo.write(40);
    request->send(200, "text/plain", "Servo On");
  });

  // Endpoint to turn servo "off" (move to 90 degrees)
  server.on("/servo/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    myServo.write(90);
    request->send(200, "text/plain", "Servo Off");
  });

  // Serve the main HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Automated Farm Field Monitoring and Control via Web Interface</title>
      <style>
        body {
          margin: 0;
          padding: 0;
          font-family: Arial, sans-serif;
          text-align: center;
          color: white;
          overflow: hidden;
          height: 100vh;
          width: 100vw;
        }

        .youtube-bg {
          position: fixed;
          top: 50%;
          left: 50%;
          width: 100vw;
          height: 56.25vw;
          z-index: -1;
          transform: translate(-50%, -50%);
          pointer-events: none;
        }

        .background-video {
          position: fixed;
          top: 0;
          left: 0;
          width: 100%;
          height: 100%;
          object-fit: cover;
          z-index: -1;
        }

        h1 {
          background-color: rgba(76, 175, 80, 0.8);
          color: rgb(255, 255, 255);
          margin: 0;
          padding: 20px;
          font-size: 2.5rem;
        }

        h2 {
          font-size: 1.5rem;
          margin-top: 20px;
          background-color: rgba(231, 224, 25, 0.8);
          display: inline-block;
          padding: 10px 20px;
          border-radius: 10px;
        }

        span {
          font-weight: bold;
          color: #4CAF50;
          font-size: 1.2rem;
        }

        iframe {
          margin: 20px auto;
          border: 2px solid #4CAF50;
          border-radius: 8px;
          background-color: rgba(255, 255, 255, 0.8);
        }

        button {
          font-size: 1rem;
          color: white;
          background-color: #4e4caf;
          border: none;
          padding: 10px 20px;
          margin: 10px;
          border-radius: 5px;
          cursor: pointer;
          transition: background-color 0.3s ease;
        }

        .content {
          position: relative;
          z-index: 1;
          padding: 20px;
        }
        button:hover {
          background-color: #45a049;
        }

        @media (max-width: 768px) {
          iframe {
            width: 100%;
            height: auto;
          }

          button {
            width: 80%;
            font-size: 1.2rem;
          }
        }
      </style>
      <script>
        async function fetchData() {
          try {
            const response = await fetch('/data');
            const data = await response.json();

            document.getElementById('waterLevel').innerText = data.waterLevel + '%';
            document.getElementById('temperature').innerText = data.temperature + ' °C';
            document.getElementById('humidity').innerText = data.humidity + ' %';
            document.getElementById('soilMoisture').innerText = data.soilMoisture + ' %';

            setTimeout(fetchData, 1000);
          } catch (error) {
            console.error('Error fetching data:', error);
          }
        }

        function controlServo(action) {
          fetch('/servo/' + action)
            .then(response => response.text())
            .then(data => console.log(data))
            .catch(error => console.error('Error:', error));
        }

        window.onload = fetchData;
      </script>
    </head>
    <body>
      <iframe 
      class="youtube-bg" 
      src="https://www.youtube.com/embed/idiL_X7rc1s?autoplay=1&mute=1&controls=0&loop=1&playlist=idiL_X7rc1s" 
      allow="autoplay" 
      frameborder="0">
    </iframe>
      <h1>Automated Farm Field Monitoring and Control via Web Interface</h1>
      <h2>Water Level: <span id="waterLevel">Loading...</span></h2>
      <h2>Temperature: <span id="temperature">Loading...</span></h2>
      <h2>Humidity: <span id="humidity">Loading...</span></h2>
      <h2>Soil Moisture:
        <span id="soilMoisture">Loading...</span></h2> 
      <iframe src=")rawliteral" + String(esp32CamIP) + R"rawliteral(" width="900" height="480"></iframe>
      <h2>Water pump Control:</h2>
      <button onclick="controlServo('on')">Turn On</button>
      <button onclick="controlServo('off')">Turn Off</button>
    </body>
    </html>
    )rawliteral";

    request->send(200, "text/html", html);
  });

  // Start server
  server.begin();
}

void loop() {
  // Nothing here; the web server handles everything
