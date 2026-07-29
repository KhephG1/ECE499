// ===================================
// GLOBAL VARIABLES
// ===================================


let nodes = [];

let selectedNode = null;

let markers = {};


// Historical charts

let temperatureChart = null;

let humidityChart = null;

let vocChart = null;

let co2Chart = null;

let pressureChart = null;

let batteryChart = null;


// Parameter graph on the dashboard page

let parameterChart = null;


// What the parameter graph is currently
// showing, so the 1 second poll only
// rebuilds it when something changed.
// Left undefined so the first poll always
// draws, even with no node selected

let parameterChartNode;

let parameterChartTimestamp;





// ===================================
// MAP SETUP
// ===================================


let map = L.map("map").setView(

    [48.46231, -123.31245],

    15

);



L.tileLayer(

    "https://tile.openstreetmap.org/{z}/{x}/{y}.png",

    {

        attribution: "OpenStreetMap"

    }

).addTo(map);







// ===================================
// RISK HELPERS
// ===================================


// A node that has stopped reporting is
// shown as Offline instead of its last
// known risk level

function nodeStatus(node) {


    if(!node.online) {

        return "Offline";

    }


    return node.risk;

}





function riskColor(risk) {


    if (risk === "Offline") {

        return "#9ca3af";

    }


    if (risk === "Normal") {

        return "green";

    }


    if (risk === "Warning") {

        return "orange";

    }


    if (risk === "Danger") {

        return "red";

    }


    return "gray";

}





function riskClass(risk) {


    if (risk === "Offline") {

        return "risk-offline";

    }


    if (risk === "Normal") {

        return "risk-normal";

    }


    if (risk === "Warning") {

        return "risk-warning";

    }


    if (risk === "Danger") {

        return "risk-danger";

    }


    return "";

}









// ===================================
// TIME HELPERS
// ===================================


// Database timestamps are local clock
// time, "YYYY-MM-DD HH:MM:SS"

function formatTimestamp(timestamp) {


    if(!timestamp) {

        return "--";

    }



    let parsed =
        new Date(
            timestamp.replace(" ", "T")
        );



    if(isNaN(parsed)) {

        return timestamp;

    }



    return parsed.toLocaleTimeString();

}





// Timestamp of the most recent reading
// received from any node

function latestReadingTime() {


    let latest = null;



    nodes.forEach(node => {


        if(!node.timestamp) {

            return;

        }


        if(latest === null || node.timestamp > latest) {

            latest = node.timestamp;

        }


    });



    return latest;

}









// ===================================
// UPDATE SELECTED NODE PANEL
// ===================================


function updatePanel(node) {


    if(!node) {

        return;

    }


    selectedNode = node;



    document.getElementById("device-id").innerHTML =
        "WF-" + node.device_id;


    document.getElementById("latitude").innerHTML =
        node.latitude + "°";


    document.getElementById("longitude").innerHTML =
        node.longitude + "°";


    document.getElementById("battery").innerHTML =
        node.battery + " %";


    document.getElementById("last-update").innerHTML =
        node.last_update;


    document.getElementById("temperature").innerHTML =
        node.temperature + " °C";


    document.getElementById("humidity").innerHTML =
        node.humidity + " %";


    document.getElementById("voc").innerHTML =
        node.voc + " ppb";


    document.getElementById("co2").innerHTML =
        node.co2 + " ppm";


    document.getElementById("rssi").innerHTML =
        node.rssi + " dBm";


    document.getElementById("snr").innerHTML =
        node.snr + " dB";



    let status = nodeStatus(node);



    let badge =
        document.getElementById("risk-badge");



    badge.innerHTML =
        status;



    badge.className =
        "risk-badge " + riskClass(status);




    // The number behind the badge. Kept
    // visible when the node is offline,
    // like the other readings, but dimmed
    // so a stale score is not mistaken
    // for a current one.

    let score =
        document.getElementById("fire-score");


    // Stored as REAL, but only ever built
    // from whole numbers, so drop the ".0"

    score.innerHTML =
        "Fire Score <strong>" +
        (
            node.fire_score === null
            ? "--"
            : Math.round(node.fire_score)
        ) +
        "</strong>";


    score.className =
        node.online
        ? "fire-score"
        : "fire-score stale";


}









