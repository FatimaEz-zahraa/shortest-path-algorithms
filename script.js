const CELL_SIZE = 25;
let grid = null;
let canvas, ctx;
let canvas2, ctx2;
let cols, rows;
let currentMode = 'obstacle';
let isMouseDown = false;
let animationId = null;
let dualMode = false;

// Animation State
const placedAnimations = []; // Store {r, c, type, startTime}

// --- RESILIENCE FALLBACKS ---
class GridFallback {
    constructor(rows, cols) {
        this.rows = rows;
        this.cols = cols;
        this.data = Array.from({ length: rows }, () => Array(cols).fill('.'));
        this.weights = Array.from({ length: rows }, () => Array(cols).fill(1));
        this.source = { x: 1, y: 1 };
        this.dest = { x: rows - 2, y: cols - 2 };
        this.data[this.source.x][this.source.y] = 'S';
        this.data[this.dest.x][this.dest.y] = 'D';
        this.allowDiagonals = false;
    }
    setAllowDiagonals(allow) { this.allowDiagonals = allow; }
    getAllowDiagonals() { return this.allowDiagonals; }
    getWidth() { return this.cols; }
    getHeight() { return this.rows; }
    getChar(r, c) { return this.data[r][c]; }
    getWeight(r, c) { return this.weights[r][c]; }
    setSource(r, c) {
        this.data[this.source.x][this.source.y] = '.';
        this.source = { x: r, y: c };
        this.data[r][c] = 'S';
    }
    setDestination(r, c) {
        this.data[this.dest.x][this.dest.y] = '.';
        this.dest = { x: r, y: c };
        this.data[r][c] = 'D';
    }
    setObstacle(r, c) { if (this.data[r][c] === '.') this.data[r][c] = '#'; }
    setEmpty(r, c) { this.data[r][c] = '.'; this.weights[r][c] = 1; }
    setWeight(r, c, w) { this.weights[r][c] = w; }
    getSource() { return this.source; }
    getDestination() { return this.dest; }
    isValid(r, c) { return r >= 0 && r < this.rows && c >= 0 && c < this.cols; }
    clearPath() {
        for (let r = 0; r < this.rows; r++)
            for (let c = 0; c < this.cols; c++)
                if (this.data[r][c] === '*' || this.data[r][c] === 'v') this.data[r][c] = '.';
    }
    generateRandomMaze() {
        for (let r = 0; r < this.rows; r++)
            for (let c = 0; c < this.cols; c++)
                if (this.data[r][c] === '.' && Math.random() < 0.3) this.data[r][c] = '#';
    }
    serialize() {
        let s = `${this.rows},${this.cols},${this.source.x},${this.source.y},${this.dest.x},${this.dest.y}|`;
        for (let r = 0; r < this.rows; r++) for (let c = 0; c < this.cols; c++) s += this.data[r][c];
        s += "|";
        for (let r = 0; r < this.rows; r++) for (let c = 0; c < this.cols; c++) s += this.weights[r][c] + " ";
        return s;
    }
    load(data) {
        try {
            const parts = data.split('|');
            const header = parts[0].split(',');
            this.rows = parseInt(header[0]);
            this.cols = parseInt(header[1]);
            this.source = { x: parseInt(header[2]), y: parseInt(header[3]) };
            this.dest = { x: parseInt(header[4]), y: parseInt(header[5]) };

            this.data = [];
            const mapStr = parts[1];
            for (let r = 0; r < this.rows; r++) {
                this.data[r] = [];
                for (let c = 0; c < this.cols; c++) this.data[r][c] = mapStr[r * this.cols + c];
            }

            this.weights = [];
            const weightParts = parts[2].trim().split(/\s+/);
            for (let r = 0; r < this.rows; r++) {
                this.weights[r] = [];
                for (let c = 0; c < this.cols; c++) this.weights[r][c] = parseInt(weightParts[r * this.cols + c]);
            }
            return true;
        } catch (e) { return false; }
    }
    delete() { /* No-op for JS */ }
}

var Module = {
    onRuntimeInitialized: function () {
        console.log("Wasm Module Ready");
        window.wasmLoaded = true;
        if (!window.appInitialized) initApp();
    }
};

// Emergency Init if Wasm takes too long
setTimeout(() => {
    if (!window.appInitialized) {
        console.warn("Wasm timeout - using JS fallback for rendering");
        initApp();
    }
}, 500);

