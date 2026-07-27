import sqlite3
from config import DATABASE



def create_database():

    conn = sqlite3.connect(DATABASE)

    cursor = conn.cursor()



    # ===============================
    # DEVICES TABLE
    # ===============================

    cursor.execute(
        """
        CREATE TABLE IF NOT EXISTS devices (

            device_id TEXT PRIMARY KEY,

            latitude REAL NOT NULL,

            longitude REAL NOT NULL

        )
        """
    )



    # ===============================
    # SENSOR DATA TABLE
    # ===============================

    cursor.execute(
        """
        CREATE TABLE IF NOT EXISTS sensor_data (

            id INTEGER PRIMARY KEY AUTOINCREMENT,


            device_id TEXT NOT NULL,


            timestamp TIMESTAMP NOT NULL,


            temperature REAL,

            humidity REAL,

            co2 REAL,

            voc REAL,

            pressure REAL,


            battery_voltage REAL,

            battery_percentage REAL,


            rssi INTEGER,

            snr REAL,


            fire_score REAL,

            fire_risk TEXT,


            FOREIGN KEY(device_id)

            REFERENCES devices(device_id)

        )
        """
    )



    conn.commit()

    conn.close()



    print("Database initialized successfully")





if __name__ == "__main__":

    create_database()