// ===================================
// NODE OVERVIEW
// ===================================


function createNodeList() {


    let list =
        document.getElementById("node-list");


    list.innerHTML = "";



    let sortedNodes = [...nodes].sort(

        (a,b) =>

        Number(a.device_id) -

        Number(b.device_id)

    );





    sortedNodes.forEach(node => {



        let item =
            document.createElement("div");



        item.className =
            "node-item";





        let status = nodeStatus(node);



        let icon = "⚪";



        if(status === "Offline") {

            icon = "⚫";

        }


        else if(status === "Normal") {

            icon = "🟢";

        }


        else if(status === "Warning") {

            icon = "🟡";

        }


        else if(status === "Danger") {

            icon = "🔴";

        }





        item.innerHTML = `


            <div class="node-title">

                ${icon}

                <b>
                    WF-${node.device_id}
                </b>

            </div>


            Risk: ${status}

            <br>


            Battery:
            ${node.battery}%


        `;





        item.classList.add(

            "node-" + status.toLowerCase()

        );





        item.onclick = function() {


            let currentNode = nodes.find(

                n =>

                n.device_id === node.device_id

            );



            if(currentNode) {


                updatePanel(currentNode);


                map.setView(

                    [

                        currentNode.latitude,

                        currentNode.longitude

                    ],

                    17

                );


            }


        };





        list.appendChild(item);


    });


}









// ===================================
// STATUS BAR
// ===================================


function updateStatusBar() {


    let warnings = 0;

    let dangers = 0;

    let online = 0;

    let offline = 0;



    nodes.forEach(node => {


        if(node.risk === "Warning") {

            warnings++;

        }



        if(node.risk === "Danger") {

            dangers++;

        }



        if(node.online) {

            online++;

        }


        else {

            offline++;

        }


    });





    document.getElementById("online-count").innerHTML =
        online;


    document.getElementById("offline-count").innerHTML =
        offline;


    document.getElementById("warning-count").innerHTML =
        warnings;


    document.getElementById("danger-count").innerHTML =
        dangers;


    document.getElementById("system-update").innerHTML =
        formatTimestamp(
            latestReadingTime()
        );


}









// ===================================
// MAP MARKERS
// ===================================


function updateMarkers() {


    nodes.forEach(node => {



        let marker =
            markers[node.device_id];





        if(!marker) {



            marker =
                L.circleMarker(

                    [

                        node.latitude,

                        node.longitude

                    ],

                    {

                        radius: 10,

                        color: "black",

                        weight: 1,

                        fillOpacity: 0.85

                    }

                ).addTo(map);





            marker.on(

                "click",

                function() {


                    let currentNode = nodes.find(

                        n =>

                        n.device_id === node.device_id

                    );



                    if(currentNode) {

                        updatePanel(currentNode);

                    }



                }

            );





            markers[node.device_id] =
                marker;


        }





        marker.setLatLng([

            node.latitude,

            node.longitude

        ]);




        let status = nodeStatus(node);




        // Offline nodes are greyed out

        marker.setStyle({

            fillColor:

                riskColor(status),


            color:

                status === "Offline"

                ? "#6b7280"

                : "black",


            fillOpacity:

                status === "Offline"

                ? 0.45

                : 0.85

        });





        marker.bindPopup(

            `

            <b>
            WF-${node.device_id}
            </b>

            <br>

            Risk:
            ${status}

            <br>

            Last update:
            ${node.last_update}

            `

        );



    });


}









// ===================================
// LOAD DATA
// ===================================


