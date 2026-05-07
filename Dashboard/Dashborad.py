import streamlit as st
import paho.mqtt.client as mqtt
import pandas as pd
import time
from datetime import datetime
import os

# ==========================================
# 1. CONFIGURATION
# ==========================================
MQTT_BROKER = "192.168.8.138" 
CSV_FILE = "mission_logs.csv"

# ==========================================
# 2. STATE & THREADING (THE FIX)
# ==========================================
# Use a class to hold state so it safely survives Streamlit reruns
class GameState:
    def __init__(self):
        self.current = {}

@st.cache_resource
def get_game_state():
    return GameState()

game_state = get_game_state()

# Cache the MQTT connection so we only ever spawn ONE listener!
@st.cache_resource
def start_mqtt_listener():
    def on_connect(client, userdata, flags, rc):
        print("Dashboard Connected to the Matrix!")
        client.subscribe("game/bomb/status")
        client.subscribe("game/pad/input")

    def on_message(client, userdata, msg):
        topic = msg.topic
        payload = msg.payload.decode('utf-8')

        if topic == "game/pad/input":
            if "start_time" in game_state.current:
                if "guess" not in game_state.current:
                    game_state.current["guess"] = ""
                game_state.current["guess"] += payload

        elif topic == "game/bomb/status":
            if payload.startswith("START:"):
                parts = payload.split(":")
                game_state.current = {
                    "start_time": time.time(),
                    "mode": parts[2] if len(parts) > 2 else "Unknown",
                    "password": parts[3] if len(parts) > 3 else "Unknown",
                    "guess": ""
                }
                print(f"Game Started! Tracking {game_state.current['mode']} mode.")

            elif payload in ["DEFUSED", "BOOM"]:
                if "start_time" in game_state.current:
                    time_taken = round(time.time() - game_state.current["start_time"], 2)
                    
                    record = {
                        "Timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                        "Mode": game_state.current.get("mode", "Unknown"),
                        "Target Password": game_state.current.get("password", "Unknown"),
                        "User Input": game_state.current.get("guess", ""),
                        "Status": "✅ DEFUSED" if payload == "DEFUSED" else "💥 BOOM",
                        "Time Taken (Sec)": time_taken
                    }
                    
                    temp_df = pd.DataFrame([record])
                    
                    # Ensure headers are written if the file is new
                    write_header = not os.path.exists(CSV_FILE)
                    temp_df.to_csv(CSV_FILE, mode='a', header=write_header, index=False)
                    
                    print(f"Record saved! User typed: {record['User Input']}")
                    game_state.current = {} # Clear for next game

    
    if hasattr(mqtt, 'CallbackAPIVersion'):
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    else:
        client = mqtt.Client()
        
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(MQTT_BROKER, 1883, 60)
    
    # loop_start() is an asynchronous built-in MQTT thread! No custom threading needed.
    client.loop_start() 
    return client

# Start the connection (Streamlit will only run this once!)
start_mqtt_listener()

# ==========================================
# 3. THE STREAMLIT UI
# ==========================================
st.set_page_config(page_title="Bomb Defusal Console", page_icon="💣", layout="wide")
st.title("💣 Mission Control Dashboard")
st.markdown("Live tracking of ESP32 Defusal Operations.")

# Load the data safely
try:
    if os.path.exists(CSV_FILE) and os.path.getsize(CSV_FILE) > 0:
        df = pd.read_csv(CSV_FILE)
        # Failsafe: if headers are somehow missing, reset them
        if "Status" not in df.columns:
            df.columns = ["Timestamp", "Mode", "Target Password", "User Input", "Status", "Time Taken (Sec)"]
    else:
        df = pd.DataFrame(columns=["Timestamp", "Mode", "Target Password", "User Input", "Status", "Time Taken (Sec)"])
except Exception:
    df = pd.DataFrame(columns=["Timestamp", "Mode", "Target Password", "User Input", "Status", "Time Taken (Sec)"])

# Top Row Metrics
if not df.empty:
    col1, col2, col3 = st.columns(3)
    total_games = len(df)
    defused_count = len(df[df["Status"] == "✅ DEFUSED"])
    win_rate = round((defused_count / total_games) * 100, 1) if total_games > 0 else 0

    col1.metric("Total Missions", total_games)
    col2.metric("Successful Defusals", defused_count)
    col3.metric("Global Win Rate", f"{win_rate}%")
else:
    st.info("Awaiting first mission data... Play a game to populate the dashboard!")

st.subheader("Mission Logs")

# Display the table
st.dataframe(df.iloc[::-1], use_container_width=True, hide_index=True)

st.write("---")
if st.button("🔄 Refresh Data"):
    st.rerun()