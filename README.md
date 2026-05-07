# 💣 IoT Bomb Defusal Game

## Overview

This project is an intense, multiplayer IoT game where players race against time to defuse a virtual bomb. It relies on edge-device communication, real-time data logging, and hardware-in-the-loop interactions.

One ESP32 acts as the **Bomb Brain** (displaying sequences and emitting a panic-inducing countdown timer), while a second ESP32 acts as the **Defusal Pad** (where players punch in the sequence). A Python-based **Mission Control Dashboard** tracks live attempts, user inputs, and win/loss statistics.

---

## 🚀 Key Features

* **Multi-Device Sync:** Seamless real-time coordination between two ESP32 microcontrollers.
* **Tension Audio (Panic Beeper):** Dynamic hardware buzzer logic that accelerates its beeping frequency as time runs out.
* **Game Modes:** * **Basic Mode:** Memorize and input a 6-digit numeric code.
  * **Encrypted Mode:** Solve a randomized cipher mapping letters to keypad inputs before time expires.
* **Live Eavesdropping Dashboard:** A Streamlit web interface that tracks the exact keys the user is pressing in real time, logs mission duration, and calculates global win rates.
* **Robust MQTT State Management:** Optimized threading ensures the dashboard captures all game data without freezing or duplicating records.

---

## 🏗️ System Architecture

The ecosystem consists of three main nodes communicating over a local Wi-Fi network using the **MQTT Protocol**.

1. **Bomb MCU (ESP32):**
   * **Hardware:** LCD Display (16x2), Buzzer, Mode Button, Trigger Button.
   * **Role:** Generates passwords/ciphers, dictates the game timer, controls the panic beep rate, and evaluates win/loss conditions.
   * **Topics:** Publishes to `game/bomb/status` (`START`, `DEFUSED`, `BOOM`).

2. **Defusal Pad MCU (ESP32):**
   * **Hardware:** 4x4 Membrane Keypad, 4-Digit 7-Segment Display.
   * **Role:** Displays the remaining time visually, reads player keypad strokes, and transmits them instantly.
   * **Topics:** Listens to `game/bomb/status`. Publishes to `game/pad/input`.

3. **Mission Control Dashboard (Python):**
   * **Tech Stack:** Streamlit, Pandas, Paho-MQTT.
   * **Role:** Subscribes to all topics. Uses an asynchronous background MQTT loop to eavesdrop on keystrokes and log final game outcomes into `mission_logs.csv` without blocking the UI thread.

---

## 🛠️ Tech Stack & Dependencies

**Hardware:**
* 2x ESP32 Microcontrollers
* Components: 4x4 Keypad, 4-Digit 7-Segment, 16x2 LCD, Active Buzzer, Push Buttons.

**Libraries (Arduino C++):**
* `WiFi.h`
* `PubSubClient` (MQTT)
* `SevSeg` (7-Segment Display)
* `Keypad`
* `LiquidCrystal`

**Libraries (Python):**
* `streamlit`
* `paho-mqtt`
* `pandas`

---

## ⚙️ Setup & Installation

1. **Start MQTT Broker:** Ensure an MQTT broker (like Eclipse Mosquitto) is running on your local network. Note the broker's IP address.
2. **Flash ESP32s:**
   * Update the `ssid`, `password`, and `mqtt_server` variables in both `pad_mcu.ino` and `bomb_mcu.ino`.
   * Upload the codes to their respective ESP32 boards.
3. **Launch Dashboard:**
   * Navigate to the `Dashboard` directory.
   * Ensure `mission_logs.csv` is deleted or clear to allow the script to generate fresh headers.
   * Run: `streamlit run Dashborad.py`

---

## 🎮 How to Play

1. Power on both ESP32 units and ensure the Dashboard says "Connected to the Matrix!".
2. On the **Bomb MCU**, press the Mode Button to select either Basic or Encrypted mode.
3. Press the Trigger Button to arm the bomb. The sequence will display.
4. Run to the **Defusal Pad MCU** and punch in the correct code before the buzzer flatlines.
5. Watch the Dashboard live-update your results!