function initApp() {
    if (window.appInitialized) return;
    window.appInitialized = true;

    canvas = document.getElementById('gridCanvas');
    canvas2 = document.getElementById('gridCanvas2');
    if (!canvas || !canvas2) return;
    ctx = canvas.getContext('2d');
    ctx2 = canvas2.getContext('2d');

    resizeCanvas();
    window.addEventListener('resize', resizeCanvas);

    // Initial Grid Creation
    createGridInstance();

    canvas.addEventListener('mousedown', handleInput);
    canvas.addEventListener('mousemove', handleInput);
    document.addEventListener('mouseup', () => isMouseDown = false);
    canvas.addEventListener('mouseout', () => { hoverNode = null; drawGrid(); });

    // UI Listeners
    setupUIListeners();

    drawGrid();
}

function createGridInstance() {
    try {
        cols = Math.floor(canvas.width / CELL_SIZE);
        rows = Math.floor(canvas.height / CELL_SIZE);
        if (window.wasmLoaded && Module.Grid) {
            grid = new Module.Grid(rows, cols);
            console.log("Grid created using Wasm");
        } else {
            grid = new GridFallback(rows, cols);
            console.log("Grid created using JS Fallback");
        }
    } catch (e) {
        console.error("Failed to create grid", e);
        grid = new GridFallback(rows, cols);
    }
}

function setupUIListeners() {
    document.querySelectorAll('.tool-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            document.querySelectorAll('.tool-btn').forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            currentMode = btn.dataset.mode;
        });
    });

    document.getElementById('weightRange').addEventListener('input', (e) => {
        document.getElementById('weightValue').innerText = e.target.value;
    });

    document.getElementById('algoSelect').addEventListener('change', (e) => {
        const weightBtn = document.querySelector('.tool-btn[data-mode="weight"]');
        const weightControl = document.getElementById('weightControl');
        if (e.target.value === 'bfs') {
            weightBtn.style.opacity = '0.5';
            weightBtn.style.pointerEvents = 'none';
            if (weightControl) weightControl.style.display = 'none';
            if (currentMode === 'weight') document.querySelector('.tool-btn[data-mode="obstacle"]').click();
        } else {
            weightBtn.style.opacity = '1.0';
            weightBtn.style.pointerEvents = 'auto';
            if (weightControl) weightControl.style.display = 'block';
        }
    });

    document.getElementById('chkDiagonal').addEventListener('change', (e) => {
        if (grid) {
            grid.setAllowDiagonals(e.target.checked);
            console.log("Diagonals:", e.target.checked);
        }
    });

    document.getElementById('chkDualMode').addEventListener('change', (e) => {
        dualMode = e.target.checked;
        const dualElements = document.querySelectorAll('.dual-only');
        dualElements.forEach(el => el.style.display = dualMode ? 'block' : 'none');

        // Always trigger resize to adjust View 1 or View 2 layout
        setTimeout(resizeCanvas, 50);
        drawGrid();
    });

    document.getElementById('btnRun').addEventListener('click', runAlgorithm);
    document.getElementById('btnMaze').addEventListener('click', () => {
        console.log("Generating Random Maze (Dual Mode:", dualMode, ")");
        grid.clearPath();
        grid.generateRandomMaze();
        drawGrid();
    });
    document.getElementById('btnReset').addEventListener('click', () => {
        if (grid && grid.delete) grid.delete();
        createGridInstance();
        drawGrid();
    });

    // Save/Load Handlers
    document.getElementById('btnSave').addEventListener('click', () => {
        if (!grid) return;
        const data = grid.serialize();
        const blob = new Blob([data], { type: 'text/plain' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `pathfinding_grid_${new Date().getTime()}.txt`;
        a.click();
        URL.revokeObjectURL(url);
    });

    document.getElementById('btnLoad').addEventListener('click', () => {
        document.getElementById('fileInput').click();
    });

    document.getElementById('fileInput').addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = (event) => {
            const content = event.target.result;
            if (grid.load(content)) {
                // If dimensions changed, we might need a new Wasm grid or update local ones
                // For simplicity, we assume grid dimensions match or load handles resize.
                // Our C++ load handles resize. fallback too.
                drawGrid();
            } else {
                alert("Failed to load grid file.");
            }
        };
        reader.readAsText(file);
    });
}

