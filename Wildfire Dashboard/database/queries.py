import sqlite3
from datetime import datetime

from config import DATABASE





# ===================================
# DATABASE CONNECTION
# ===================================


def get_connection():

    conn = sqlite3.connect(
        DATABASE
    )

    conn.row_factory = sqlite3.Row

    return conn





# ===================================
# DEVICE MANAGEMENT
# ===================================


def add_device(
    device_id,
    latitude,
    longitude
):

    conn = get_connection()

    cursor = conn.cursor()



    cursor.execute(

        """

        INSERT OR IGNORE INTO devices

        (
            device_id,
            latitude,
            longitude
        )

        VALUES (?, ?, ?)

        """,

        (
            str(device_id),
            latitude,
            longitude
        )

    )


    conn.commit()

    conn.close()





def update_device_location(
    device_id,
    latitude,
    longitude
):

    conn = get_connection()

    cursor = conn.cursor()



    cursor.execute(

        """

        UPDATE devices

        SET
            latitude = ?,

            longitude = ?


        WHERE device_id = ?

        """,

        (
            latitude,
            longitude,
            str(device_id)
        )

    )


    updated = cursor.rowcount


    conn.commit()

    conn.close()


    return updated > 0





def get_devices():

    conn = get_connection()

    cursor = conn.cursor()



    cursor.execute(

        """
        SELECT *
        FROM devices
        """

    )


    rows = cursor.fetchall()


    conn.close()


    return rows





# ===================================
# SENSOR DATA INSERT
# ===================================


def add_sensor_data(data):

    conn = get_connection()

    cursor = conn.cursor()


    timestamp = datetime.now().strftime(
        "%Y-%m-%d %H:%M:%S"
    )


    cursor.execute(
        """
        INSERT INTO sensor_data
        (
            device_id,
            timestamp,

            temperature,
            humidity,
            co2,
            voc,
            pressure,

            battery_voltage,
            battery_percentage,

            rssi,
            snr,

            fire_score,
            fire_risk
        )

        VALUES
        (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)

        """,

        (

            str(data["device_id"]),

            timestamp,


            data["temperature"],

            data["humidity"],

            data["co2"],

            data["voc"],

            data["pressure"],


            data["battery_voltage"],

            data["battery_percentage"],


            data["rssi"],

            data["snr"],


            data["fire_score"],

            data["fire_risk"]

        )

    )


    conn.commit()

    conn.close()





# ===================================
# GET LATEST DATA FOR EACH NODE
# ===================================


def get_latest_nodes():

    conn = get_connection()

    cursor = conn.cursor()


    cursor.execute(

        """
        SELECT

            s.*,

            d.latitude,

            d.longitude


        FROM sensor_data s


        INNER JOIN devices d

        ON s.device_id = d.device_id



        INNER JOIN

        (

            SELECT

                device_id,

                MAX(timestamp) AS latest


            FROM sensor_data


            GROUP BY device_id

        ) latest


        ON

        s.device_id = latest.device_id

        AND

        s.timestamp = latest.latest


        ORDER BY

            CAST(s.device_id AS INTEGER)

        """

    )


    rows = cursor.fetchall()


    conn.close()


    return rows





# ===================================
# HISTORICAL DATA
# ===================================


def get_sensor_history(
    device_id,
    history_range
):


    conn = get_connection()

    cursor = conn.cursor()



    # Time range options

    ranges = {

        "1h": "-1 hour",

        "6h": "-6 hours",

        "24h": "-24 hours",

        "7d": "-7 days"

    }




    # ===================================
    # ALL DATA
    # ===================================

    if history_range == "all":


        cursor.execute(

            """

            SELECT *

            FROM sensor_data

            WHERE device_id = ?

            ORDER BY timestamp ASC

            """,

            (

                str(device_id),

            )

        )



    # ===================================
    # FILTERED DATA
    # ===================================

    else:


        cursor.execute(

            """

            SELECT *

            FROM sensor_data

            WHERE device_id = ?

            AND timestamp >= datetime('now', 'localtime', ?)

            ORDER BY timestamp ASC

            """,

            (

                str(device_id),

                ranges.get(

                    history_range,

                    "-24 hours"

                )

            )

        )



    rows = cursor.fetchall()


    conn.close()


    return rows
