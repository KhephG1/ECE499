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

let batteryChart = null;





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


function riskColor(risk) {


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



    let badge =
        document.getElementById("risk-badge");



    badge.innerHTML =
        node.risk;



    badge.className =
        "risk-badge " + riskClass(node.risk);


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





        let icon = "⚪";



        if(node.risk === "Normal") {

            icon = "🟢";

        }


        else if(node.risk === "Warning") {

            icon = "🟡";

        }


        else if(node.risk === "Danger") {

            icon = "🔴";

        }





        item.innerHTML = `


            <div class="node-title">

                ${icon}

                <b>
                    WF-${node.device_id}
                </b>

            </div>


            Risk: ${node.risk}

            <br>


            Battery:
            ${node.battery}%


        `;





        item.classList.add(

            "node-" + node.risk.toLowerCase()

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



    nodes.forEach(node => {


        if(node.risk === "Warning") {

            warnings++;

        }



        if(node.risk === "Danger") {

            dangers++;

        }


    });





    document.getElementById("online-count").innerHTML =
        nodes.length;


    document.getElementById("warning-count").innerHTML =
        warnings;


    document.getElementById("danger-count").innerHTML =
        dangers;


    document.getElementById("system-update").innerHTML =
        new Date().toLocaleTimeString();


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





        marker.setStyle({

            fillColor:

                riskColor(node.risk)

        });





        marker.bindPopup(

            `

            <b>
            WF-${node.device_id}
            </b>

            <br>

            Risk:
            ${node.risk}

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


    }


    catch(error) {


        console.error(error);


        addMessage(

            "Failed to update node data"

        );


    }


}









// ===================================
// MESSAGE LOG
// ===================================


function addMessage(message) {


    let log =
        document.getElementById("message-log");



    let entry =
        document.createElement("p");



    entry.innerHTML =

        new Date().toLocaleTimeString()

        +

        "  "

        +

        message;




    log.appendChild(entry);



    log.scrollTop =

        log.scrollHeight;


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



    batteryChart = createChart(

        "battery-chart",

        "Battery (%)",

        {

            time:data.timestamps,

            data:data.battery

        }

    );


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