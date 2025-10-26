# Beitragsrichtlinien

Dieses Repo enthält mehrere Geräte-Firmwares, gebaut mit PlatformIO (Arduino-Framework) für ESP32-C3.

## Branching

- main: stabil, deploybar
- feat/<name>: neue Features
- fix/<name>: Bugfixes

## Commit-Stil

- typ(scope): Zusammenfassung
- Beispiele: `feat(veml7700): deep sleep hinzugefügt`, `fix(bme280): mqtt retry`

## Code-Stil

- .clang-format und .editorconfig beachten (auto-format on save)
- Anonymous namespace für file-lokale Konstanten bevorzugen
- Namen: `kCamelCase` für Konstanten, `snake_case` für lokale Vars, `ensureX()` für Connection-Guards

## Build-Matrix (muss durchlaufen)

- env: `esp32-c3-devkitm-1` (BME280) -> baut `src/main.cpp`
- env: `esp32-c3-veml7700` (VEML7700) -> baut `src/main_veml7700.cpp`

## Secrets

- `include/secrets.h` NICHT committen
- `include/secrets.h.example` nach `include/secrets.h` kopieren und lokal anpassen

## Pre-commit Hooks

Zur Sicherstellung der Code-Qualität gibt es automatische Checks:

```bash
pip install pre-commit
pre-commit install
```

Bei jedem Commit werden dann automatisch ausgeführt:

- clang-format (Code-Formatierung)
- Trailing Whitespace entfernen
- End-of-File Newline prüfen
- Secrets-Check (hardcodierte Passwörter verhindern)

## PR-Checkliste

- [ ] Beide Envs bauen: `pio run -e esp32-c3-devkitm-1` und `pio run -e esp32-c3-veml7700`
- [ ] Keine hardcodierten Zugangsdaten
- [ ] Serial-Logs sind knapp und auf Deutsch
- [ ] MQTT-Topics und Client-IDs folgen Schema `sensors/<device>` und `esp32c3-<device>`
- [ ] CHANGELOG.md aktualisiert (falls relevant)
- [ ] PR-Template ausgefüllt
