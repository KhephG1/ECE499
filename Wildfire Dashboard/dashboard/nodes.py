from database.queries import get_latest_nodes
from datetime import datetime

from config import OFFLINE_THRESHOLD

# ===================================
# FORMAT DATABASE DATA FOR DASHBOARD
# ===================================

def seconds_since(timestamp):

    """
    Age of a reading, in seconds.
    """


    update_time = datetime.strptime(
        timestamp,
        "%Y-%m-%d %H:%M:%S"
    )


    return (
        datetime.now() - update_time
    ).total_seconds()




def time_since(seconds):

    if seconds < 60:

        return f"{int(seconds)} seconds ago"


    elif seconds < 3600:

        minutes = int(seconds / 60)

        return f"{minutes} minutes ago"


    else:

        hours = int(seconds / 3600)

        return f"{hours} hours ago"

def get_nodes():

    """
    Reads latest sensor data from database
    and formats it for the Flask dashboard.
    """



    rows = get_latest_nodes()



    nodes = []



    for row in rows:


        # ===============================
        # ONLINE / OFFLINE STATUS
        # ===============================

        age = seconds_since(
            row["timestamp"]
        )


        online = age <= OFFLINE_THRESHOLD



        node = {


            "device_id":
                row["device_id"],



            "latitude":
                row["latitude"],


            "longitude":
                row["longitude"],





            # Risk

            "risk":
                row["fire_risk"],



            "fire_score":
                row["fire_score"],






            # Power

            "battery":
                row["battery_percentage"],







            # Environmental

            "temperature":
                row["temperature"],


            "humidity":
                row["humidity"],


            "voc":
                row["voc"],


            "co2":
                row["co2"],







            # Communication

            "rssi":
                row["rssi"],


            "snr":
                row["snr"],







            # Time

            "last_update":
                time_since(age),


            "seconds_since_update":
                int(age),



            # Status

            "online":
                online


        }



        nodes.append(node)

    nodes.sort(
        key=lambda x: int(x["device_id"])
    )

    return nodes