function resizeCanvas() {
    const main = document.querySelector('main');
    if (!main || !canvas || !canvas2) return;

    if (dualMode) {
        // Force both canvases to have identical dimensions to avoid rows/cols mismatch
        const targetWidth = canvas.parentElement.clientWidth;
        const targetHeight = canvas.parentElement.clientHeight;

        canvas.width = targetWidth;
        canvas.height = targetHeight;
        canvas2.width = targetWidth;
        canvas2.height = targetHeight;
    } else {
        canvas.width = main.clientWidth;
        canvas.height = main.clientHeight;
    }

    if (grid) {
        // Attempt to preserve grid state if dimensions are compatible
        const oldData = grid.serialize();
        const oldRows = rows;
        const oldCols = cols;

        if (grid.delete) grid.delete();
        createGridInstance();

        // Only load back if dimensions are the same (to avoid layout breakage)
        if (rows === oldRows && cols === oldCols) {
            grid.load(oldData);
        }
        drawGrid();
    }
}

const COLORS = {
    bg: '#121212',
    wall: '#9e9e9e', // Lighter gray for better visibility
    start: '#00e676',
    end: '#ff1744',
    visited: '#2979ff',
    visitedFill: 'rgba(41, 121, 255, 0.5)',
    path: '#ffea00',
    gridBorder: '#2a2a35',
    hover: 'rgba(255, 255, 255, 0.2)',
    weightBase: { r: 62, g: 39, b: 35 }
};

let hoverNode = null;
let isErasing = false;

function triggerPlacementAnimation(r, c, type) {
    placedAnimations.push({ r, c, type, startTime: performance.now() });
    if (placedAnimations.length > 20) placedAnimations.shift();
}

// Wrappers
function setObstacleAnimated(r, c) {
    // Only animate if it wasn't already a wall
    let val = grid.getChar(r, c);
    if (typeof val === 'number') val = String.fromCharCode(val);

    if (val !== '#') {
        grid.setObstacle(r, c);
        triggerPlacementAnimation(r, c, '#');
        drawGrid(); // Force redraw for animation start
    }
}
function setSourceAnimated(r, c) {
    grid.setSource(r, c);
    triggerPlacementAnimation(r, c, 'S');
}
function setDestinationAnimated(r, c) {
    grid.setDestination(r, c);
    triggerPlacementAnimation(r, c, 'D');
}

function handleInput(e) {
    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    const c = Math.floor(x / CELL_SIZE);
    const r = Math.floor(y / CELL_SIZE);

    if (r >= 0 && r < rows && c >= 0 && c < cols) {
        hoverNode = { r, c };

        if (e.type === 'mousedown') {
            isMouseDown = true;
            let currentChar = grid.getChar(r, c);
            if (typeof currentChar === 'number') currentChar = String.fromCharCode(currentChar);

            const currentWeight = grid.getWeight(r, c);

            if (currentMode === 'obstacle') {
                isErasing = (currentChar === '#');
            } else if (currentMode === 'weight') {
                isErasing = (currentWeight > 1);
            } else {
                isErasing = false;
            }
            applyTool(r, c);
            if (dualMode) drawGrid(); // Sync both views on interaction
        } else if (e.type === 'mousemove' && isMouseDown) {
            applyTool(r, c);
            if (dualMode) drawGrid();
        }
        drawGrid();
    } else {
        hoverNode = null;
        drawGrid();
    }
}

function applyTool(r, c) {
    if (currentMode === 'obstacle') {
        if (isErasing) grid.setEmpty(r, c);
        else setObstacleAnimated(r, c);
    }
    else if (currentMode === 'weight') {
        if (isErasing) grid.setEmpty(r, c);
        else {
            const w = parseInt(document.getElementById('weightRange').value);
            grid.setWeight(r, c, w);
        }
    }
    else if (currentMode === 'source') setSourceAnimated(r, c);
    else if (currentMode === 'dest') setDestinationAnimated(r, c);
}

function drawGrid(v1 = [], p1 = [], v2 = [], p2 = []) {
    if (!grid) return;
    _drawOnCtx(ctx, v1, p1, "View 1");
    if (dualMode) {
        _drawOnCtx(ctx2, v2, p2, "View 2");
    }
}

