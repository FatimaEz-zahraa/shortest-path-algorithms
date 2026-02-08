# GraphAlgo Visualizer Pro v3.0

A high-performance, interactive pathfinding visualizer built with **C++ (WebAssembly)** and **JavaScript**. This project allows users to visualize and compare pathfinding algorithms like Dijkstra, A*, and BFS on a weighted grid.

## 🌟 Features

-   **Dual Mode Comparison**: Run two algorithms side-by-side (e.g., Dijkstra vs A*) to compare efficiency.
-   **Real-time Statistics**:
    -   **Time**: Execution time in milliseconds.
    -   **Speed**: Nodes visited per millisecond.
    -   **Cost**: True weighted path cost.
    -   **Visited**: Total number of nodes explored.
-   **Interactive Grid**:
    -   **Draw Walls**: Create obstacles.
    -   **Add Weights**: Set custom weights (cost) for cells.
    -   **Move Start/End**: Drag and drop start/end points.
    -   **Random Maze**: Generate complex mazes instantly.
-   **High Performance**: Core algorithms are compiled to WebAssembly (Wasm) for near-native speed.

## 🛠️ Technology Stack

-   **Frontend**: HTML5, CSS3 (Modern Glassmorphism Design), JavaScript.
-   **Backend Logic**: C++ 17 (compiled to Wasm via Emscripten).
-   **Build Tools**: Emscripten (emsdk), Python/Node (for local serving).

## 🚀 How to Run Locally

### Prerequisites
-   A local web server (due to Wasm CORS policies).

### Steps
1.  Clone the repository.
2.  Run the provided server script:
    -   **Windows**: Double-click `run_server.bat`.
    -   **Manual**: `python -m http.server 8080`
3.  Open `http://localhost:8080` in your browser.

## 📦 Deployment

This project is static and can be deployed on **Netlify**, **GitHub Pages**, or **Vercel**.
See `DEPLOY.md` for detailed instructions.

## 📷 Screenshots

*(Add your screenshots here)*

## 📄 License

This project is open-source and available under the MIT License.
