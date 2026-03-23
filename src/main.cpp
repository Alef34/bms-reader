
// src/main.cpp - ESP32-C3 FIXED VERSION

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"
#include "MakitaBMS.h"
#include <lvgl.h>
#include <TFT_eSPI.h>

// --- Settings and global objects ---
#define ONEWIRE_PIN 4
#define ENABLE_PIN 1

TFT_eSPI tft = TFT_eSPI();

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 10];

// UI prvky
lv_obj_t *label_voltage;
lv_obj_t *label_cells;
lv_obj_t *label_temp;
lv_obj_t *label_status;

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

    DynamicJsonDocument doc(2048);
    doc["type"] = type;

    JsonObject dataObj = doc.createNestedObject("data");
    dataObj["model"] = data.model;
    dataObj["charge_cycles"] = data.charge_cycles;
    dataObj["lock_status"] = data.lock_status;
    dataObj["status_code"] = data.status_code;
    dataObj["mfg_date"] = data.mfg_date;
    dataObj["capacity"] = data.capacity;
    dataObj["battery_type"] = data.battery_type;
    dataObj["pack_voltage"] = data.pack_voltage;

    JsonArray cellV = dataObj.createNestedArray("cell_voltages");
    for (int i = 0; i < 5; i++)
        cellV.add(data.cell_voltages[i]);

    dataObj["cell_diff"] = data.cell_diff;
    dataObj["temp1"] = data.temp1;
    dataObj["temp2"] = data.temp2;
    dataObj["rom_id"] = data.rom_id;

    if (features)
    {
        JsonObject featuresObj = doc.createNestedObject("features");
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

    DynamicJsonDocument doc(512);
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

    DynamicJsonDocument doc(64);
    doc["type"] = "presence";
    doc["present"] = is_present;

    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void logToClients(const String &message, LogLevel level)
{
    Serial.println(message);
    String prefix = (level == LOG_LEVEL_DEBUG) ? "[DBG] " : "";
    sendFeedback("debug", prefix + message);
}

// --- WebSocket events ---

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len)
{

    if (type == WS_EVT_CONNECT)
    {
        Serial.printf("WS client #%lu connected\n", client->id());
        sendPresence(bms.isPresent());
    }

    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.printf("WS client #%lu disconnected\n", client->id());
    }

    else if (type == WS_EVT_DATA)
    {

        DynamicJsonDocument doc(256);

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

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void create_ui()
{
    lv_obj_t *scr = lv_scr_act();

    label_voltage = lv_label_create(scr);
    lv_obj_align(label_voltage, LV_ALIGN_TOP_LEFT, 10, 10);

    label_cells = lv_label_create(scr);
    lv_obj_align(label_cells, LV_ALIGN_TOP_LEFT, 10, 40);

    label_temp = lv_label_create(scr);
    lv_obj_align(label_temp, LV_ALIGN_TOP_LEFT, 10, 70);

    label_status = lv_label_create(scr);
    lv_obj_align(label_status, LV_ALIGN_TOP_LEFT, 10, 100);
}

void updateDisplay()
{
    char buffer[128];

    sprintf(buffer, "Voltage: %.2f V", cached_data.pack_voltage);
    lv_label_set_text(label_voltage, buffer);

    sprintf(buffer, "Cells: %.2f %.2f %.2f %.2f %.2f",
            cached_data.cell_voltages[0],
            cached_data.cell_voltages[1],
            cached_data.cell_voltages[2],
            cached_data.cell_voltages[3],
            cached_data.cell_voltages[4]);
    lv_label_set_text(label_cells, buffer);

    sprintf(buffer, "Temp: %.1f / %.1f C",
            cached_data.temp1,
            cached_data.temp2);
    lv_label_set_text(label_temp, buffer);

    sprintf(buffer, "Cycles: %d", cached_data.charge_cycles);
    lv_label_set_text(label_status, buffer);
}

// --- Setup ---

void setup()
{

    Serial.begin(115200);
    Serial.println("\nStarting Makita BMS Tool...");

    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS mount failed");
        return;
    }

    Serial.println("SPIFFS mounted successfully.");

    bms.setLogCallback(logToClients);

    // ===== WIFI FIX FOR ESP32-C3 =====

    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);

    bool result = WiFi.softAP(ssid, "12345678", 1);

    if (result)
        Serial.println("Access Point started");
    else
        Serial.println("Access Point FAILED");

    Serial.print("SSID: ");
    Serial.println(ssid);

    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());

    // ===== WEBSOCKET =====

    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    dnsServer.start(53, "*", WiFi.softAPIP());

    server.addHandler(new CaptiveRequestHandler());

    server.begin();

    Serial.println("HTTP server with WebSocket is ready.");

    // ===== DISPLAY INIT =====

    tft.begin();
    tft.setRotation(1);

    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH); // backlight

    lv_init();

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    lv_disp_drv_register(&disp_drv);

    create_ui();
}

// --- Loop ---

void loop()
{
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate > 1000)
    {
        updateDisplay();
        lastUpdate = millis();
    }

    lv_timer_handler();
    dnsServer.processNextRequest();
}
