const canvas = document.getElementById("graph");
const ctx = canvas.getContext("2d");
const readingEl = document.getElementById("reading");

const maxPoints = 60;
const data = [];

function drawGraph() {
    const w = canvas.width;
    const h = canvas.height;
    const padding = 40;
    const graphW = w - padding * 2;
    const graphH = h - padding * 2;

    ctx.clearRect(0, 0, w, h);

    // background
    ctx.fillStyle = "#ffffff";
    ctx.fillRect(padding, padding, graphW, graphH);

    // axes
    ctx.strokeStyle = "#000000";
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(padding, padding);
    ctx.lineTo(padding, h - padding);
    ctx.lineTo(w - padding, h - padding);
    ctx.stroke();

    // y-axis labels (BAC 0.00 to 0.40)
    ctx.fillStyle = "#000000";
    ctx.font = "12px Arial";
    ctx.textAlign = "right";
    for (let i = 0; i <= 4; i++) {
        const val = (i * 0.10).toFixed(2);
        const y = h - padding - (i / 4) * graphH;
        ctx.fillText(val, padding - 5, y + 4);

        // grid line
        ctx.strokeStyle = "#dddddd";
        ctx.beginPath();
        ctx.moveTo(padding, y);
        ctx.lineTo(w - padding, y);
        ctx.stroke();
    }

    // x-axis label
    ctx.textAlign = "center";
    ctx.fillText("Time (s)", w / 2, h - 5);

    // y-axis label
    ctx.save();
    ctx.translate(12, h / 2);
    ctx.rotate(-Math.PI / 2);
    ctx.fillText("BAC", 0, 0);
    ctx.restore();

    if (data.length < 2) return;

    // draw data line
    ctx.strokeStyle = "#cc0000";
    ctx.lineWidth = 2;
    ctx.beginPath();

    for (let i = 0; i < data.length; i++) {
        const x = padding + (i / (maxPoints - 1)) * graphW;
        const y = h - padding - (data[i] / 0.40) * graphH;
        if (i === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    }
    ctx.stroke();

    // draw dots
    ctx.fillStyle = "#cc0000";
    for (let i = 0; i < data.length; i++) {
        const x = padding + (i / (maxPoints - 1)) * graphW;
        const y = h - padding - (data[i] / 0.40) * graphH;
        ctx.beginPath();
        ctx.arc(x, y, 3, 0, Math.PI * 2);
        ctx.fill();
    }
}

const ws = new WebSocket("ws://" + window.location.host + "/ws");

ws.onopen = function() {
    console.log("WebSocket connected");
    // request data every 500ms
    setInterval(function() { ws.send("read"); }, 500);
};

ws.onmessage = function(event) {
    var obj = JSON.parse(event.data);
    data.push(obj.bac);
    if (data.length > maxPoints) {
        data.shift();
    }
    readingEl.textContent = "BAC: " + obj.bac.toFixed(4);
    drawGraph();
};

ws.onclose = function() {
    console.log("WebSocket disconnected");
};

drawGraph();
