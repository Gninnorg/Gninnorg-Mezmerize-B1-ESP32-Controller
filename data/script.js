var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getValues(){
    websocket.send("getValues");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getValues();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function updateVolume(element) {
    var sliderValue = document.getElementById(element.id).value;
    document.getElementById(element.id).innerHTML = sliderValue;
    console.log("Sent: "+element.id+":"+sliderValue);
    websocket.send(element.id+":"+sliderValue);
}

function onMessage(event) {
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);
    var values = Object.values(myObj);
    console.log("Recv:" + event.data);
    console.log("keys: "+ keys)
    console.log("values: "+ values)
    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
        var value = values[i];
        document.getElementById(key).value = myObj[key];
        document.getElementById(key).innerHTML = String(value);
        // Display the volume setting in text also (uses its own id)
        if (key == "Volume") document.getElementById("VolumeValue").innerHTML = String(value);
    }
}