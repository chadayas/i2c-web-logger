const canvas = document.getElementById("graph");
const ctx = canvas.getContext("2d");
const readingEl = document.getElementById("reading");
const rawEl = document.getElementById("raw");
const mvEl = document.getElementById("mv");
const peakEl = document.getElementById("peak");
const statusEl = document.getElementById("status");

const maxPoints = 60;
const data = [];
var peakBac = 0;
var paused = false;

function togglePause() {
    paused = !paused;
    document.getElementById("pauseBtn").textContent = paused ? "Resume" : "Pause";
}

function tarePeak() {
    peakBac = 0;
    peakEl.textContent = "0.0000";
}

function drawGraph() {
    const w = canvas.width;
    const h = canvas.height;
    const padding = 50;
    const graphW = w - padding * 2;
    const graphH = h - padding * 2;

    ctx.clearRect(0, 0, w, h);

    // dark background
    ctx.fillStyle = "#0f0f23";
    ctx.fillRect(0, 0, w, h);

    // subtle grid
    ctx.strokeStyle = "rgba(0, 212, 255, 0.08)";
    ctx.lineWidth = 1;
    for (var i = 0; i <= 8; i++) {
        var y = padding + (i / 8) * graphH;
        ctx.beginPath();
        ctx.moveTo(padding, y);
        ctx.lineTo(w - padding, y);
        ctx.stroke();
    }
    for (var i = 0; i <= 12; i++) {
        var x = padding + (i / 12) * graphW;
        ctx.beginPath();
        ctx.moveTo(x, padding);
        ctx.lineTo(x, h - padding);
        ctx.stroke();
    }

    // axes
    ctx.strokeStyle = "#444";
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(padding, padding);
    ctx.lineTo(padding, h - padding);
    ctx.lineTo(w - padding, h - padding);
    ctx.stroke();

    // y-axis labels (BAC 0.00 to 0.40)
    ctx.fillStyle = "#888";
    ctx.font = "12px Courier New";
    ctx.textAlign = "right";
    for (var i = 0; i <= 4; i++) {
        var val = (i * 0.10).toFixed(2);
        var y = h - padding - (i / 4) * graphH;
        ctx.fillText(val, padding - 8, y + 4);
    }

    // x-axis label
    ctx.fillStyle = "#666";
    ctx.textAlign = "center";
    ctx.font = "11px Segoe UI, Arial";
    ctx.fillText("Time (seconds)", w / 2, h - 8);

    // y-axis label
    ctx.save();
    ctx.translate(4, h / 2);
    ctx.rotate(-Math.PI / 2);
    ctx.fillText("BAC", 0, 0);
    ctx.restore();

    if (data.length < 2) {
        // placeholder text when no data
        ctx.fillStyle = "#333";
        ctx.font = "16px Courier New";
        ctx.textAlign = "center";
        ctx.fillText("Waiting for data...", w / 2, h / 2);
        return;
    }

    // gradient fill under the line
    var gradient = ctx.createLinearGradient(0, padding, 0, h - padding);
    gradient.addColorStop(0, "rgba(0, 212, 255, 0.3)");
    gradient.addColorStop(1, "rgba(0, 212, 255, 0.0)");

    ctx.beginPath();
    for (var i = 0; i < data.length; i++) {
        var x = padding + (i / (maxPoints - 1)) * graphW;
        var y = h - padding - (data[i] / 0.40) * graphH;
        if (y < padding) y = padding;
        if (i === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    }
    // close the path down to the baseline for fill
    var lastX = padding + ((data.length - 1) / (maxPoints - 1)) * graphW;
    ctx.lineTo(lastX, h - padding);
    ctx.lineTo(padding, h - padding);
    ctx.closePath();
    ctx.fillStyle = gradient;
    ctx.fill();

    // glow effect on line
    ctx.shadowColor = "#00d4ff";
    ctx.shadowBlur = 10;
    ctx.strokeStyle = "#00d4ff";
    ctx.lineWidth = 2;
    ctx.beginPath();
    for (var i = 0; i < data.length; i++) {
        var x = padding + (i / (maxPoints - 1)) * graphW;
        var y = h - padding - (data[i] / 0.40) * graphH;
        if (y < padding) y = padding;
        if (i === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    }
    ctx.stroke();
    ctx.shadowBlur = 0;

    // dots at each point
    for (var i = 0; i < data.length; i++) {
        var x = padding + (i / (maxPoints - 1)) * graphW;
        var y = h - padding - (data[i] / 0.40) * graphH;
        if (y < padding) y = padding;

        ctx.fillStyle = "#00d4ff";
        ctx.shadowColor = "#00d4ff";
        ctx.shadowBlur = 6;
        ctx.beginPath();
        ctx.arc(x, y, 2.5, 0, Math.PI * 2);
        ctx.fill();
    }
    ctx.shadowBlur = 0;

    // latest point highlight
    var lx = padding + ((data.length - 1) / (maxPoints - 1)) * graphW;
    var ly = h - padding - (data[data.length - 1] / 0.40) * graphH;
    if (ly < padding) ly = padding;

    ctx.fillStyle = "#fff";
    ctx.shadowColor = "#00d4ff";
    ctx.shadowBlur = 12;
    ctx.beginPath();
    ctx.arc(lx, ly, 4, 0, Math.PI * 2);
    ctx.fill();
    ctx.shadowBlur = 0;
}

var ws = new WebSocket("ws://" + window.location.host + "/ws");

ws.onopen = function() {
    console.log("WebSocket connected");
    statusEl.textContent = "Live";
    statusEl.style.color = "#00ff88";
    setInterval(function() { ws.send("read"); }, 500);
};

ws.onmessage = function(event) {
    if (paused) return;

    var obj = JSON.parse(event.data);
    data.push(obj.bac);
    if (data.length > maxPoints) {
        data.shift();
    }

    if (obj.bac > peakBac) {
        peakBac = obj.bac;
    }

    readingEl.textContent = "BAC: " + obj.bac.toFixed(4);
    rawEl.textContent = obj.raw;
    mvEl.textContent = obj.mv.toFixed(0) + " mV";
    peakEl.textContent = peakBac.toFixed(4);

    drawGraph();
};

ws.onclose = function() {
    console.log("WebSocket disconnected");
    statusEl.textContent = "Offline";
    statusEl.style.color = "#ff4444";
};

drawGraph();
