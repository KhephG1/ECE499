from flask import Flask, render_template, jsonify, request

from dashboard.nodes import get_nodes

from database.queries import (

    get_sensor_history,

    update_device_location

)



app = Flask(__name__)





# ===================================
# DASHBOARD PAGE
# ===================================


@app.route("/")
def home():


    nodes = get_nodes()


    return render_template(

        "index.html",

        nodes=nodes,

        node=nodes[0] if nodes else None

    )









# ===================================
# LIVE NODE DATA API
# ===================================


@app.route("/api/nodes")
def api_nodes():


    nodes = get_nodes()


    return jsonify(nodes)









# ===================================
# NODE LOCATION API
# ===================================


@app.route("/api/node-location", methods=["POST"])
def api_node_location():


    data = request.get_json(silent=True) or {}


    device_id = str(
        data.get("device_id", "")
    ).strip()



    if not device_id:

        return jsonify({

            "success": False,

            "error": "A node must be selected"

        }), 400




    # ===============================
    # VALIDATE COORDINATES
    # ===============================

    try:

        latitude = float(
            data.get("latitude")
        )

        longitude = float(
            data.get("longitude")
        )


    except (TypeError, ValueError):

        return jsonify({

            "success": False,

            "error": "Latitude and longitude must be numbers"

        }), 400



    if not -90 <= latitude <= 90:

        return jsonify({

            "success": False,

            "error": "Latitude must be between -90 and 90"

        }), 400



    if not -180 <= longitude <= 180:

        return jsonify({

            "success": False,

            "error": "Longitude must be between -180 and 180"

        }), 400




    # ===============================
    # SAVE TO DATABASE
    # ===============================

    updated = update_device_location(

        device_id,

        latitude,

        longitude

    )



    if not updated:

        return jsonify({

            "success": False,

            "error": f"Unknown node WF-{device_id}"

        }), 404



    return jsonify({

        "success": True,

        "device_id": device_id,

        "latitude": latitude,

        "longitude": longitude

    })









# ===================================
# HISTORICAL DATA API
# ===================================


@app.route("/api/history/<device_id>")
def api_history(device_id):


    history_range = request.args.get(

        "range",

        "24h"

    )


    rows = get_sensor_history(

        device_id,

        history_range

    )



    history = {


        "timestamps": [],

        "temperature": [],

        "humidity": [],

        "voc": [],

        "co2": [],

        "battery": []

    }





    for row in rows:


        history["timestamps"].append(

            row["timestamp"]

        )


        history["temperature"].append(

            row["temperature"]

        )


        history["humidity"].append(

            row["humidity"]

        )


        history["voc"].append(

            row["voc"]

        )


        history["co2"].append(

            row["co2"]

        )


        history["battery"].append(

            row["battery_percentage"]

        )





    return jsonify(history)









# ===================================
# START SERVER
# ===================================


if __name__ == "__main__":


    app.run(

        host="127.0.0.1",

        port=5000,

        debug=True

    )