import serial
import csv
from datetime import datetime
from pathlib import Path

PORT = "COM4"
BAUD = 115200
FILE = Path.home() / "Documents" / "bme680_live_log.csv"

headers = [
    "Computer Time",
    "Arduino Millis",
    "IAQ",
    "Accuracy",
    "Temperature_C",
    "Humidity_Percent",
    "CO2_ppm",
    "VOC",
    "Pressure_hPa",
    "Gas_Ohms"
]

new_file = not FILE.exists()

print(f"Live logging to: {FILE}")
print("Close Arduino Serial Monitor before running this.")
print("Press Ctrl+C to stop.")

ser = serial.Serial(PORT, BAUD, timeout=2)

with open(FILE, "a", newline="") as f:
    writer = csv.writer(f)

    if new_file:
        writer.writerow(headers)
        f.flush()

    try:
        while True:
            line = ser.readline().decode(errors="ignore").strip()

            if not line:
                continue

            if not line.startswith("DATA,TIME,"):
                print("Skipping:", line)
                continue

            parts = line.split(",")

            if len(parts) != 11:
                print("Bad line:", line)
                continue

            row = [
                datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                parts[2],
                parts[3],
                parts[4],
                parts[5],
                parts[6],
                parts[7],
                parts[8],
                parts[9],
                parts[10]
            ]

            writer.writerow(row)
            f.flush()

            print(row)

    except KeyboardInterrupt:
        print("\nStopped logging.")
        ser.close()