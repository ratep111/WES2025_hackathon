# 🧭 FIRMA_VCU MQTT Simulator

This is a lightweight terminal-based Python application that connects to an MQTT broker and sends navigational directions (LEFT, RIGHT, STRAIGHT) via MQTT messages. It's ideal for quick prototyping, testing MQTT setups, or integrating into IoT/robotics projects.

---

## 📁 Project Structure

```
.
├── .venv/                  # Python virtual environment
├── app/
│   ├── __init__.py         # Entry point for the app
│   ├── actions.py          # Defines menu callbacks for direction sending
│   ├── mqtt_handler.py     # Handles MQTT connection and publishing
│   ├── terminal_ux.py      # Manages terminal user interface
│   └── assets/
│       └── ascii_art.txt   # Contains splash screen ASCII art
├── run.py                  # Script to launch the app
├── .gitignore
└── README.md
```

---

## 🚀 Getting Started

### 1. Clone the Repository

```bash
git clone <your-repo-url>
cd <your-project-directory>
```

### 2. Set Up Virtual Environment (Optional but Recommended)

```bash
python3 -m venv .venv
source .venv/bin/activate
```

### 3. Install Required Packages

You likely need only `paho-mqtt`:

```bash
pip install paho-mqtt
```

---

## 🧠 How It Works

When you run the application:

1. It prints a cool ASCII art banner from `ascii_art.txt`.
2. Connects to an MQTT broker.
3. Presents a simple terminal menu:
   - First, choose an action (currently just “Send direction”).
   - Then, choose a direction: LEFT, RIGHT, or STRAIGHT.
4. Sends a corresponding MQTT message to the topic `gps/directions`.

The message payload looks like:

```json
{ "direction": "LEFT" }
```

---

## ▶️ Running the App

You can start the app using:

```bash
python run.py
```

Make sure `run.py` contains something like:

```python
from app import start

if __name__ == "__main__":
    start()
```

---

## 📡 MQTT Setup

Update your MQTT connection details inside `mqtt_handler.py`. Example setup:

```python
broker = "mqtt.eclipseprojects.io"
port = 1883
topic = "gps/directions"
```

---

## 🔧 Customization

- Add more directions or topics in `actions.py`.
- Update `ascii_art.txt` with your own custom banner.
- Extend the terminal UX in `terminal_ux.py`.

---

## 📦 Dependencies

- `paho-mqtt`
- `Python 3.7+`

---

## 💡 Example Output

```text

========= MENU =========
Send direction

> Send direction

========= MENU =========
LEFT
RIGHT
STRAIGHT

> LEFT

Sent data via MQTT of a direction LEFT
```

---

## 🛠️ License

This project is provided under the MIT License. Use it freely!

---

## 🤝 Contributions

Contributions, issues, and feature requests are welcome! Feel free to fork the repo and submit pull requests.