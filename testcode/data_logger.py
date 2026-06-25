import serial
import time
import csv
import os

# Set your specific COM port (Windows) or device path (Mac/Linux)
SERIAL_PORT = "COM19"
BAUD_RATE = 115200
CSV_FILE = "ENS_160_AHT21.csv"

HEADERS = [
    "Timestamp",
    "AQI",
    "TVOC",
    "eCO2",
    "R HP0",
    "R HP1",
    "R HP2",
    "R HP3",
    "T_C",
    "RH_",
]


def parse_sensor_line(line):
    """
    Parses a line like:
    'AQI: 255, TVOC: 65535ppb, eCO2: 65535ppm, R HP0: 4293514240Ohm, ..., T_C=29.93, RH_=23.21'
    Returns a dict of {label: value}
    """
    data = {}
    parts = line.split(",")
    for part in parts:
        part = part.strip()
        # Handle "Key: ValueUnit" format (e.g. "AQI: 255", "TVOC: 65535ppb")
        if ": " in part:
            key, value = part.split(": ", 1)
            number = "".join(c for c in value if c.isdigit() or c == ".")
            data[key.strip()] = number
        # Handle "Key=Value" format (e.g. "T_C=29.93", "RH_=23.21")
        elif "=" in part:
            key, value = part.split("=", 1)
            number = "".join(c for c in value if c.isdigit() or c == ".")
            data[key.strip()] = number
    return data


# Only write the header if the file doesn't exist yet (avoids duplicates on re-run)
write_header = not os.path.exists(CSV_FILE)

# Open the serial connection with error handling
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
except serial.SerialException as e:
    print(f"Error opening serial port {SERIAL_PORT}: {e}")
    exit(1)

print(f"Logging data from {SERIAL_PORT} to {CSV_FILE}. Press Ctrl+C to stop.")

# Keep the file open for the duration of logging (efficient)
try:
    with open(CSV_FILE, mode="a", newline="") as file:
        writer = csv.writer(file)

        if write_header:
            writer.writerow(HEADERS)

        while True:
            if ser.in_waiting:
                # Read line from serial and decode, ignoring malformed bytes
                line = ser.readline().decode("utf-8", errors="replace").strip()

                if not line:  # Skip empty lines
                    continue

                timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
                parsed = parse_sensor_line(line)

                # Write columns in consistent order, empty string if key missing
                row = [timestamp] + [parsed.get(h, "") for h in HEADERS[1:]]
                writer.writerow(row)
                file.flush()  # Ensure data is written to disk immediately

                print(f"{timestamp} | {parsed}")

except KeyboardInterrupt:
    print("\nLogging stopped.")
finally:
    if ser.is_open:
        ser.close()
        print("Serial port closed.")
