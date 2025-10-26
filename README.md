# ESP32-C3 Firmware Lab Stack

Zwei PlatformIO-Environments für ESP32-C3-Geräte:

- BME280 (SPI) – Umweltsensor
- VEML7700 (I2C) – Lichtsensor mit Deep Sleep und PPFD-Schätzung

## Struktur

- `src/main.cpp` – BME280-Firmware
- `src/main_veml7700.cpp` – VEML7700-Firmware
- `include/secrets.h` – lokale Zugangsdaten (nicht committet)
- `include/secrets.h.example` – Vorlage für lokale Secrets
- `platformio.ini` – Build-Environments

## Schnellstart

1. Secrets-Vorlage kopieren und ausfüllen:
   - Windows PowerShell: `Copy-Item include/secrets.h.example include/secrets.h`
2. Eines der Environments bauen:
   - BME280: `pio run -e esp32-c3-devkitm-1`
   - VEML7700: `pio run -e esp32-c3-veml7700`
3. Upload und Monitor nach Bedarf.

## VS Code Tasks

Strg/Cmd+Shift+P → „Run Task" und wählen:

- Build: BME280 (esp32-c3-devkitm-1)
- Upload: BME280 (esp32-c3-devkitm-1)
- Monitor: BME280 (115200)
- Build: VEML7700 (esp32-c3-veml7700)
- Upload: VEML7700 (esp32-c3-veml7700)
- Monitor: VEML7700 (115200)

## MQTT-Topics und IDs

- BME280: Topic `sensors/bme280`, Client-ID `esp32c3-bme280`
- VEML7700: Topic `sensors/veml7700`, Client-ID `esp32c3-veml7700`

## VEML7700-Hinweise

- Deep-Sleep-Intervall: `SLEEP_DURATION_SEC` (Standard 300s)
- PPFD-Näherung: `ppfd = lux / LUX_TO_PPFD_FACTOR` (Standard 70)
- I2C-Pins: SDA=GPIO6, SCL=GPIO7 (ESP32-C3)

## Beitragen

Siehe `CONTRIBUTING.md` für Commit-Stil, Branching und PR-Checkliste. Beide Envs müssen bauen, bevor ein PR geöffnet wird.

## Enterprise-Features

- ✅ GitHub Actions CI/CD (automatische Builds bei jedem PR)
- ✅ Pre-commit Hooks (clang-format, trailing whitespace, secrets check)
- ✅ Issue/PR Templates für strukturierte Zusammenarbeit
- ✅ CODEOWNERS für automatische Review-Zuweisungen
- ✅ CHANGELOG.md mit Semantic Versioning
- ✅ Code-Formatierung (.clang-format, .editorconfig)

### Pre-commit Hooks installieren

```bash
pip install pre-commit
pre-commit install
```

Danach wird der Code bei jedem Commit automatisch formatiert.
