import streamlit as st
import paho.mqtt.client as mqtt
import pandas as pd
import time
from datetime import datetime
import os
import threading

# ==========================================
# 1. CONFIGURATION
# ==========================================
MQTT_BROKER = "192.168.8.138"  # <-- Make sure this is your Mac's IP!
CSV_FILE = "mission_logs.csv"

# Ensure the database file exists WITH 6 COLUMNS
if not os.path.exists(CSV_FILE):
    df = pd.DataFrame(columns=["Timestamp", "Mode", "Target Password", "User Input", "Status", "Time Taken (Sec)"])
    df.to_csv(CSV_FILE, index=False)

# ==========================================
# 2. THE BACKGROUND MQTT LISTENER
# ==========================================
if 'mqtt_started' not in st.session_state:
    st.session_state.mqtt_started = True
    st.session_state.current_game = {}

    def on_connect(client, userdata, flags, rc):
        print("Dashboard Connected to the Matrix!")
        client.subscribe("game/bomb/status")
        client.subscribe("game/pad/input") # <-- NEW: Eavesdropping on the keypad!

    def on_message(client, userdata, msg):
        topic = msg.topic
        payload = msg.payload.decode('utf-8')

        # --- A. EAVESDROP ON THE KEYPAD ---
        if topic == "game/pad/input":
            if "start_time" in st.session_state.current_game:
                if "guess" not in st.session_state.current_game:
                    st.session_state.current_game["guess"] = ""
                st.session_state.current_game["guess"] += payload

        # --- B. LISTEN FOR GAME STATUS ---
        elif topic == "game/bomb/status":
            
            # A new game started!
            if payload.startswith("START:"):
                parts = payload.split(":")
                st.session_state.current_game = {
                    "start_time": time.time(),
                    "mode": parts[2] if len(parts) > 2 else "Unknown",
                    "password": parts[3] if len(parts) > 3 else "Unknown",
                    "guess": "" # Reset the typed keys for the new game
                }
                print(f"Game Started! Tracking {st.session_state.current_game['mode']} mode.")

            # The game ended!
            elif payload in ["DEFUSED", "BOOM"]:
                if "start_time" in st.session_state.current_game:
                    time_taken = round(time.time() - st.session_state.current_game["start_time"], 2)
                    
                    # Create the final record
                    record = {
                        "Timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                        "Mode": st.session_state.current_game.get("mode", "Unknown"),
                        "Target Password": st.session_state.current_game.get("password", "Unknown"),
                        "User Input": st.session_state.current_game.get("guess", ""), # <-- NEW LOG
                        "Status": "✅ DEFUSED" if payload == "DEFUSED" else "💥 BOOM",
                        "Time Taken (Sec)": time_taken
                    }
                    
                    # Save to CSV
                    temp_df = pd.DataFrame([record])
                    temp_df.to_csv(CSV_FILE, mode='a', header=False, index=False)
                    print(f"Record saved! User typed: {record['User Input']}")
                    
                    # Clear memory for the next game
                    st.session_state.current_game = {} 

    def mqtt_loop():
        client = mqtt.Client()
        client.on_connect = on_connect
        client.on_message = on_message
        try:
            client.connect(MQTT_BROKER, 1883, 60)
            client.loop_forever()
        except Exception as e:
            print(f"MQTT Error: {e}")

    thread = threading.Thread(target=mqtt_loop, daemon=True)
    thread.start()

# ==========================================
# 3. THE STREAMLIT UI
# ==========================================
st.set_page_config(page_title="Bomb Defusal Console", page_icon="💣", layout="wide")

st.title("💣 Mission Control Dashboard")
st.markdown("Live tracking of ESP32 Defusal Operations.")

# Load the latest data
try:
    df = pd.read_csv(CSV_FILE)
except:
    df = pd.DataFrame(columns=["Timestamp", "Mode", "Target Password", "User Input", "Status", "Time Taken (Sec)"])

# Top Row Metrics
if not df.empty:
    col1, col2, col3 = st.columns(3)
    total_games = len(df)
    defused_count = len(df[df["Status"] == "✅ DEFUSED"])
    win_rate = round((defused_count / total_games) * 100, 1)

    col1.metric("Total Missions", total_games)
    col2.metric("Successful Defusals", defused_count)
    col3.metric("Global Win Rate", f"{win_rate}%")
else:
    st.info("Awaiting first mission data... Play a game to populate the dashboard!")

st.subheader("Mission Logs")

# Display the table
st.dataframe(
    df.iloc[::-1],
    use_container_width=True,
    hide_index=True
)

st.write("---")
if st.button("🔄 Refresh Data"):
    st.rerun()