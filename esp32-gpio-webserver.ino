#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

#include "secrets.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* otaPassword = OTA_PASSWORD;

WebServer server(80);

// Output GPIO pins
const int outputPins[] = {4, 5, 16, 17};
const int numberOfOutputs = 4;

// Input GPIO pins
const int inputPins[] = {14, 27, 13};
const int numberOfInputs = 3;


// --------------------------------------------------
// Return HIGH or LOW as text
// --------------------------------------------------

String gpioStateText(int pin) {
  return digitalRead(pin) == HIGH ? "HIGH" : "LOW";
}


// --------------------------------------------------
// Create the ON/OFF buttons for an output
// --------------------------------------------------

String makeButton(int pin) {
  String html = "";

  html += "<p>";

  html += "GPIO ";
  html += pin;
  html += " is currently <strong id='output";
  html += pin;
  html += "'>";
  html += gpioStateText(pin);
  html += "</strong><br>";

  html += "<button onclick='setOutput(";
  html += pin;
  html += ",1)'>GPIO ";
  html += pin;
  html += " ON</button> ";

  html += "<button onclick='setOutput(";
  html += pin;
  html += ",0)'>GPIO ";
  html += pin;
  html += " OFF</button>";

  html += "</p>";

  return html;
}


// --------------------------------------------------
// Main webpage
// --------------------------------------------------

void handleRoot() {
  String html = "";

  html.reserve(6000);

  html += "<!DOCTYPE html>";
  html += "<html>";

  html += "<head>";

  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";

  html += "<title>ESP32 GPIO Control</title>";

  html += "<style>";

  html += "body {";
  html += "font-family: Arial, sans-serif;";
  html += "margin: 30px;";
  html += "background-color: #f5f5f5;";
  html += "}";

  html += "h1 {";
  html += "color: #222;";
  html += "}";

  html += ".box {";
  html += "background-color: white;";
  html += "border: 1px solid #ccc;";
  html += "padding: 15px;";
  html += "margin-bottom: 20px;";
  html += "border-radius: 8px;";
  html += "box-shadow: 0 2px 5px rgba(0,0,0,0.1);";
  html += "}";

  html += "button {";
  html += "padding: 10px 20px;";
  html += "margin: 5px;";
  html += "font-size: 16px;";
  html += "cursor: pointer;";
  html += "}";

  html += ".high {";
  html += "color: green;";
  html += "}";

  html += ".low {";
  html += "color: red;";
  html += "}";

  html += "</style>";

  html += "</head>";

  html += "<body>";

  html += "<h1>ESP32 GPIO Web Server OTA</h1>";

  // Outputs
  html += "<div class='box'>";
  html += "<h2>Outputs</h2>";

  for (int i = 0; i < numberOfOutputs; i++) {
    html += makeButton(outputPins[i]);
  }

  html += "</div>";

  // Inputs
  html += "<div class='box'>";
  html += "<h2>Inputs</h2>";

  for (int i = 0; i < numberOfInputs; i++) {
    int pin = inputPins[i];

    html += "<p>";
    html += "GPIO ";
    html += pin;
    html += " input state: ";

    html += "<strong id='input";
    html += i;
    html += "'>";

    html += gpioStateText(pin);

    html += "</strong>";
    html += "</p>";
  }

  html += "</div>";

  html += "<p>Input states update automatically every 500 milliseconds.</p>";

  // JavaScript
  html += "<script>";

  // Update the colour of HIGH/LOW text
  html += "function setStateColour(element) {";
  html += "if (!element) return;";
  html += "if (element.textContent.trim() === 'HIGH') {";
  html += "element.className = 'high';";
  html += "} else {";
  html += "element.className = 'low';";
  html += "}";
  html += "}";

  // Fetch input states without reloading page
  html += "async function updateInputs() {";

  html += "try {";

  html += "const response = await fetch('/inputs', {";
  html += "cache: 'no-store'";
  html += "});";

  html += "if (!response.ok) return;";

  html += "const text = await response.text();";
  html += "const states = text.split(',');";

  html += "for (let i = 0; i < states.length; i++) {";

  html += "const element = document.getElementById('input' + i);";

  html += "if (element) {";
  html += "element.textContent = states[i];";
  html += "setStateColour(element);";
  html += "}";

  html += "}";

  html += "} catch (error) {";
  html += "console.log('Input update failed');";
  html += "}";

  html += "}";

  // Set an output without reloading the whole webpage
  html += "async function setOutput(pin, state) {";

  html += "try {";

  html += "const response = await fetch('/set?pin=' + pin + '&state=' + state, {";
  html += "cache: 'no-store'";
  html += "});";

  html += "if (!response.ok) {";
  html += "console.log('Output command failed');";
  html += "return;";
  html += "}";

  html += "const element = document.getElementById('output' + pin);";

  html += "if (element) {";
  html += "element.textContent = state === 1 ? 'HIGH' : 'LOW';";
  html += "setStateColour(element);";
  html += "}";

  html += "} catch (error) {";
  html += "console.log('Output command failed');";
  html += "}";

  html += "}";

  // Apply colours on initial page load
  html += "window.addEventListener('load', function() {";

  for (int i = 0; i < numberOfInputs; i++) {
    html += "setStateColour(document.getElementById('input";
    html += i;
    html += "'));";
  }

  for (int i = 0; i < numberOfOutputs; i++) {
    html += "setStateColour(document.getElementById('output";
    html += outputPins[i];
    html += "'));";
  }

  html += "updateInputs();";

  html += "});";

  // Poll input states every 500 ms
  html += "setInterval(updateInputs, 500);";

  html += "</script>";

  html += "</body>";
  html += "</html>";

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");

  server.send(200, "text/html", html);
}


