
// src/main.cpp - ESP32-C3 FIXED VERSION
#define USER_SETUP_LOADED

#include <TFT_eSPI.h>
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"
#include "MakitaBMS.h"

#include "esp_task_wdt.h"

TFT_eSPI tft = TFT_eSPI();

// --- Settings and global objects ---
#define ONEWIRE_PIN 4
#define ENABLE_PIN 16

// TFT_eSPI tft = TFT_eSPI();

static bool displayInitialized = false; // track if display is set up

// UI prvky - removed for simple TFT

const char *ssid = "Makita_BMS_Toolicek";

DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

MakitaBMS bms(ONEWIRE_PIN, ENABLE_PIN);

static BatteryData cached_data;

// --- WebSocket JSON sender ---

void sendJsonResponse(const String &type, const BatteryData &data, const SupportedFeatures *features)
{
    if (ws.count() == 0)
        return;

    JsonDocument doc;
    doc["type"] = type;

    JsonObject dataObj = doc["data"].to<JsonObject>();
    dataObj["model"] = data.model;
    dataObj["charge_cycles"] = data.charge_cycles;
    dataObj["lock_status"] = data.lock_status;
    dataObj["status_code"] = data.status_code;
    dataObj["mfg_date"] = data.mfg_date;
    dataObj["capacity"] = data.capacity;
    dataObj["battery_type"] = data.battery_type;
    dataObj["pack_voltage"] = data.pack_voltage;

    JsonArray cellV = dataObj["cell_voltages"].to<JsonArray>();
    for (int i = 0; i < 5; i++)
        cellV.add(data.cell_voltages[i]);

    dataObj["cell_diff"] = data.cell_diff;
    dataObj["temp1"] = data.temp1;
    dataObj["temp2"] = data.temp2;
    dataObj["rom_id"] = data.rom_id;

    if (features)
    {
        JsonObject featuresObj = doc["features"].to<JsonObject>();
        featuresObj["read_dynamic"] = features->read_dynamic;
        featuresObj["led_test"] = features->led_test;
        featuresObj["clear_errors"] = features->clear_errors;
    }

    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void sendFeedback(const String &type, const String &message)
{
    if (ws.count() == 0)
        return;

    JsonDocument doc;
    doc["type"] = type;
    doc["message"] = message;

    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void sendPresence(bool is_present)
{
    if (ws.count() == 0)
        return;

    JsonDocument doc;
    doc["type"] = "presence";
    doc["present"] = is_present;

    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void logToClients(const String &message, LogLevel level)
{
    // Serial.println(message);
    String prefix = (level == LOG_LEVEL_DEBUG) ? "[DBG] " : "";
    sendFeedback("debug", prefix + message);
}

// --- WebSocket events ---

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len)
{

    if (type == WS_EVT_CONNECT)
    {
        // Serial.printf("WS client #%lu connected\n", client->id());
        sendPresence(bms.isPresent());
    }

    else if (type == WS_EVT_DISCONNECT)
    {
        // Serial.printf("WS client #%lu disconnected\n", client->id());
    }

    else if (type == WS_EVT_DATA)
    {

        JsonDocument doc;

        if (deserializeJson(doc, (char *)data) != DeserializationError::Ok)
            return;

        String command = doc["command"];
        String error_msg;

        if (command == "presence")
        {
            sendPresence(bms.isPresent());
        }

        else if (command == "read_static")
        {

            BatteryData fresh_data;
            SupportedFeatures fresh_features;

            error_msg = bms.readStaticData(fresh_data, fresh_features);

            if (error_msg == "")
            {
                cached_data = fresh_data;
                sendJsonResponse("static_data", cached_data, &fresh_features);
                sendPresence(true);
            }
            else
            {
                sendFeedback("error", error_msg);
            }
        }

        else if (command == "read_dynamic")
        {

            error_msg = bms.readDynamicData(cached_data);

            if (error_msg == "")
            {
                sendJsonResponse("dynamic_data", cached_data, nullptr);
            }
            else
            {
                sendFeedback("error", error_msg);
            }
        }

        else if (command == "led_on")
        {

            error_msg = bms.ledTest(true);

            if (error_msg == "")
                sendFeedback("success", "LEDs ON command sent.");
            else
                sendFeedback("error", error_msg);
        }

        else if (command == "led_off")
        {

            error_msg = bms.ledTest(false);

            if (error_msg == "")
                sendFeedback("success", "LEDs OFF command sent.");
            else
                sendFeedback("error", error_msg);
        }

        else if (command == "clear_errors")
        {

            error_msg = bms.clearErrors();

            if (error_msg == "")
                sendFeedback("success", "Clear Errors command sent.");
            else
                sendFeedback("error", error_msg);
        }

        else if (command == "set_logging")
        {

            bool enabled = doc["enabled"];

            bms.setLogLevel(enabled ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO);

            logToClients(
                String("Log level set to ") + (enabled ? "DEBUG" : "INFO"),
                LOG_LEVEL_INFO);
        }
    }
}

// --- Captive portal handler ---

class CaptiveRequestHandler : public AsyncWebHandler
{
public:
    bool canHandle(AsyncWebServerRequest *request) { return true; }

    void handleRequest(AsyncWebServerRequest *request)
    {
        request->send(SPIFFS, "/index.html", "text/html");
    }
};

// LVGL functions removed for simple TFT

// --- Setup ---

void setup()
{

    // Disable watchdog timer to prevent resets during initialization
    esp_task_wdt_deinit();

    Serial.begin(115200);
    delay(1000); // Wait for serial monitor to connect
    Serial.println("\n\n===== Starting Makita BMS Tool =====");

    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS mount failed");
        return;
    }

    Serial.println("SPIFFS mounted OK");

    bms.setLogCallback(logToClients);

    // ===== WIFI FIX FOR ESP32-C3 =====

    WiFi.mode(WIFI_AP_STA);              // Changed from WIFI_AP to WIFI_AP_STA for better stability
    WiFi.persistent(false);              // Don't save settings to flash
    WiFi.setSleep(false);                // Disable WiFi sleep mode
    WiFi.setTxPower(WIFI_POWER_19_5dBm); // Set medium TX power

    // Increase beacon interval for stability
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),  // IP
        IPAddress(192, 168, 4, 1),  // Gateway
        IPAddress(255, 255, 255, 0) // Subnet mask
    );

    bool result = WiFi.softAP(ssid, "12345678", 1, false, 4); // channel 1, no hidden, max 4 clients
    delay(100);                                               // Stability improvement

    if (result)
    {
        Serial.println("Access Point started successfully");
    }
    else
    {
        Serial.println("Access Point FAILED");
    }

    Serial.print("SSID: ");
    Serial.println(ssid);

    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());

    // ===== WEBSOCKET =====

    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    dnsServer.start(53, "*", WiFi.softAPIP());
    delay(50); // Stability improvement

    server.addHandler(new CaptiveRequestHandler());

    server.begin();
    delay(100); // Stability improvement
    Serial.println("HTTP server ready");

    Serial.println("Setup complete - waiting for connections");

    // ===== TFT INIT =====

    Serial.println("TFT init...");

    // ⚠️ HARD RESET SPI + TFT

    // HARD RESET TFT
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);
    delay(50);
    digitalWrite(4, HIGH);
    delay(150);

    // ⚠️ NECHAJ TFT_eSPI robiť SPI!
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("Makita BMS");
    tft.println("READY");

    Serial.println("TFT initialized successfully");
    displayInitialized = true;
}

void loop()
{
    static unsigned long lastUpdate = 0;

    // Display updates disabled (no LVGL to save memory)
    // if (displayInitialized && millis() - lastUpdate > 1000)
    // {
    //     updateDisplay();
    //     lastUpdate = millis();
    // }

    dnsServer.processNextRequest();
    yield();  // Allow background tasks (WiFi watchdog, etc.)
    delay(1); // Prevent watchdog timeout
}
