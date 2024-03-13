#include <Arduino.h>  
#include <AsyncTCP.h>  
#include <DNSServer.h>
#include <esp_wifi.h>			//Used for mpdu_rx_disable android workaround
#include <EEPROM.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <M5Atom.h>

DNSServer dnsServer;
AsyncWebServer server(80);

String user_name;
String proficiency;
bool name_received = false;
bool proficiency_received = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Welcome Page</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin: 0;
            padding: 0;
            background-color: #f4f4f4;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
            padding: 50px 20px;
        }
        h1 {
            font-size: 2.5rem;
            margin-bottom: 30px;
            color: #333;
        }
        p {
            font-size: 1.1rem;
            margin-bottom: 30px;
            color: #666;
        }
        .button {
            display: inline-block;
            padding: 15px 30px;
            background-color: #007bff;
            color: #fff;
            text-decoration: none;
            border-radius: 8px;
            transition: background-color 0.3s;
            font-size: 1.2rem;
            border: none;
            cursor: pointer;
        }
        .button:hover {
            background-color: #0056b3;
        }
        @media screen and (max-width: 768px) {
            .container {
                padding: 30px 10px;
            }
            h1 {
                font-size: 2rem;
            }
            p {
                font-size: 1rem;
            }
            .button {
                padding: 12px 25px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>METABIM IoT Electric Plug System</h1>
        <p>Click the button below to continue.</p>
        <a href="dashboard.html" class="button">Continue</a>
    </div>
</body>
</html>

)rawliteral";

class CaptiveRequestHandler : public AsyncWebHandler
{
public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request)
    {
        // request->addInterestingHeader("ANY");
        return true;
    }

    void handleRequest(AsyncWebServerRequest *request)
    {
        request->send_P(200, "text/html", index_html);
    }
};

void setupServer()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      request->send_P(200, "text/html", index_html); 
      Serial.println("Client Connected"); });

    server.on("/dashboard.html", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/dashboard.html", String(), false); });
}

void setup()
{
    M5.begin(true, false, true);
    delay(50);
    M5.dis.fillpix(0x0000ff);
    // your other setup stuff...
    Serial.begin(115200);
    Serial.println();
    Serial.println("Setting up AP Mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("BIM_PLUG_001", "ACLAB2023");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("Setting up Async WebServer");
    setupServer();
    Serial.println("Starting DNS Server");
    dnsServer.start(53, "*", WiFi.softAPIP());
    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); // only when requested from AP
    // more handlers...
    server.begin();
    Serial.println("All Done!");
}

void loop()
{
    dnsServer.processNextRequest();
    if (name_received && proficiency_received)
    {
        Serial.print("Hello ");
        Serial.println(user_name);
        Serial.print("You have stated your proficiency to be ");
        Serial.println(proficiency);
        name_received = false;
        proficiency_received = false;
        Serial.println("We'll wait for the next client now");
    }
}