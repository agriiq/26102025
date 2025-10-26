# VEML7700 Lichtsensor Firmware

Batteriebetriebene ESP32-C3 Firmware für VEML7700 Lichtsensor mit Deep Sleep und MQTT.

## 🌱 Verwendungszweck

Überwachung der Pflanzenbeleuchtung im Gewächshaus unter Greenception GCX 25 LED-Lampe.

## ⚡ Features

- ✅ **I2C-Kommunikation** mit VEML7700 Lichtsensor
- ✅ **Lux-Messung** mit hoher Genauigkeit
- ✅ **PPFD-Berechnung** (µmol/m²/s) für Pflanzenbeleuchtung
- ✅ **Deep Sleep** zwischen Messungen (5 Minuten Intervall)
- ✅ **Batterieschonend** - Stromverbrauch im µA-Bereich im Sleep
- ✅ **MQTT Publishing** der Messwerte
- ✅ **WiFi Auto-Connect** mit Timeout
- ✅ **Status-LED** für Fehlermeldungen
- ✅ **Kalibrierbar** - PPFD-Faktor anpassbar

## 📊 Gemessene Werte

Die Firmware sendet folgende Daten per MQTT (JSON):

```json
{
  "lux": 45000.00,
  "ppfd": 642.86,
  "white": 48500.00,
  "als": 12345,
  "device": "esp32c3-veml7700"
}
```

- **lux**: Beleuchtungsstärke in Lux
- **ppfd**: Photosynthetic Photon Flux Density in µmol/m²/s
- **white**: White Channel Wert (Diagnose)
- **als**: ALS Rohwert (Diagnose)

## 🔧 Hardware

### Verkabelung

| ESP32-C3 Pin | VEML7700 Pin | Funktion |
|--------------|--------------|----------|
| GPIO 6       | SDA          | I2C Data |
| GPIO 7       | SCL          | I2C Clock|
| 3V3          | VIN          | Power    |
| GND          | GND          | Ground   |
| GPIO 8       | -            | Status LED (optional) |

### Sensor-Einstellungen

Optimiert für Gewächshaus/starke LED-Beleuchtung:

- **Gain**: 1/8x (bis zu 120.000 Lux messbar)
- **Integration Time**: 100ms
- **I2C-Adresse**: 0x10 (Standard)

## 🚀 Installation

### 1. Projekt bauen

```bash
# VEML7700-Firmware bauen
pio run -e esp32-c3-veml7700

# Auf ESP32-C3 flashen
pio run -e esp32-c3-veml7700 -t upload

# Serial Monitor
pio device monitor -e esp32-c3-veml7700
```

### 2. Konfiguration anpassen

In `include/secrets.h`:

```cpp
// WiFi
#define WIFI_SSID "Ihr-WLAN"
#define WIFI_PASSWORD "Ihr-Passwort"

// MQTT Broker
#define MQTT_BROKER_HOST "192.168.1.100"

// VEML7700 spezifisch
#define LUX_TO_PPFD_FACTOR 70.0  // Anpassbar nach Kalibrierung
#define SLEEP_DURATION_SEC 300    // 5 Minuten
```

## 📐 PPFD-Kalibrierung

Der Standard-Umrechnungsfaktor **70** ist eine Schätzung für LED-Vollspektrum.

### Kalibrierung mit Referenzsensor

Falls Sie einen echten Quantum-Sensor (z.B. Apogee SQ-Series) haben:

1. Beide Sensoren unter die Lampe platzieren
2. VEML7700 Lux-Wert ablesen: z.B. **56.000 lx**
3. Quantum-Sensor PPFD ablesen: z.B. **800 µmol/m²/s**
4. Faktor berechnen: `56000 / 800 = 70`
5. In `secrets.h` anpassen:

```cpp
#define LUX_TO_PPFD_FACTOR 70.0  // Ihr kalibrierter Wert
```

## 🔋 Batteriebetrieb

### Stromverbrauch

- **Aktiv** (Messung + WiFi + MQTT): ~80-150 mA für ~10 Sekunden
- **Deep Sleep**: ~10-50 µA
- **Durchschnitt** bei 5 Min Intervall: ~0.5 mA

### Batterielaufzeit (Rechnung)

Mit 3x AA Batterien (3000 mAh):

```
Laufzeit ≈ 3000 mAh / 0.5 mA = 6000 Stunden = 250 Tage
```

> **Tipp**: LiPo-Akku (3.7V) mit USB-C Laderegler empfohlen!

## 📡 MQTT Topic

Standard-Topic: `sensors/veml7700`

Subscriber-Beispiel (Mosquitto):

```bash
mosquitto_sub -h 192.168.178.44 -t "sensors/veml7700" -v
```

## 🐛 Troubleshooting

### LED blinkt 5x

**Problem**: Sensor, WiFi oder MQTT-Verbindung fehlgeschlagen

**Lösung**:
1. Verkabelung prüfen (I2C: SDA/SCL)
2. WiFi-Zugangsdaten in `secrets.h` prüfen
3. MQTT Broker erreichbar? (`ping 192.168.178.44`)
4. Serial Monitor prüfen (`pio device monitor`)

### Unrealistische Lux-Werte

**Problem**: Sensor übersteuert oder falsche Gain-Einstellung

**Lösung**:
1. Bei sehr hellem Licht (>100.000 lx): Firmware anpassen
2. In `main_veml7700.cpp` Zeile ~135: `VEML7700_GAIN_1_8` beibehalten
3. Bei schwachem Licht (<1000 lx): auf `VEML7700_GAIN_1` ändern

### Deep Sleep funktioniert nicht

**Problem**: ESP32 wacht nicht auf

**Lösung**:
1. USB-Kabel kann Deep Sleep verhindern (normale Nutzung!)
2. Nach Flash: ESP32 reset und von USB trennen
3. Mit Batterie betreiben für echten Deep Sleep Test

## 📝 Unterschied zu BME280-Firmware

| Feature | BME280 | VEML7700 |
|---------|--------|----------|
| Sensor | Temp/Luftf./Druck | Licht (Lux) |
| Interface | SPI | I2C |
| Deep Sleep | ❌ Nein | ✅ Ja |
| Batteriebetrieb | ❌ Nein | ✅ Ja |
| Dauerbetrieb | ✅ Ja | ❌ Nein |
| Publish | Alle 2.5 Min | Alle 5 Min |

## 🔄 Build-Umgebungen

Das Projekt enthält zwei Geräte-Konfigurationen:

```bash
# BME280 (Original)
pio run -e esp32-c3-devkitm-1

# VEML7700 (Neu)
pio run -e esp32-c3-veml7700
```

## 📚 Bibliotheken

- [Adafruit VEML7700](https://github.com/adafruit/Adafruit_VEML7700) - Sensor-Treiber
- [PubSubClient](https://github.com/knolleary/pubsubclient) - MQTT Client

## 🌟 Optimierungen für Pflanzenbeleuchtung

Die Firmware ist speziell optimiert für:

- ✅ Greenception GCX 25 LED-Lampe
- ✅ Hohe Lichtintensität (bis 120.000 Lux)
- ✅ Batterielaufzeit (Deep Sleep)
- ✅ PPFD-Berechnung für Pflanzenwachstum
- ✅ Langzeitüberwachung (Monate ohne Batteriewechsel)

---

**Happy Growing!** 🌱💡
