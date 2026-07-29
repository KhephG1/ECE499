import serial
import time


from config import SERIAL_PORT, BAUD_RATE


from database.queries import (
    add_device,
    add_sensor_data
)


from dashboard.risk import calculate_fire_risk





# ===================================
# DEVICE LOCATIONS
# ===================================

# Temporary hardcoded locations.
# Stored once in the database.

DEVICE_LOCATIONS = {


    "1":
    (
        48.461182,
        -123.310607
    ),


    "2":
    (
        48.461124,
        -123.310587
    ),


    "3":
    (
        48.460663,
        -123.309851
    )

}







# ===================================
# PACKET PARSER
# ===================================


def parse_packet(packet):

    """
    Converts serial string into
    database-ready dictionary.


    Incoming packet:

    SCD CO2,
    SCD Temp,
    SCD Humidity,
    BME CO2eq,
    BME VOC,
    BME Humidity,
    BME Temp,
    BME Pressure,
    BME Status,
    Battery Voltage,
    RSSI,
    SNR,
    Device ID

    """



    values = packet.split(",")



    if len(values) != 13:

        print(
            "Invalid packet:",
            packet
        )

        return None





    try:


        data = {


            # Device

            "device_id":
                values[12],



            # Environmental data
            # Using SCD values where available

            "temperature":
                float(values[6]),


            "humidity":
                float(values[5]),


            # SCD40 measures CO2 directly.
            # The BME680 only estimates an
            # equivalent from VOC.

            "co2":
                float(values[0]),


            "voc":
                float(values[4]),


            "pressure":
                float(values[7]),





            # Power

            "battery_voltage":
                float(values[9]),



            "battery_percentage":
                battery_percentage(
                    float(values[9])
                ),





            # Radio

            "rssi":
                int(float(values[10])),


            "snr":
                float(values[11])

        }


        return data



    except ValueError:


        print(
            "Failed to parse packet:",
            packet
        )


        return None







# ===================================
# BATTERY CONVERSION
# ===================================


def battery_percentage(voltage):

    """
    Approximate LiPo percentage.

    Assumes:
        4.2V = 100%
        3.3V = 0%

    """



    percentage = (

        (voltage - 3.3)

        /

        (4.2 - 3.3)

    ) * 100



    return max(
        0,
        min(
            100,
            round(percentage, 1)
        )
    )








# ===================================
# STORE DEVICE
# ===================================


def register_device(device_id):


    if device_id not in DEVICE_LOCATIONS:

        print(
            "Unknown device location:",
            device_id
        )

        return



    latitude, longitude = DEVICE_LOCATIONS[device_id]



    add_device(

        device_id,

        latitude,

        longitude

    )









# ===================================
# MAIN LOGGER LOOP
# ===================================


def start_logger():


    print(
        "Opening serial port..."
    )



    ser = serial.Serial(

        SERIAL_PORT,

        BAUD_RATE,

        timeout=1

    )



    print(
        "Serial logger running"
    )





    while True:


        try:


            line = (
                ser.readline()
                .decode()
                .strip()
            )



            if not line:

                continue




            print(
                "Received:",
                line
            )




            data = parse_packet(line)



            if data is None:

                continue






            device_id = data["device_id"]



            register_device(
                device_id
            )






            # ---------------------------
            # Calculate fire risk
            # ---------------------------


            risk = calculate_fire_risk(data)



            data["fire_score"] = (
                risk["score"]
            )


            data["fire_risk"] = (
                risk["risk"]
            )







            # ---------------------------
            # Store database entry
            # ---------------------------


            add_sensor_data(data)



            print(

                f"Stored {device_id} | "

                f"Risk: {data['fire_risk']}"

            )





        except Exception as error:


            print(
                "Logger error:",
                error
            )


            time.sleep(1)









# ===================================
# PROGRAM START
# ===================================


if __name__ == "__main__":


    start_logger()