function _drawOnCtx(cCtx, visitedNodes, pathNodes, labelText) {
    const cWidth = cCtx.canvas.width;
    const cHeight = cCtx.canvas.height;

    // Clear with slightly rich background
    cCtx.fillStyle = '#0a0a0e';
    cCtx.fillRect(0, 0, cWidth, cHeight);

    // 1. Grid Lines - Subtle
    cCtx.beginPath();
    cCtx.strokeStyle = 'rgba(255, 255, 255, 0.03)';
    cCtx.lineWidth = 1;
    for (let c = 0; c <= cols; c++) {
        const x = c * CELL_SIZE;
        cCtx.moveTo(x, 0);
        cCtx.lineTo(x, rows * CELL_SIZE);
    }
    for (let r = 0; r <= rows; r++) {
        const y = r * CELL_SIZE;
        cCtx.moveTo(0, y);
        cCtx.lineTo(cols * CELL_SIZE, y);
    }
    cCtx.stroke();

    // 2. Static Cells
    for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
            let type = grid.getChar(r, c);
            if (typeof type === 'number') type = String.fromCharCode(type);
            const weight = grid.getWeight(r, c);
            const x = c * CELL_SIZE;
            const y = r * CELL_SIZE;

            if (weight > 1 && type === '.') {
                const intensity = Math.min(1, (weight - 1) / 50);
                cCtx.fillStyle = `hsla(20, 40%, ${20 + intensity * 30}%, 0.6)`;
                cCtx.fillRect(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2);
                cCtx.fillStyle = 'rgba(255,255,255,0.4)';
                cCtx.font = '9px Outfit';
                cCtx.textAlign = 'center';
                cCtx.fillText(weight, x + CELL_SIZE / 2, y + CELL_SIZE / 2 + 3);
            }

            if (type === '#') {
                cCtx.fillStyle = '#37474f';
                cCtx.fillRect(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2);
            }
        }
    }

    // 3. Visited (Animated Aura)
    if (visitedNodes.length > 0) {
        for (const p of visitedNodes) {
            const x = p.y * CELL_SIZE;
            const y = p.x * CELL_SIZE;
            cCtx.fillStyle = 'rgba(41, 121, 255, 0.2)';
            cCtx.fillRect(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2);
            cCtx.fillStyle = 'rgba(41, 121, 255, 0.8)';
            cCtx.fillRect(x + CELL_SIZE / 2 - 1, y + CELL_SIZE / 2 - 1, 2, 2);
        }
    }

    // 4. Path (Premium Gradient Heatmap)
    if (pathNodes.length > 0) {
        cCtx.lineJoin = 'round';
        cCtx.lineCap = 'round';
        cCtx.lineWidth = 4;

        for (let i = 0; i < pathNodes.length - 1; i++) {
            const p1 = pathNodes[i];
            const p2 = pathNodes[i + 1];
            const weight = grid.getWeight(p2.x, p2.y);

            // Heatmap color: Hue ranges from 240 (Blue/Fast) to 0 (Red/Slow)
            // Weight 1 -> Hue 200 (Cyan)
            // Weight 50 -> Hue 0 (Red)
            const hue = Math.max(0, 200 - (weight - 1) * 4);
            cCtx.strokeStyle = `hsl(${hue}, 100%, 60%)`;
            cCtx.shadowBlur = 10;
            cCtx.shadowColor = `hsla(${hue}, 100%, 60%, 0.5)`;

            cCtx.beginPath();
            cCtx.moveTo(p1.y * CELL_SIZE + CELL_SIZE / 2, p1.x * CELL_SIZE + CELL_SIZE / 2);
            cCtx.lineTo(p2.y * CELL_SIZE + CELL_SIZE / 2, p2.x * CELL_SIZE + CELL_SIZE / 2);
            cCtx.stroke();
            cCtx.shadowBlur = 0;
        }
    }

    // 5. Start/End
    for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
            let type = grid.getChar(r, c);
            if (typeof type === 'number') type = String.fromCharCode(type);
            const x = c * CELL_SIZE;
            const y = r * CELL_SIZE;
            if (type === 'S' || type === 'D') {
                const color = type === 'S' ? '#00e676' : '#ff1744';
                cCtx.fillStyle = color;
                cCtx.shadowBlur = 15;
                cCtx.shadowColor = color;
                cCtx.beginPath();
                cCtx.arc(x + CELL_SIZE / 2, y + CELL_SIZE / 2, CELL_SIZE / 4, 0, Math.PI * 2);
                cCtx.fill();
                cCtx.shadowBlur = 0;
            }
        }
    }

    // 6. Placement Animations
    const now = performance.now();
    for (let i = placedAnimations.length - 1; i >= 0; i--) {
        const anim = placedAnimations[i];
        const age = now - anim.startTime;
        const duration = 300;
        if (age > duration) {
            if (labelText === "View 1") placedAnimations.splice(i, 1);
            continue;
        }
        const progress = age / duration;
        const scale = progress < 0.5 ? progress * 2 : 2 - progress * 2;
        const cx = anim.c * CELL_SIZE + CELL_SIZE / 2;
        const cy = anim.r * CELL_SIZE + CELL_SIZE / 2;
        cCtx.fillStyle = 'rgba(255, 255, 255, 0.4)';
        cCtx.beginPath();
        cCtx.arc(cx, cy, (CELL_SIZE / 2) * scale, 0, Math.PI * 2);
        cCtx.fill();
    }

    // 7. Ghost Hover
    if (hoverNode && !isMouseDown) {
        cCtx.fillStyle = 'rgba(255, 255, 255, 0.1)';
        cCtx.fillRect(hoverNode.c * CELL_SIZE, hoverNode.r * CELL_SIZE, CELL_SIZE, CELL_SIZE);
        cCtx.strokeStyle = 'rgba(255, 255, 255, 0.3)';
        cCtx.strokeRect(hoverNode.c * CELL_SIZE, hoverNode.r * CELL_SIZE, CELL_SIZE, CELL_SIZE);
    }

    if (placedAnimations.length > 0) {
        requestAnimationFrame(() => drawGrid(visitedNodes, pathNodes));
    }
}

