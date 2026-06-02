import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import argparse

CSV_FILE = "ENS_160_AHT21.csv"

# --- Argument parsing ---
parser = argparse.ArgumentParser(
    description="Plot sensor data from ENS160/AHT21 CSV log."
)
parser.add_argument(
    "columns",
    nargs="*",  # zero or more arguments
    help="List of columns to plot (e.g. AQI TVOC T_C). If omitted, all columns are plotted.",
)
args = parser.parse_args()

# Load and parse the CSV
df = pd.read_csv(CSV_FILE)
df["Timestamp"] = pd.to_datetime(df["Timestamp"])

# All sensor columns (everything except Timestamp)
available_columns = [col for col in df.columns if col != "Timestamp"]

# Use provided columns or fall back to all
if args.columns:
    # Validate each requested column
    invalid = [col for col in args.columns if col not in available_columns]
    if invalid:
        print(f"Error: Unknown column(s): {invalid}")
        print(f"Available columns: {available_columns}")
        exit(1)
    sensor_columns = args.columns
else:
    sensor_columns = available_columns

# Create one subplot per sensor
fig, axes = plt.subplots(
    len(sensor_columns), 1, figsize=(12, 3 * len(sensor_columns)), sharex=True
)

# If there's only one sensor column, wrap it in a list for consistent indexing
if len(sensor_columns) == 1:
    axes = [axes]

for ax, col in zip(axes, sensor_columns):
    ax.plot(df["Timestamp"], pd.to_numeric(df[col], errors="coerce"), linewidth=1.2)
    ax.set_ylabel(col, fontsize=9)
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.tick_params(axis="y", labelsize=8)

# Format the shared x-axis timestamps
axes[-1].xaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))
axes[-1].xaxis.set_major_locator(mdates.AutoDateLocator())
plt.setp(axes[-1].xaxis.get_majorticklabels(), rotation=45, ha="right", fontsize=8)

fig.suptitle(
    "ENS160 / AHT21 Sensor Data Over Time", fontsize=13, fontweight="bold", y=0.98
)
plt.xlabel("Time", fontsize=10)
plt.tight_layout()
plt.savefig("sensor_plot.png", dpi=150, bbox_inches="tight")
plt.show()
