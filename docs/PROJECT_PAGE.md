# Space Apps project page: draft text (English)

Copy each section into the matching field on the Space Apps project submission page.

## Project Name
BioNova

## High-Level Project Summary
BioNova is an offline astronaut health dashboard paired with a wearable motion sensor. On Mars a message to Earth can take up to 20 minutes, so astronauts must assess their own health. BioNova combines a live sensor with a 1-minute daily check-in (blood pressure, blood glucose, sleep, mood), shows color-coded levels, 7- and 30-day trends and a clickable 3D anatomy model, and turns each result into one clear next step: self-care, alert the crew, or call the flight surgeon. NASA DONKI space weather and NASA-STD-3001 limits drive its alerts. It runs from a single HTML file with no internet.

## Link to Project Demo
https://github.com/bigsmokecj365/BioNovaTR/blob/main/docs/BioNova_Pitch_EN.pdf

## Link to Final Project
https://github.com/bigsmokecj365/BioNovaTR

## Detailed Project Description
**What it does.** BioNova has two screens. The astronaut screen collects health indicators from a wearable sensor (XIAO ESP32-S3 + MPU6050: motion, posture, steps, falls) and from a short daily check-in typed by the astronaut (blood pressure, blood glucose in mg/dL, sleep hours, mood). Every metric is color-coded, charted over 7 and 30 days, and linked to a body system on a clickable 3D anatomy model. A "?" card explains how to take the measurement, why it matters, and which NASA HRP hazards (radiation, isolation, distance, gravity, hostile environment) affect it. Every alert says who should act: the astronaut, the crew, or the flight surgeon. The ground-control screen receives the same data with the communication delay of the ISS, the Moon or Mars.

**Benefits.** Raw numbers become one clear action, so astronauts neither wait when they should act nor panic when they don't need to. The same approach fits people far from a doctor on Earth: polar stations, ships, offshore platforms and rural clinics.

**How it works.** The dashboard is one HTML file (JavaScript, three.js) that runs offline in any modern browser. The sensor streams over Wi-Fi WebSocket or USB. A small Python relay connects the astronaut and ground-control laptops. Health data stays in the browser.

**Honest status.** Live in the demo: motion, posture, steps, falls, NASA space weather. Typed in: blood pressure, glucose, sleep, mood. Simulated: heart rate, SpO₂, ECG, HRV (our MAX30102 and AD8232 modules failed at the event), temperature, respiration, radiation dose and the 30-day history. The dashboard labels simulated values as "Simulation". Thresholds are general clinical ranges, not yet spaceflight-specific. BioNova is a prototype, not a medical device.

**Next steps.** Spaceflight-specific thresholds reviewed with flight surgeons, tests with crew data from NASA's Open Science Data Repository (OSDR), a working ECG and temperature probe, and a 36 × 35 mm custom board.

## NASA Data
- DONKI space weather API (api.nasa.gov): solar flares (FLR) and solar energetic particle events (SEP) from the last 30 days drive the space-weather card and the shelter alert.
- NASA-STD-3001 technical briefs: radiation protection (OCHMO-TB-020), carbon dioxide (OCHMO-TB-004), sleep accommodations; used as alarm thresholds.
- NASA Human Research Program: The Human Body in Space (five hazards, RIDGE), Human System Risks, Risk of Spaceflight-Induced Bone Changes, Counteracting Bone and Muscle Loss in Microgravity.
- NASA Technical Reports Server: NTRS 20230003916.
- Space Faring: The Radiation Challenge (NASA educator guide).
- NASA 3D Resources: Extravehicular Mobility Unit (EMU) model.

## Space Agency Partner & Other Data
- BodyParts3D © The Database Center for Life Science (DBCLS), CC BY-SA 2.1 JP (3D anatomy).
- three.js + GLTFLoader (MIT), SparkFun MAX3010x library (MIT), arduinoWebSockets (LGPL-2.1), Oswald and Inter fonts (SIL OFL 1.1).
- General clinical reference ranges for blood pressure, glucose, heart rate and SpO₂.
- Hardware: Seeed XIAO ESP32-S3, MPU6050 (GY-521), MAX30102 (HW-605), AD8232.

## Use of Artificial Intelligence
- The teaser video voice-over is AI-generated (stated in the video, its metadata and this page).
- _Team: add the other AI tools you used here (for code, text or images) before submitting._