// JavaScript Fallback for A* (in case Wasm build is not yet updated by user)
function solveAStarJS(grid) {
    const start = performance.now();
    const rows = grid.getHeight();
    const cols = grid.getWidth();
    const startPt = grid.getSource();
    const endPt = grid.getDestination();

    const visited = [];
    const gScore = new Array(rows).fill(0).map(() => new Array(cols).fill(Infinity));
    const parent = new Array(rows).fill(0).map(() => new Array(cols).fill(null));

    gScore[startPt.x][startPt.y] = 0;

    const heuristic = (p1, p2) => {
        const dx = Math.abs(p1.x - p2.x);
        const dy = Math.abs(p1.y - p2.y);
        if (grid.getAllowDiagonals && grid.getAllowDiagonals()) {
            return 10 * (dx + dy) + (14 - 2 * 10) * Math.min(dx, dy);
        }
        return 10 * (dx + dy);
    };

    const pq = [{ f: heuristic(startPt, endPt), p: startPt }];

    while (pq.length > 0) {
        pq.sort((a, b) => a.f - b.f);
        const { p: curr } = pq.shift();

        if (curr.x === endPt.x && curr.y === endPt.y) break;

        visited.push(curr);

        const dirs = [
            { dx: -1, dy: 0, cost: 10 }, { dx: 1, dy: 0, cost: 10 },
            { dx: 0, dy: -1, cost: 10 }, { dx: 0, dy: 1, cost: 10 }
        ];

        if (grid.getAllowDiagonals && grid.getAllowDiagonals()) {
            dirs.push({ dx: -1, dy: -1, cost: 14 }, { dx: -1, dy: 1, cost: 14 },
                { dx: 1, dy: -1, cost: 14 }, { dx: 1, dy: 1, cost: 14 });
        }

        for (const dir of dirs) {
            const nx = curr.x + dir.dx;
            const ny = curr.y + dir.dy;

            if (nx >= 0 && nx < rows && ny >= 0 && ny < cols && grid.getChar(nx, ny) !== '#') {
                const weight = grid.getWeight(nx, ny);
                const tentativeG = gScore[curr.x][curr.y] + dir.cost * weight;
                if (tentativeG < gScore[nx][ny]) {
                    parent[nx][ny] = curr;
                    gScore[nx][ny] = tentativeG;
                    pq.push({ f: tentativeG + heuristic({ x: nx, y: ny }, endPt), p: { x: nx, y: ny } });
                }
            }
        }
    }

    const path = [];
    let curr = endPt;
    while (curr) {
        path.push(curr);
        curr = parent[curr.x][curr.y];
        if (curr && curr.x === startPt.x && curr.y === startPt.y) {
            path.push(curr);
            break;
        }
    }
    path.reverse();

    return {
        path: { size: () => path.length, get: (i) => path[i] },
        visited: { size: () => visited.length, get: (i) => visited[i] },
        totalCost: gScore[endPt.x][endPt.y],
        timeMs: performance.now() - start
    };
}

