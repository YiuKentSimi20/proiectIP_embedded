#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>


const char MAIN_page[] PROGMEM = R"=====( 
<!DOCTYPE html>
<html>
<head>
  <title>Directie masina</title>
  <script>
    document.addEventListener('keydown', function(event) {
      var key = event.key.toLowerCase();
      let path = "";

      if (key === 'w') path = "/Fata";
      else if (key === 's') path = "/Spate";
      else if (key === 'a') path = "/Stanga";
      else if (key === 'd') path = "/Dreapta";
      else if (key === 'x') path = "/Stop";

      if (path) {
        fetch(path).then(response => response.text()).then(console.log).catch(console.error);
      }
    });
  </script>
</head>
<body>
  <center>
    <h1>Directie masina</h1>
    <p>Use keyboard:</p>
    <ul style="list-style: none;">
      <li><b>W</b> = Fata</li>
      <li><b>S</b> = Spate</li>
      <li><b>A</b> = Stanga</li>
      <li><b>D</b> = Dreapta</li>
      <li><b>X</b> = Stop</li>
    </ul>
    <hr>
  </center>
</body>
</html>
)=====";

const char* ssid = "LinksysB019"; // LinksysB019
const char* password = "wm83y3fby4"; // wm83y3fby4



const char* mobile_url = "http://10.100.1.162:8000/robot/api/confirmari_rfid";  // IP backend FastAPI

const char* web_url = "http://10.100.0.95:3001/robot/error";

String statusRobot = "";

ESP8266WebServer server(80);


void handleRoot() {
  Serial.println("You called root page");
  server.send(200, "text/html", MAIN_page);
}

void handleDirection(String dir) {
  Serial.println(dir);
  server.send(200, "text/html", MAIN_page);
}

void handleWebDirection(String dir) {
  Serial.println(dir);
  server.send(200, "text/html", "Comanda trimisa");
}


void setup(void){
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

 

  server.on("/", handleRoot);
  server.on("/Fata", [](){ handleDirection("F"); });
  server.on("/Spate", [](){ handleDirection("B"); });
  server.on("/Stanga", [](){ handleDirection("L"); }); 
  server.on("/Dreapta", [](){ handleDirection("R"); });
  server.on("/Stop", [](){ handleDirection("Stop"); });

  server.on("/fata", [](){ handleWebDirection("F"); });
  server.on("/spate", [](){ handleWebDirection("B"); });
  server.on("/stanga", [](){ handleWebDirection("L"); }); 
  server.on("/dreapta", [](){ handleWebDirection("R"); });
  server.on("/stop", [](){ handleWebDirection("Stop"); });




  server.on("/rc-on", []() { 
    Serial.println("RC-ON");
    server.send(200, "text/html", MAIN_page);
  });


  server.on("/rc-off", []() { 
    Serial.println("RC-OFF");
    server.send(200, "text/html", MAIN_page);
  });

  // Request comanda
  server.on("/comanda", HTTP_POST, []() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
      server.send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
      return;
    }

    String salon = doc["salon"] | "";
    String pat = doc["pat"] | "";
    String rfid = doc["rfid"] | "";

    if (salon == "" || pat == "" || rfid == "") {
      server.send(400, "application/json", "{\"error\": \"Missing fields\"}");
      return;
    }

    Serial.println("Comanda: " + salon + " " + pat + " " + rfid);

    server.send(200, "application/json", "{\"status\": \"Received\"}");
  } else {
    server.send(400, "application/json", "{\"error\": \"Missing body\"}");
  }
  });

  server.on("/status", HTTP_GET, []() {
  StaticJsonDocument<200> doc;
  doc["status"] = statusRobot;
  
  String response;
  serializeJson(doc, response);

  server.send(200, "application/json", response);
  });
  

  server.begin();
  Serial.println("HTTP server started");
}

String serial_string_aux = "";

void loop(void){
  String serial_input = "";

  if (Serial.available()) {       // If anything comes in Serial1 (pins 0 & 1)
    char serial_char = Serial.read();
    serial_string_aux += serial_char;
    if(serial_char == '\n'){
    serial_input = serial_string_aux;
    serial_input.trim();
    serial_string_aux = "";
    }
  }

  if (!serial_input.equals("")) {
    statusRobot = serial_input;
    if (WiFi.status() == WL_CONNECTED) {
      if(statusRobot.startsWith("Rfid validat:")) {

        HTTPClient http_mobile;
        WiFiClient client;
      
        http_mobile.begin(client, mobile_url);
        http_mobile.addHeader("Content-Type", "application/json");

        String uid = statusRobot.substring(statusRobot.indexOf(':') + 2);

        StaticJsonDocument<200> doc;
        doc["rfid_medicament"] = uid;
        
        String requestBody;
        serializeJson(doc, requestBody);

        int httpResponseCode = http_mobile.POST(requestBody);
        String payload = http_mobile.getString();

        Serial.println("Status Code: " + String(httpResponseCode));
        Serial.println("Response: " + payload);

        http_mobile.end();
      }

      if(statusRobot.startsWith("Alarma: ")) {

        HTTPClient http_web;
        WiFiClient client;
        http_web.begin(client, web_url); // Use https and WiFiClientSecure for secure endpoints
        http_web.addHeader("Content-Type", "application/json");

        StaticJsonDocument<200> doc;
        doc["descriere"] = statusRobot;
        
        String requestBody;
        serializeJson(doc, requestBody);

        int httpResponseCode = http_web.POST(requestBody);
        String payload = http_web.getString();


        Serial.println("Status Code: " + String(httpResponseCode));
        Serial.println("Response: " + payload);

        http_web.end();
      }


    }

    

    
  }

  server.handleClient();
}
