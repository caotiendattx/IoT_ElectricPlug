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
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Điều Khiển Ổ Cắm Thông Minh</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.15.4/css/all.min.css">

    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #f4f4f4;
            display: flex;
            flex-direction: column;
            min-height: 100vh;
        }

        header {
            background-color: #007bff;
            color: #fff;
            padding: 20px;
            text-align: center;
            width: 100%;
            box-shadow: 0px 2px 4px rgba(0, 0, 0, 0.1);
        }

        nav {
            background-color: #333;
            color: #fff;
            padding: 10px;
            text-align: center;
            width: 100%;
        }

        nav a {
            color: #fff;
            text-decoration: none;
            margin: 0 10px;
        }

        .container {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            flex: 1;
            padding: 20px;
        }

        .button-container {
            margin-bottom: 20px;
        }

        .button {
            width: 100px;
            height: 100px;
            border-radius: 50%;
            background-color: #007bff;
            color: #fff;
            font-size: 24px;
            line-height: 100px;
            text-align: center;
            cursor: pointer;
            box-shadow: 0px 0px 20px rgba(0, 0, 0, 0.3);
            transition: background-color 0.3s, box-shadow 0.3s;
        }

        .button:hover {
            background-color: #0056b3;
            box-shadow: 0px 0px 30px rgba(0, 0, 0, 0.5);
        }

        .button.on {
            background-color: #4caf50; /* green */
        }

        .button.off {
            background-color: #f44336; /* red */
        }

        .status {
            font-size: 18px;
            color: #333;
            margin-bottom: 10px;
        }

        .sensor-item {
            font-size: 16px;
            color: #666;
            margin-bottom: 5px;
        }

        footer {
            background-color: #333;
            color: #fff;
            text-align: center;
            padding: 20px;
            width: 100%;
            box-shadow: 0px -2px 4px rgba(0, 0, 0, 0.1);
        }
    </style>
</head>
<body>
    <header>
        <h1>Điều Khiển Ổ Cắm Thông Minh</h1>
    </header>

    <nav>
        <a href="#"><i class="fas fa-home"></i> Trang Chủ</a>
        <a href="#"><i class="fas fa-cogs"></i> Cài Đặt</a>
        <a href="#"><i class="fas fa-info-circle"></i> Giới Thiệu</a>
    </nav>

    <div class="container">
        <div class="button-container">
            <div class="button" id="toggleButton"><i class="fas fa-power-off"></i></div>
        </div>
        <div class="status" id="status">Trạng Thái: Tắt</div>
        
        <div class="sensor-item" id="activePower"><i class="fas fa-bolt"></i> Công Suất Hoạt Động: 0W</div>
        <div class="sensor-item" id="apparentPower"><i class="fas fa-bolt"></i> Công Suất Tương Phản: 0VA</div>
        <div class="sensor-item" id="powerFactor"><i class="fas fa-angle-double-right"></i> Hệ Số Công Suất: 0</div>
        <div class="sensor-item" id="kwh"><i class="fas fa-leaf"></i> KW/Giờ: 0</div>
    </div>

    <footer>
        &copy; 2024 Giải Pháp Ổ Cắm Thông Minh
    </footer>

    <script src="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.15.4/js/all.min.js"></script>
<script>
        const button = document.getElementById('toggleButton');
        const statusText = document.getElementById('status');
        let isOn = false;

        function updateStatus() {
            statusText.textContent = `Status: ${isOn ? 'On' : 'Off'}`;
        }

        button.addEventListener('click', function() {
            isOn = !isOn;
            button.classList.toggle('on', isOn);
            button.classList.toggle('off', !isOn);
            updateStatus();
        });

        // Initial status update
        updateStatus();
    </script>
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