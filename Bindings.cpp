#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <vector>
#include <string>
#include <chrono>
#include "Grid.h"
#include "Algorithms.h"

using namespace emscripten;

// Result structure specifically formatted for the JS interface
struct WasmResult {
    std::vector<Point> path;
    std::vector<Point> visited;
    double timeMs;
    bool success;
};

// Wasm Implementation of the Observer to capture steps for the frontend animation
class WasmObserver : public IAlgorithmObserver {
public:
    WasmObserver(const Grid& grid) : m_grid(grid) {}
    
    void onNodeVisited(Node n) override {
        visited.push_back(m_grid.toPoint(n));
    }
    
    void onNodeCurrent(Node n) override {
        // Could be used for specific "current node" animation in JS
    }
    
    void onLog(const std::string& msg) override {
        // Optional: send logs back to JS via emscripten::val if needed
    }

    std::vector<Point> visited;

private:
    const Grid& m_grid;
};

// Helper to convert core result to JS-friendly structure
WasmResult convertResult(const AlgoResult& res, const Grid& grid, const std::vector<Point>& visited) {
    WasmResult wr;
    wr.visited = visited;
    wr.timeMs = res.timeMs;
    wr.success = res.success;
    for (auto n : res.path) {
        wr.path.push_back(grid.toPoint(n));
    }
    return wr;
}

WasmResult solveDijkstra(Grid& grid) {
    WasmObserver observer(grid);
    Node start = grid.toNode(grid.getSource().x, grid.getSource().y);
    Node end = grid.toNode(grid.getDestination().x, grid.getDestination().y);
    
    AlgoResult res = runDijkstra(grid, start, end, &observer);
    return convertResult(res, grid, observer.visited);
}

WasmResult solveBFS(Grid& grid) {
    WasmObserver observer(grid);
    Node start = grid.toNode(grid.getSource().x, grid.getSource().y);
    Node end = grid.toNode(grid.getDestination().x, grid.getDestination().y);
    
    AlgoResult res = runBFS(grid, start, end, &observer);
    return convertResult(res, grid, observer.visited);
}

WasmResult solveAStar(Grid& grid) {
    WasmObserver observer(grid);
    Node start = grid.toNode(grid.getSource().x, grid.getSource().y);
    Node end = grid.toNode(grid.getDestination().x, grid.getDestination().y);
    
    AlgoResult res = runAStar(grid, start, end, &observer);
    return convertResult(res, grid, observer.visited);
}

EMSCRIPTEN_BINDINGS(my_module) {
    value_object<Point>("Point")
        .field("x", &Point::x)
        .field("y", &Point::y);

    value_object<WasmResult>("AlgoResult")
        .field("path", &WasmResult::path)
        .field("visited", &WasmResult::visited)
        .field("timeMs", &WasmResult::timeMs)
        .field("success", &WasmResult::success);

    register_vector<Point>("vector<Point>");
    
    class_<Grid>("Grid")
        .constructor<int, int>()
        .function("setObstacle", &Grid::setObstacle)
        .function("setSource", &Grid::setSource)
        .function("setDestination", &Grid::setDestination)
        .function("setWeight", &Grid::setWeight)
        .function("getWeight", &Grid::getWeight)
        .function("getChar", &Grid::getChar)
        .function("getWidth", &Grid::getWidth)
        .function("getHeight", &Grid::getHeight)
        .function("setEmpty", &Grid::setEmpty)
        .function("setAllowDiagonals", &Grid::setAllowDiagonals)
        .function("getAllowDiagonals", &Grid::getAllowDiagonals)
        .function("generateRandomMaze", &Grid::generateRandomMaze)
        .function("serialize", &Grid::serialize)
        .function("load", &Grid::load)
        .function("clearPath", &Grid::clearPath);

    function("solveDijkstra", &solveDijkstra);
    function("solveBFS", &solveBFS);
    function("solveAStar", &solveAStar);
}