// --------------------------------------------------
// Send only the input states
//
// Example response:
// HIGH,LOW,HIGH
// --------------------------------------------------

void handleInputStates() {
  String states = "";

  for (int i = 0; i < numberOfInputs; i++) {
    if (i > 0) {
      states += ",";
    }

    states += gpioStateText(inputPins[i]);
  }

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "text/plain", states);
}


// --------------------------------------------------
// Change an output
// --------------------------------------------------

void handleSetOutput() {
  if (!server.hasArg("pin") || !server.hasArg("state")) {
    server.send(400, "text/plain", "Missing pin or state argument");
    return;
  }

  int pin = server.arg("pin").toInt();
  int state = server.arg("state").toInt();

  bool validPin = false;

  for (int i = 0; i < numberOfOutputs; i++) {
    if (outputPins[i] == pin) {
      validPin = true;
      break;
    }
  }

  if (!validPin) {
    server.send(400, "text/plain", "Invalid output pin");
    return;
  }

  if (state != 0 && state != 1) {
    server.send(400, "text/plain", "Invalid state");
    return;
  }

  digitalWrite(pin, state == 1 ? HIGH : LOW);

  server.send(200, "text/plain", gpioStateText(pin));
}


// --------------------------------------------------
// Handle unknown URLs
// --------------------------------------------------

void handleNotFound() {
  String message = "File Not Found\n\n";

  message += "URI: ";
  message += server.uri();

  message += "\nMethod: ";

  if (server.method() == HTTP_GET) {
    message += "GET";
  } else {
    message += "POST";
  }

  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " ";
    message += server.argName(i);
    message += ": ";
    message += server.arg(i);
    message += "\n";
  }

  server.send(404, "text/plain", message);
}


// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Starting ESP32 GPIO Web Server");

  // Set output pins
  for (int i = 0; i < numberOfOutputs; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
  }

  // Set input pins
  for (int i = 0; i < numberOfInputs; i++) {
    pinMode(inputPins[i], INPUT);
  }

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println();
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start mDNS for the webpage
  if (MDNS.begin("esp32")) {
    Serial.println("mDNS responder started");
    Serial.println("Web page: http://esp32.local/");
  } else {
    Serial.println("mDNS failed to start");
    Serial.println("Use the IP address shown above");
  }

  // ------------------------------------------------
  // OTA setup
  // ------------------------------------------------

  ArduinoOTA.setHostname("esp32-gpio");
  ArduinoOTA.setPassword(otaPassword);

  ArduinoOTA.onStart([]() {
    Serial.println();
    Serial.println("OTA update starting");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println();
    Serial.println("OTA update complete");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int percent = (progress * 100U) / total;

    Serial.printf("OTA progress: %u%%\r", percent);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error [%u]: ", error);

    if (error == OTA_AUTH_ERROR) {
      Serial.println("Authentication failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connection failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End failed");
    }
  });

  ArduinoOTA.begin();

  Serial.println("OTA updates enabled");
  Serial.println("OTA hostname: esp32-gpio");

  // ------------------------------------------------
  // Web-server routes
  // ------------------------------------------------

  server.on("/", HTTP_GET, handleRoot);
  server.on("/inputs", HTTP_GET, handleInputStates);
  server.on("/set", HTTP_GET, handleSetOutput);

  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println("HTTP server started");
}


// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  delay(1);
}