async function loadNodes() {


    try {


        let response =
            await fetch("/api/nodes");



        nodes =
            await response.json();


        updateMarkers();


        createNodeList();


        updateStatusBar();


        updateNodeSelect();





        if(nodes.length > 0) {



            if(selectedNode) {


                let updated =
                    nodes.find(

                        n =>

                        n.device_id === selectedNode.device_id

                    );



                if(updated) {

                    updatePanel(updated);

                }


            }


            else {

                updatePanel(nodes[0]);

            }


        }



        refreshParameterChart();


    }


    catch(error) {


        console.error(error);


        setStatus(

            "Failed to update node data",

            "error"

        );


    }


}









// ===================================
// NODE LOCATION FORM
// ===================================


function setStatus(message, type) {


    let status =
        document.getElementById("location-status");



    status.innerHTML =

        new Date().toLocaleTimeString()

        +

        "  "

        +

        message;



    status.className =
        "form-status " + type;


}





function updateNodeSelect() {


    let select =
        document.getElementById("location-node");



    let ids = nodes.map(
        n => n.device_id
    );



    // Only rebuild when the node list changes,
    // so polling does not clobber the selection

    if(select.dataset.ids === ids.join(",")) {

        return;

    }



    let previous = select.value;


    select.innerHTML =
        "<option value=''>Select a node</option>";



    nodes.forEach(node => {


        let option =
            document.createElement("option");


        option.value =
            node.device_id;


        option.innerHTML =
            "WF-" + node.device_id;


        select.appendChild(option);


    });



    select.value = previous;


    select.dataset.ids = ids.join(",");


}





async function submitNodeLocation(event) {


    event.preventDefault();



    let deviceId =
        document.getElementById("location-node").value;


    let latitude =
        document.getElementById("location-latitude").value;


    let longitude =
        document.getElementById("location-longitude").value;




    try {


        let response = await fetch(

            "/api/node-location",

            {

                method: "POST",

                headers: {

                    "Content-Type": "application/json"

                },

                body: JSON.stringify({

                    device_id: deviceId,

                    latitude: latitude,

                    longitude: longitude

                })

            }

        );



        let result = await response.json();



        if(!result.success) {


            setStatus(

                result.error,

                "error"

            );


            return;


        }



        setStatus(

            "WF-" +

            result.device_id +

            " moved to " +

            result.latitude +

            ", " +

            result.longitude,

            "success"

        );



        // Show the new position without waiting
        // for a page refresh

        await loadNodes();


    }


    catch(error) {


        console.error(error);


        setStatus(

            "Failed to save node location",

            "error"

        );


    }


}









// ===================================
// PARAMETER GRAPH
// ===================================


// Logged parameters, keyed by the field
// name used by /api/history

const PARAMETER_LABELS = {

    temperature: "Temperature (°C)",

    humidity: "Humidity (%)",

    voc: "VOC (ppb)",

    co2: "CO₂ (ppm)",

    pressure: "Pressure (hPa)",

    battery: "Battery (%)"

};





function setParameterHint(message) {


    document.getElementById(
        "parameter-hint"
    ).innerHTML = message;


}





// Rebuilds the graph from scratch. Called
// when the user picks a parameter, and by
// refreshParameterChart() once the data
// behind it has moved on.

async function updateParameterChart() {


    if(parameterChart) {

        parameterChart.destroy();

        parameterChart = null;

    }




    if(!selectedNode) {


        parameterChartNode = null;

        parameterChartTimestamp = null;


        setParameterHint(
            "Select a node to see its data."
        );


        return;

    }




    let parameter =
        document.getElementById(
            "parameter-select"
        ).value;



    // Remember what is on screen before the
    // request, so a slow response cannot be
    // mistaken for a stale graph

    parameterChartNode =
        selectedNode.device_id;

    parameterChartTimestamp =
        selectedNode.timestamp;




    try {


        let response = await fetch(

            "/api/history/" +

            encodeURIComponent(selectedNode.device_id) +

            "?range=24h"

        );


        let data = await response.json();




        if(data.timestamps.length === 0) {


            setParameterHint(
                "No readings in the last 24 hours."
            );


            return;

        }




        parameterChart = createChart(

            "parameter-chart",

            PARAMETER_LABELS[parameter],

            {

                time: data.timestamps,

                data: data[parameter]

            }

        );



        setParameterHint(
            "WF-" +
            selectedNode.device_id +
            ", last 24 hours"
        );


    }


    catch(error) {


        console.error(error);


        // Let the next poll try again

        parameterChartTimestamp = null;


        setParameterHint(
            "Failed to load graph data."
        );


    }


}





