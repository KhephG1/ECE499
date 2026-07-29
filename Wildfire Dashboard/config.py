import sys
# ===================================
# PROJECT CONFIGURATION
# ===================================


# -----------------------------
# DATABASE
# -----------------------------


DATABASE = "database/wildfire.db"




# -----------------------------
# SERIAL COMMUNICATION
# -----------------------------


SERIAL_PORT = sys.argv[1]

BAUD_RATE = 115200




# -----------------------------
# DASHBOARD
# -----------------------------


HOST = "0.0.0.0"

PORT = 5000




# -----------------------------
# NODE STATUS
# -----------------------------


# Seconds without a reading before a
# node is considered offline

OFFLINE_THRESHOLD = 30