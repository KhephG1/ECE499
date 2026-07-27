from flask import Flask, render_template, jsonify, request

from dashboard.nodes import get_nodes

from database.queries import get_sensor_history



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