// Called on every poll. Redraws only when
// the selected node changed or that node
// reported a new reading.

function refreshParameterChart() {


    let deviceId =
        selectedNode ? selectedNode.device_id : null;


    let timestamp =
        selectedNode ? selectedNode.timestamp : null;



    if(

        deviceId === parameterChartNode

        &&

        timestamp === parameterChartTimestamp

    ) {

        return;

    }



    updateParameterChart();


}









// ===================================
// HISTORICAL DATA
// ===================================


async function showMoreData() {


    document.getElementById(
        "dashboard-page"
    ).style.display = "none";


    document.getElementById(
        "more-data-page"
    ).style.display = "block";


    if(selectedNode) {


        document.getElementById(
            "history-title"
        ).innerHTML =


        "WF-" +

        selectedNode.device_id +

        " Historical Data";



        await createHistoricalCharts();

    }


}






function showDashboard() {


    document.getElementById(
        "dashboard-page"
    ).style.display = "block";



    document.getElementById(
        "more-data-page"
    ).style.display = "none";


}









// ===================================
// HISTORICAL CHART CREATION
// ===================================


async function createHistoricalCharts() {


    const historyRange = document.getElementById(
        "history-range"
    ).value;

    let response = await fetch(

        "/api/history/" +

        selectedNode.device_id +

        "?range=" +

        historyRange

    );


    let data = await response.json();



    destroyCharts();




    temperatureChart = createChart(

        "temperature-chart",

        "Temperature (°C)",

        {

            time:data.timestamps,

            data:data.temperature

        }

    );



    humidityChart = createChart(

        "humidity-chart",

        "Humidity (%)",

        {

            time:data.timestamps,

            data:data.humidity

        }

    );



    vocChart = createChart(

        "voc-chart",

        "VOC (ppb)",

        {

            time:data.timestamps,

            data:data.voc

        }

    );



    co2Chart = createChart(

        "co2-chart",

        "CO₂ (ppm)",

        {

            time:data.timestamps,

            data:data.co2

        }

    );



    pressureChart = createChart(

        "pressure-chart",

        "Pressure (hPa)",

        {

            time:data.timestamps,

            data:data.pressure

        }

    );



    batteryChart = createChart(

        "battery-chart",

        "Battery (%)",

        {

            time:data.timestamps,

            data:data.battery

        }

    );


}








// ===================================
// HISTORICAL DATA EXPORT
// ===================================


function exportHistory() {


    if(!selectedNode) {

        return;

    }



    const historyRange = document.getElementById(
        "history-range"
    ).value;



    // Letting the browser follow the link keeps
    // the file out of memory and gives the user
    // the normal download prompt

    window.location.href =

        "/api/history/" +

        encodeURIComponent(selectedNode.device_id) +

        "/export?range=" +

        encodeURIComponent(historyRange);


}









function createChart(id, label, values) {


    return new Chart(

        document.getElementById(id),

        {

            type: "line",

            data: {

                labels: values.time,

                datasets: [

                    {

                        label: label,

                        data: values.data,

                        tension: 0.3

                    }

                ]

            },

            options: {

                responsive: true,

                maintainAspectRatio: false

            }

        }

    );


}









function destroyCharts() {


    if(temperatureChart)

        temperatureChart.destroy();


    if(humidityChart)

        humidityChart.destroy();


    if(vocChart)

        vocChart.destroy();


    if(co2Chart)

        co2Chart.destroy();


    if(pressureChart)

        pressureChart.destroy();


    if(batteryChart)

        batteryChart.destroy();


}



// ===================================
// STARTUP
// ===================================


loadNodes();



setInterval(

    loadNodes,

    1000

);