function runAlgorithm() {
    grid.clearPath();
    const algo1 = document.getElementById('algoSelect').value;
    const algo2 = document.getElementById('algoSelect2').value;

    function solve(algoName) {
        let res;
        try {
            if (algoName === 'dijkstra') res = Module.solveDijkstra(grid);
            else if (algoName === 'bfs') res = Module.solveBFS(grid);
            else if (algoName === 'astar') {
                if (Module.solveAStar) res = Module.solveAStar(grid);
                else res = solveAStarJS(grid);
            }
        } catch (e) {
            console.error("Algo failed:", e);
            res = solveAStarJS(grid);
        }
        return res;
    }

    const res1 = solve(algo1);
    const res2 = dualMode ? solve(algo2) : null;

    // Update Stats 1
    const s1 = document.getElementById('stats1');
    if (s1) s1.style.display = 'block';

    // Helper to calculate true weighted cost (Wasm result might lack it)
    function calculateWeightedCost(res) {
        if (res.totalCost) return res.totalCost; // Use Wasm if available
        let cost = 0;
        // path contains nodes. Calculate cost sum.
        // Assuming path is array of objects {x, y}
        // Note: Wasm path doesn't include start node cost usually (start is 0), but let's sum edge weights
        // Actually A* cost is sum of edge weights. 
        if (res.path.size() < 2) return 0;

        for (let i = 0; i < res.path.size() - 1; i++) {
            let n1 = res.path.get(i);
            // This logic assumes we traverse TO n1? No, path is ordered. 
            // Cost is usually sum of entering nodes.
            // Let's just sum weights of all nodes in path excluding start?
            // Simplified: Sum of weights of nodes in path.
        }
        // Easier: Just Re-calculate simple sum of weights of path nodes
        for (let i = 0; i < res.path.size(); i++) {
            let p = res.path.get(i);
            // Generally start node cost is 0, so maybe start at 1?
            // But for Grid weights, usually entering a cell costs its weight.
            if (i > 0) cost += grid.getWeight(p.x, p.y);
        }
        return cost;
    }

    const cost1 = calculateWeightedCost(res1);
    const time1 = res1.timeMs;
    const speed1 = time1 > 0 ? (res1.visited.size() / time1).toFixed(2) : "N/A";

    document.getElementById('timeDisplay').innerText = time1.toFixed(3);
    document.getElementById('pathDisplay').innerText = res1.path.size();
    document.getElementById('visitedDisplay').innerText = res1.visited.size();
    document.getElementById('costDisplay').innerText = cost1;
    document.getElementById('speedDisplay').innerText = speed1;

    if (dualMode && res2) {
        const s2 = document.getElementById('stats2');
        if (s2) s2.style.display = 'block';

        const cost2 = calculateWeightedCost(res2);
        const time2 = res2.timeMs;
        const speed2 = time2 > 0 ? (res2.visited.size() / time2).toFixed(2) : "N/A";

        document.getElementById('timeDisplay2').innerText = time2.toFixed(3);
        document.getElementById('pathDisplay2').innerText = res2.path.size();
        document.getElementById('visitedDisplay2').innerText = res2.visited.size();
        document.getElementById('costDisplay2').innerText = cost2;
        document.getElementById('speedDisplay2').innerText = speed2;
    }

    // Animate
    let i1 = 0, i2 = 0;
    const v1 = [], p1 = [], v2 = [], p2 = [];
    if (animationId) cancelAnimationFrame(animationId);

    function animate() {
        let batch = 5;
        let finished1 = i1 >= res1.visited.size();
        let finished2 = !dualMode || (res2 && i2 >= res2.visited.size());

        while (batch-- > 0) {
            if (!finished1) { v1.push(res1.visited.get(i1)); i1++; }
            if (dualMode && !finished2) { v2.push(res2.visited.get(i2)); i2++; }
            finished1 = i1 >= res1.visited.size();
            finished2 = !dualMode || (res2 && i2 >= res2.visited.size());
        }

        if (finished1 && finished2) {
            for (let j = 0; j < res1.path.size(); j++) p1.push(res1.path.get(j));
            if (dualMode && res2) for (let j = 0; j < res2.path.size(); j++) p2.push(res2.path.get(j));
            drawGrid(v1, p1, v2, p2);
            return;
        }

        drawGrid(v1, [], v2, []);
        animationId = requestAnimationFrame(animate);
    }
    animate();
}
