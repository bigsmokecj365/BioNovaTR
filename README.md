# BioNova — Astronaut Health Dashboard

**NASA Space Apps Challenge 2026 · Bursa, Türkiye**
**Challenge:** [Create Health Monitoring Software for Astronauts on Space Missions](https://www.spaceappschallenge.org/2026/challenges/health-monitoring-software/)
**Team BioNovaTR:** Eymen AKCA · Oğuz FENLİ · Ömer Abdullah DEMİR

> A message from Mars can take up to 20 minutes. BioNova lets astronauts check their own health,
> understand it, and know who should act next, without waiting for Earth.

🇹🇷 Türkçe: [README.tr.md](README.tr.md)

## Demo

| | Link |
|---|---|
| Slide deck (English, 7 slides) | [docs/BioNova_Pitch_EN.pdf](docs/BioNova_Pitch_EN.pdf) · [.pptx](docs/BioNova_Pitch_EN.pptx) |
| Video (28.9 s, English voice-over + English subtitles) | [medya/BioNova_teaser.mp4](medya/BioNova_teaser.mp4) · [subtitles .srt](medya/BioNova_teaser_EN.srt) |
| Dashboard (single file, offline) | [BioNova_Panel.html](BioNova_Panel.html): download it and open it in Chrome or Edge |
| Turkish deck (local jury) | [docs/BioNova_Sunum_TR.pdf](docs/BioNova_Sunum_TR.pdf) · [.pptx](docs/BioNova_Sunum_TR.pptx) |

## What it does

- **Measure:** a wearable motion sensor (XIAO ESP32-S3 + MPU6050) plus a 1-minute daily check-in for blood pressure, blood glucose (mg/dL), sleep and mood.
- **Understand:** color-coded levels, 7- and 30-day trends, and a clickable 3D anatomy model with 6 body systems. Each metric has a "?" card that explains how to measure it, why it matters, and which NASA HRP hazards affect it.
- **Act:** every alert says who should act: **self-care**, **alert the crew**, or **call the flight surgeon**. A second screen for ground control receives the data with the real link delay for the ISS, the Moon and Mars (shortened to 8 s in the demo).
- 8 training scenarios · ISS / Moon / Mars · English and Turkish · runs offline from a single HTML file.

## NASA data and resources

| NASA source | How BioNova uses it |
|---|---|
| DONKI API (api.nasa.gov) | Fetches 30 days of solar flares and SEP events, caches them for offline use, and drives the space-weather card and the "go to shelter" alert |
| NASA-STD-3001 technical briefs | Radiation (OCHMO-TB-020), CO₂ (OCHMO-TB-004) and sleep limits become alarm thresholds |
| HRP: The Human Body in Space (RIDGE) + Human System Risks | Each metric is mapped to a body system and to the 5 spaceflight hazards |
| HRP bone and exercise sources | 1–1.5 %/month bone loss in hips and spine, used for the bone-health estimate |
| NASA 3D Resources: EMU suit | Embedded 3D astronaut that mirrors the wearer's posture |

All links are in the dashboard's **Sources** panel.

## What is real in the demo

- **Live:** motion, posture, steps and falls (MPU6050); NASA DONKI space weather (last cached data when offline)
- **Typed in:** blood pressure, blood glucose, sleep, mood
- **Simulated:** heart rate and SpO₂ (our MAX30102 module failed at the event), ECG and HRV (our AD8232 failed), temperature, respiration, radiation dose, 30-day sample history, 8 scenarios. The dashboard shows a "Simulation" label whenever a sensor is not connected.
- **Limits:** thresholds are general clinical ranges, not yet spaceflight-specific. Posture and fall detection need gravity, so they apply on the Moon and Mars, not in microgravity. BioNova is not a medical device and must not be used for diagnosis.

## How to run

- **Dashboard only:** open `BioNova_Panel.html` in Chrome or Edge. No install and no internet needed.
- **With the sensor (USB or Wi-Fi):** flash `firmware/BioNova_S3/BioNova_S3.ino` with Arduino IDE (board: XIAO_ESP32S3, library: WebSockets by Markus Sattler). Wiring is in `firmware/BAGLANTI.md`.
- **Two screens over a server:** copy the three HTML files into `sunucu/` and run `bash kur.sh` on Ubuntu/Debian (see `sunucu/KURULUM.txt`). Routes: `/astronot`, `/yerkontrol`, `/panel`.
- **Two screens without a server:** run `aktarici/BASLAT.bat` on one laptop (see `aktarici/NASIL_KULLANILIR.txt`).

`BioNova_Astronot.html` and `BioNova_YerKontrol.html` are the same dashboard, opened directly in the astronaut or ground-control role. The server needs all three files.

## Repository layout

| Path | Contents |
|---|---|
| `BioNova_Panel.html` | Dashboard with role selection (astronaut / ground control) |
| `BioNova_Astronot.html`, `BioNova_YerKontrol.html` | The same dashboard locked to one role |
| `docs/` | Slide decks (English and Turkish) as PDF and PPTX |
| `medya/` | Teaser video and concept diagram |
| `firmware/` | XIAO ESP32-S3 code, wiring and perfboard layout |
| `sunucu/` | Small Python server that serves the pages and relays data between screens |
| `aktarici/` | Local relay for two laptops on the same Wi-Fi |

## Use of AI

- The voice-over in the teaser video is AI-generated (also stated in the video and its metadata).

## Licenses and credits

- **Our code:** MIT License, see [`LICENSE`](LICENSE).
- **Third-party content keeps its own license.** The HTML files embed third-party assets, so the MIT license covers only our own code inside them:
  - BodyParts3D © The Database Center for Life Science (DBCLS), **CC BY-SA 2.1 JP**: the anatomy model and anything derived from it stay under CC BY-SA 2.1 JP.
  - NASA 3D Resources EMU model: NASA media usage guidelines.
  - three.js + GLTFLoader: MIT · SparkFun MAX3010x library: MIT · arduinoWebSockets: LGPL-2.1 · Oswald and Inter fonts: SIL OFL 1.1.
- NASA does not endorse this project.
