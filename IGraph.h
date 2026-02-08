#pragma once
#include <vector>
#include <string>

// Generic Node representation
struct Node {
    int id; // Unique identifier for any graph type
    // Comparison operators for use in maps/sets
    bool operator==(const Node& other) const { return id == other.id; }
    bool operator!=(const Node& other) const { return id != other.id; }
    bool operator<(const Node& other) const { return id < other.id; }
};

// Generic Edge representation
struct Edge {
    Node target;
    int weight;
};

// Core Graph Interface
class IGraph {
public:
    virtual ~IGraph() = default;
    virtual std::vector<Edge> getNeighbors(Node n) const = 0;
    virtual int getHeuristic(Node start, Node target) const { return 0; } // Optional for A*
};

// Interface for observing algorithm progress (Visualization)
class IAlgorithmObserver {
public:
    virtual ~IAlgorithmObserver() = default;
    virtual void onNodeVisited(Node n) = 0;
    virtual void onNodeCurrent(Node n) = 0;
    virtual void onLog(const std::string& msg) = 0;
};
