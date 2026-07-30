# ===================================
# WILDFIRE RISK CALCULATION
# ===================================


def calculate_fire_risk(data):

    """
    Calculates wildfire risk based on
    environmental sensor readings.

    Expected input:

    {
        "temperature": float,
        "humidity": float,
        "voc": float,
        "co2": float
    }


    Returns:

    {
        "score": int,
        "risk": str
    }

    """

    score = 0

    # ===================================
    # TEMPERATURE CONTRIBUTION
    # ===================================


    temperature = data.get(
        "temperature",
        0
    )


    if temperature >= 45:

        score += 35


    elif temperature >= 35:

        score += 20


    elif temperature >= 30:

        score += 15


    # ===================================
    # HUMIDITY CONTRIBUTION
    # ===================================


    humidity = data.get(
        "humidity",
        100
    )


    if humidity <= 20:

        score += 30


    elif humidity <= 35:

        score += 15

    elif humidity >= 50:
        score -= 30

    # ===================================
    # VOC CONTRIBUTION
    # ===================================


    voc = data.get(
        "voc",
        0
    )


    if voc >= 1.0:

        score += 10


    elif voc >= 0.85:

        score += 5

    # ===================================
    # CO2 CONTRIBUTION
    # ===================================


    co2 = data.get(
        "co2",
        400
    )


    if co2 >= 1200:
        score += 25
    elif co2 >= 1100:
        score += 20
    elif co2 >= 1000:
        score += 15
    elif co2 >= 800:
        score += 10


    # ===================================
    # DETERMINE RISK LEVEL
    # ===================================

    if score < 30:

        risk = "Normal"


    elif score < 60:

        risk = "Warning"


    else:

        risk = "Danger"


    return {


        "score": score,


        "risk": risk


    }