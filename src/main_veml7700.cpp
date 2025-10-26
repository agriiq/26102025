#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_VEML7700.h>
#include <esp_sleep.h>
#include <esp_wifi.h>

#include "secrets.h"

#ifndef WIFI_SSID
#error "Please define WIFI_SSID in secrets.h"
#endif

#ifndef VEML7700_MQTT_CLIENT_ID
#define VEML7700_MQTT_CLIENT_ID "esp32c3-veml7700"
#endif

#ifndef VEML7700_MQTT_TOPIC
#define VEML7700_MQTT_TOPIC "sensors/veml7700"
#endif

#ifndef LUX_TO_PPFD_FACTOR
#define LUX_TO_PPFD_FACTOR 70.0
#endif

#ifndef SLEEP_DURATION_SEC
#define SLEEP_DURATION_SEC 300
#endif

namespace
{
constexpr int kStatusLedPin = 8;
constexpr int kVemlSdaPin = 6;
constexpr int kVemlSclPin = 7;
constexpr unsigned long kReconnectDelayMs = 5000;
constexpr size_t kPayloadSize = 256;
constexpr unsigned long kWiFiTimeoutMs = 30000;
constexpr unsigned long kMqttTimeoutMs = 15000;

Adafruit_VEML7700 veml = Adafruit_VEML7700();
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
}

void setErrorLed(bool on)
{
    digitalWrite(kStatusLedPin, on ? HIGH : LOW);
}

void blinkLed(int times, int delayMs = 200)
{
    for (int i = 0; i < times; i++)
    {
        setErrorLed(true);
        delay(delayMs);
        setErrorLed(false);
        delay(delayMs);
    }
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
        
        if (millis() - startAttempt > kWiFiTimeoutMs)
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
    
    mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    
    Serial.print("MQTT Broker: ");
    Serial.println(MQTT_BROKER_HOST);

    String clientId = String(VEML7700_MQTT_CLIENT_ID) + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    
    unsigned long startAttempt = millis();
    while (!mqttClient.connected())
    {
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
            Serial.println("MQTT verbunden!");
            setErrorLed(false);
            return;
        }

        Serial.print("MQTT Fehler, RC=");
        Serial.println(mqttClient.state());
        
        if (millis() - startAttempt > kMqttTimeoutMs)
        {
            Serial.println("MQTT-Verbindung fehlgeschlagen - gehe in Sleep");
            return;
        }
        
        delay(kReconnectDelayMs);
    }
}

bool initVEML7700()
{
    Wire.begin(kVemlSdaPin, kVemlSclPin);
    
    if (!veml.begin())
    {
        Serial.println("VEML7700 nicht gefunden!");
        return false;
    }

    Serial.println("VEML7700 initialisiert");
    
    veml.setGain(VEML7700_GAIN_1_8);
    veml.setIntegrationTime(VEML7700_IT_100MS);
    
    delay(200);
    
    return true;
}

void publishSensorData()
{
    float lux = veml.readLux();
    
    if (isnan(lux) || lux < 0)
    {
        Serial.println("Ungültiger Lux-Wert!");
        blinkLed(3, 100);
        return;
    }
    
    float ppfd = lux / LUX_TO_PPFD_FACTOR;
    float white = veml.readWhite();
    uint16_t als = veml.readALS();
    
    Serial.println("=== Messwerte ===");
    Serial.print("Lux: ");
    Serial.print(lux, 2);
    Serial.println(" lx");
    Serial.print("PPFD: ");
    Serial.print(ppfd, 2);
    Serial.println(" µmol/m²/s");
    Serial.print("White: ");
    Serial.println(white, 2);
    Serial.print("ALS: ");
    Serial.println(als);
    
    char payload[kPayloadSize];
    snprintf(payload, kPayloadSize,
             "{\"lux\":%.2f,\"ppfd\":%.2f,\"white\":%.2f,\"als\":%u,\"device\":\"%s\"}",
             lux, ppfd, white, als, VEML7700_MQTT_CLIENT_ID);
    
    if (mqttClient.publish(VEML7700_MQTT_TOPIC, payload, false))
    {
        Serial.println("Daten erfolgreich gesendet!");
        blinkLed(1, 100);
    }
    else
    {
        Serial.println("MQTT Publish fehlgeschlagen!");
        blinkLed(3, 100);
    }
}

void enterDeepSleep()
{
    Serial.print("Gehe in Deep Sleep für ");
    Serial.print(SLEEP_DURATION_SEC);
    Serial.println(" Sekunden...");
    Serial.flush();
    
    mqttClient.disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_SEC * 1000000ULL);
    
    setErrorLed(false);
    
    esp_deep_sleep_start();
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=== VEML7700 Lichtsensor Firmware ===");
    Serial.println("Batteriebetrieb mit Deep Sleep");
    Serial.println("=====================================\n");
    
    pinMode(kStatusLedPin, OUTPUT);
    setErrorLed(false);
    
    blinkLed(2, 200);
    
    if (!initVEML7700())
    {
        Serial.println("FEHLER: VEML7700 Init fehlgeschlagen!");
        blinkLed(5, 100);
        delay(5000);
        enterDeepSleep();
        return;
    }
    
    ensureWiFi();
    ensureMqtt();
    
    if (mqttClient.connected())
    {
        publishSensorData();
    }
    else
    {
        Serial.println("Keine MQTT-Verbindung - überspringe Publish");
    }
    
    delay(500);
    
    enterDeepSleep();
}

void loop()
{
    delay(1000);
    enterDeepSleep();
}
