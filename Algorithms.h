#pragma once
#include "IGraph.h"
#include <vector>
#include <string>

struct AlgoResult {
    std::vector<Node> path;
    int visitedCount;
    int totalCost;
    double timeMs;
    bool success;
};

AlgoResult runDijkstra(const IGraph& graph, Node start, Node end, IAlgorithmObserver* observer = nullptr);
AlgoResult runBFS(const IGraph& graph, Node start, Node end, IAlgorithmObserver* observer = nullptr);
AlgoResult runAStar(const IGraph& graph, Node start, Node end, IAlgorithmObserver* observer = nullptr);
