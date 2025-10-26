#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_BME280.h>
#include <FS.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <Update.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <esp_ota_ops.h>

#include "secrets.h"

#ifndef WIFI_SSID
#error "Please define WIFI_SSID in secrets.h"
#endif

namespace
{
constexpr int kStatusLedPin = 8;
constexpr int kBmeCsPin = 10;
constexpr int kBmeSckPin = 4;
constexpr int kBmeMisoPin = 5;
constexpr int kBmeMosiPin = 21;
constexpr unsigned long kReconnectDelayMs = 5000;
constexpr size_t kPayloadSize = 256;
constexpr uint32_t kWatchdogTimeoutSec = 10;
constexpr size_t kMaxErrorLogBytes = 16 * 1024;

const char* kOtaCmdTopic = "cmd/" MQTT_CLIENT_ID "/ota";

SPIClass spi = SPIClass(FSPI);
Adafruit_BME280 bme(kBmeCsPin, &spi);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
unsigned long lastPublish = 0;
uint32_t errorCount = 0;
}

void setErrorLed(bool on)
{
    digitalWrite(kStatusLedPin, on ? HIGH : LOW);
}

void ensureWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    setErrorLed(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Verbinde mit WLAN: ");
    Serial.println(WIFI_SSID);

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print('.');
        esp_task_wdt_reset();
        if (millis() - startAttempt > 30000)
        {
            Serial.println("\nWLAN-Verbindung fehlgeschlagen, neuer Versuch in 5s.");
            delay(kReconnectDelayMs);
            WiFi.disconnect(true);
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            startAttempt = millis();
        }
    }

    Serial.print("\nWLAN verbunden, IP: ");
    Serial.println(WiFi.localIP());
    setErrorLed(false);
}

void ensureMqtt()
{
    if (mqttClient.connected())
    {
        return;
    }

    setErrorLed(true);
    Serial.println("\n>>> MQTT NICHT VERBUNDEN - Starte Verbindungsversuch <<<");
    const char* mqttBrokers[] = {MQTT_BROKER_HOST, "host.docker.internal", "mosquitto"};
    const size_t brokerCount = sizeof(mqttBrokers) / sizeof(mqttBrokers[0]);

    Serial.print("MQTT Broker-Kandidaten: ");
    for (size_t i = 0; i < brokerCount; ++i)
    {
        Serial.print(mqttBrokers[i]);
        if (i + 1 < brokerCount)
            Serial.print(", ");
    }
    Serial.println();

    while (!mqttClient.connected())
    {
        for (size_t i = 0; i < brokerCount && !mqttClient.connected(); ++i)
        {
            const char* host = mqttBrokers[i];
            mqttClient.setServer(host, MQTT_BROKER_PORT);

            Serial.print("MQTT: ");
            Serial.println(host);

            String clientId = String(MQTT_CLIENT_ID) + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
            bool ok = false;
            if (strlen(MQTT_USERNAME) > 0)
            {
                ok = mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD);
            }
            else
            {
                ok = mqttClient.connect(clientId.c_str());
            }

            if (ok)
            {
                Serial.print("OK: ");
                Serial.println(host);
                setErrorLed(false);
                mqttClient.subscribe(kOtaCmdTopic);
                break;
            }

            Serial.print("FAIL RC=");
            Serial.println(mqttClient.state());
            delay(1000);
        }

        if (!mqttClient.connected())
        {
            Serial.println("Keine Broker erreichbar — erneuter Gesamtversuch in 5s");
            delay(kReconnectDelayMs);
            esp_task_wdt_reset();
        }
    }
}

void appendErrorLog(const char* msg)
{
    if (!SPIFFS.begin(true))
    {
        return;
    }
    File f = SPIFFS.open("/error.log", FILE_APPEND);
    if (!f)
        return;
    f.println(msg);
    f.close();

    File rf = SPIFFS.open("/error.log", FILE_READ);
    if (rf)
    {
        size_t sz = rf.size();
        rf.close();
        if (sz > kMaxErrorLogBytes)
        {
            SPIFFS.remove("/error.log");
        }
    }
}

