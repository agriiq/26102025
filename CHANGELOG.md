# Changelog

Alle wichtigen Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

Format basiert auf [Keep a Changelog](https://keepachangelog.com/de/1.0.0/),
Versionierung folgt [Semantic Versioning](https://semver.org/lang/de/).

## [Unreleased]

### Hinzugefügt
- VEML7700 Lichtsensor-Firmware mit Deep Sleep (5 Min Intervall)
- PPFD-Schätzung aus Lux-Werten (konfigurierbarer Faktor)
- I2C-Konfiguration für VEML7700 (SDA=GPIO6, SCL=GPIO7)
- Separate PlatformIO-Environments für BME280 und VEML7700
- Repository-Standards: CONTRIBUTING.md, README.md, .editorconfig, .clang-format
- VS Code Integration: Tasks für Build/Upload/Monitor pro Environment
- GitHub Actions CI/CD Pipeline für automatische Builds
- Secrets-Workflow mit .example-Template und .gitignore

### Geändert
- BME280-Firmware in eigenes Environment ausgelagert
- Build-System auf env-spezifische src-Filter umgestellt

## [0.1.0] - 2025-10-26

### Hinzugefügt
- Initiale BME280-Firmware mit SPI-Anbindung
- MQTT-Publishing von Temperatur, Luftfeuchtigkeit, Luftdruck
- OTA-Update-Funktion via MQTT-Command
- Watchdog-Timer (120s Timeout)
- SPIFFS-basiertes Error-Logging (max 16KB)
- WiFi und MQTT Auto-Reconnect
