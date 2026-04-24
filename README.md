# IoT Pattern Bomb Defusal Game

## Overview

This project is an IoT-based multiplayer game developed for COE 454.
One device acts as a **Bomb**, displaying a sequence and countdown timer, while another device acts as a **Defusal Pad**, where the player must input the correct sequence before time runs out.

The system demonstrates key IoT concepts including sensing, actuation, network communication, edge computing, and performance evaluation.

---

## Game Modes

The game supports multiple modes to enhance gameplay:

* **Classic Mode (Single Round):**
  A single sequence is generated. The player must input it correctly before the timer expires.

* **Survival Mode (Multi-Round):**
  The game continues across multiple rounds.
  Each round becomes progressively harder:

  * The time limit decreases
  * The sequence becomes longer/more complex

---

## System Architecture

The system is divided into three main components:

* **Defusal Pad (ESP32)**
  Captures user input via a keypad and publishes it over MQTT.

* **Bomb Unit (ESP32)**
  Generates sequences, validates input, and controls outputs (LED display + buzzer).

* **Dashboard (Streamlit)**
  Logs game data, tracks scores, and visualizes performance metrics.

All components communicate through an MQTT broker using a publish–subscribe model.

---

## Communication

* **Protocol:** MQTT
* **Broker:** Eclipse Mosquitto (local)
* **Network:** Wi-Fi

Devices exchange data via topics such as:

* `game/input`
* `game/state`
* `game/sequence`
* `game/score`

---

## Tech Stack

* **Hardware:** ESP32
* **Languages:**

  * C/C++ (Arduino IDE)
  * Python (Streamlit)
* **Tools:**

  * Arduino IDE
  * VS Code
* **Libraries:**

  * PubSubClient (ESP32 MQTT)
  * paho-mqtt (Python)
  * Streamlit

---

## Repository Structure

```plaintext
iot-bomb-defusal/
├── ESP32/
│   ├── bomb_mcu/
│   └── pad_mcu/
└── dashboard/
```

---

## Key Features

* Real-time interaction using MQTT
* Multiple game modes (classic & survival)
* Edge-based game logic
* Latency measurement and performance evaluation

---

## Notes

* All devices must be on the same Wi-Fi network
* MQTT broker must be running before starting the system
* Topic names and message formats must be consistent across all components

---