bool performOta(const String& url)
{
    Serial.printf("OTA Start: %s\n", url.c_str());
    appendErrorLog("OTA start");
    HTTPClient http;
    if (!http.begin(url))
    {
        appendErrorLog("OTA http.begin failed");
        return false;
    }
    int code = http.GET();
    if (code != HTTP_CODE_OK)
    {
        appendErrorLog("OTA HTTP GET failed");
        http.end();
        return false;
    }
    int contentLength = http.getSize();
    WiFiClient* stream = http.getStreamPtr();

    if (!Update.begin(contentLength > 0 ? contentLength : UPDATE_SIZE_UNKNOWN))
    {
        appendErrorLog("Update.begin failed");
        http.end();
        return false;
    }

    const size_t buffSize = 4096;
    uint8_t buff[buffSize];
    size_t written = 0;
    while (http.connected() && (contentLength > 0 || contentLength == -1))
    {
        size_t avail = stream->available();
        if (avail)
        {
            size_t read = stream->readBytes(buff, min(avail, buffSize));
            if (read > 0)
            {
                size_t w = Update.write(buff, read);
                written += w;
                if (w != read)
                {
                    appendErrorLog("Update.write mismatch");
                    Update.abort();
                    http.end();
                    return false;
                }
            }
        }
        delay(1);
        esp_task_wdt_reset();
        if (contentLength > 0)
        {
            contentLength -= avail ? min((int)avail, (int)buffSize) : 0;
        }
    }

    if (!Update.end())
    {
        appendErrorLog("Update.end failed");
        http.end();
        return false;
    }
    http.end();
    if (!Update.isFinished())
    {
        appendErrorLog("Update not finished");
        return false;
    }

    appendErrorLog("OTA success, rebooting");
    delay(100);
    ESP.restart();
    return true;
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    esp_task_wdt_reset();
    String t = String(topic);
    if (t == kOtaCmdTopic)
    {
        String json;
        json.reserve(length + 1);
        for (unsigned int i = 0; i < length; ++i) json += (char)payload[i];
        int u = json.indexOf("\"url\"");
        int q1 = json.indexOf('"', u + 5);
        int q2 = json.indexOf('"', q1 + 1);
        String url = (u >= 0 && q1 >= 0 && q2 > q1) ? json.substring(q1 + 1, q2) : String();
        if (url.length() > 0)
        {
            appendErrorLog((String("OTA request URL=") + url).c_str());
            performOta(url);
        }
        else
        {
            appendErrorLog("OTA ignored: no url");
        }
    }
}

bool publishSensorData()
{
    if (!mqttClient.connected())
    {
        Serial.println("❌ MQTT NICHT VERBUNDEN - kann nicht publishen!");
        return false;
    }

    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressurePa = bme.readPressure();

    if (isnan(temperature) || isnan(humidity) || isnan(pressurePa))
    {
        Serial.println("Sensorwerte ungültig.");
        return false;
    }

    const float pressureHpa = pressurePa / 100.0f;
    const int rssi = WiFi.RSSI();
    const uint32_t heapFree = ESP.getFreeHeap();
    const uint32_t heapMin = esp_get_minimum_free_heap_size();

    char payload[kPayloadSize];
    int written = snprintf(payload, sizeof(payload),
                           "{\"device\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f,\"rssi\":%d,\"heap_free\":%u,\"heap_min\":%u,\"error_count\":%u,\"uptime\":%lu,\"status\":\"ok\"}",
                           MQTT_CLIENT_ID, temperature, humidity, pressureHpa, rssi, heapFree, heapMin, errorCount, millis()/1000UL);

    if (written <= 0 || written >= static_cast<int>(sizeof(payload)))
    {
        Serial.println("Payload Buffer zu klein.");
        return false;
    }

    bool published = mqttClient.publish(MQTT_TOPIC, payload, false);
    if (published)
    {
        Serial.print("✅ Publiziert auf ");
        Serial.print(MQTT_TOPIC);
        Serial.print(": ");
        Serial.println(payload);
    }
    else
    {
        Serial.print("❌ Publishing fehlgeschlagen! MQTT State: ");
        Serial.print(mqttClient.state());
        Serial.print(" | Connected: ");
        Serial.println(mqttClient.connected() ? "JA" : "NEIN");
    }

    return published;
}

void setup()
{
    pinMode(kStatusLedPin, OUTPUT);
    setErrorLed(true);

    Serial.begin(115200);
    unsigned long serialWaitStart = millis();
    while (!Serial && (millis() - serialWaitStart) < 2000)
    {
        delay(10);
    }

    spi.begin(kBmeSckPin, kBmeMisoPin, kBmeMosiPin, kBmeCsPin);
    bool bmeOk = bme.begin();
    if (!bmeOk)
    {
        Serial.println("BME280 nicht gefunden. LED bleibt an.");
        setErrorLed(true);
        appendErrorLog("BME280 not found");
    }
    else
    {
        Serial.println("BME280 initialisiert.");
        setErrorLed(false);
    }

    esp_task_wdt_init(kWatchdogTimeoutSec, true);
    esp_task_wdt_add(NULL);

    esp_ota_mark_app_valid_cancel_rollback();

    mqttClient.setCallback(mqttCallback);
}

void loop()
{
    ensureWiFi();
    ensureMqtt();

    if (!mqttClient.loop())
    {
        setErrorLed(true);
        return;
    }

    unsigned long now = millis();
    if (now - lastPublish >= PUBLISH_INTERVAL_MS)
    {
        lastPublish = now;
        bool ok = publishSensorData();
        setErrorLed(!ok);
    }

    esp_task_wdt_reset();
}
