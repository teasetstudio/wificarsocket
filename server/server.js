const express = require('express');
const http = require('http');
const WebSocket = require('ws');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

let controller = null;
let camera = null;
let latestFrame = null;

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
            console.log("Camera frame:", data.length, "bytes");
            latestFrame = data;

            return;
        }

        // Controller registration
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
        'Pragma': 'no-cache'
    });

    const interval = setInterval(() => {

        if(latestFrame){

            res.write(
                `--frame\r\n` +
                `Content-Type: image/jpeg\r\n` +
                `Content-Length: ${latestFrame.length}\r\n\r\n`
            );

            res.write(latestFrame);
            res.write('\r\n');
        }
    }, 50); // ~20 FPS

    req.on('close', () => {
        clearInterval(interval);
    });
});

// запуск
server.listen(3030, '0.0.0.0', () => {
    console.log("Server running:");
    console.log("http://192.168.88.5:3030");
});
