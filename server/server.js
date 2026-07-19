const express = require('express');
const http = require('http');
const WebSocket = require('ws');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

let controller = null;
let camera = null;
let latestFrame = null;
let streamClients = [];

// отдаём HTML
app.use(express.static('public'));

// ESP32 подключается по WebSocket
wss.on('connection', (ws, req) => {
    console.log("New WS connection");

    ws.on('message', (data, isBinary) => {

        // Camera registration
        if (!isBinary && data.toString() === "CAMERA") {
            camera = ws;
            console.log("Camera registered");
            return;
        }

        // Binary data = camera frame
        if (isBinary) {
            latestFrame = data;
            broadcastFrame(data);

            return;
        }

        // Controller registration
        console.log("WS message:", data.toString());
        if (data.toString() === "CONTROLLER") {
            controller = ws;
            console.log("Controller registered");
            return;
        }

        // Controller commands
        if (controller === ws) {

            console.log("Controller command:", data.toString());

            // forward command if needed
        }
    });


    ws.on('close', () => {
        console.log("WS disconnected");

        if (controller === ws)
            controller = null;

        if (camera === ws)
            camera = null;
    });
});

// HTTP API от браузера
app.get('/cmd/:dir', (req, res) => {
    const cmd = req.params.dir;
    console.log("CMD from browser:", cmd);
    console.log("Controller state:", controller ? controller.readyState : "null");

    if (controller && controller.readyState === 1) {
        controller.send(cmd);
    }

    res.send("OK");
});

app.get('/stream', (req, res) => {
    res.writeHead(200, {
        'Content-Type': 'multipart/x-mixed-replace; boundary=frame',
        'Cache-Control': 'no-cache',
        'Connection': 'keep-alive',
        'Pragma': 'no-cache',
        'X-Accel-Buffering': 'no'
    });

    res.socket.setNoDelay(true);

    streamClients.push(res);

    req.on('close', () => {
        streamClients = streamClients.filter(c => c !== res);
    });
});

function broadcastFrame(frame){
    for (const res of streamClients){

        res.write(
            `--frame\r\n` +
            `Content-Type: image/jpeg\r\n` +
            `Content-Length: ${frame.length}\r\n\r\n`
        );

        res.write(frame);
        res.write('\r\n');
    }
}

// запуск
server.listen(3030, '0.0.0.0', () => {
    console.log("Server running:");
    console.log("http://192.168.88.5:3030");
});
