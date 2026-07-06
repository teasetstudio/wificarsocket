const express = require('express');
const http = require('http');
const WebSocket = require('ws');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

let esp32 = null;

// отдаём HTML
app.use(express.static('public'));

// ESP32 подключается по WebSocket
wss.on('connection', (ws, req) => {

    console.log("New WS connection");

    // считаем что ESP32 — первый клиент
    esp32 = ws;

    ws.on('message', (msg) => {
        console.log("From ESP32:", msg.toString());
    });

    ws.on('close', () => {
        console.log("ESP32 disconnected");
        esp32 = null;
    });
});

// HTTP API от браузера
app.get('/cmd/:dir', (req, res) => {

    const cmd = req.params.dir;

    console.log("CMD from browser:", cmd);

    if (esp32 && esp32.readyState === 1) {
        esp32.send(cmd);
    }

    res.send("OK");
});

// запуск
server.listen(3000, '0.0.0.0', () => {
    console.log("Server running:");
    console.log("http://192.168.88.5:3030");
});
