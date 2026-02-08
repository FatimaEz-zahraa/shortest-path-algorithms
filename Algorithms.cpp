#include "Algorithms.h"
#include <queue>
#include <map>
#include <algorithm>
#include <chrono>

using namespace std;

// Internal Helper to reconstruct path from parent map
vector<Node> reconstructPathInternal(const map<Node, Node>& parent, Node start, Node end) {
    vector<Node> path;
    Node curr = end;
    while (curr.id != -1) {
        path.push_back(curr);
        if (curr == start) break;
        if (parent.find(curr) == parent.end()) break;
        curr = parent.at(curr);
    }
    reverse(path.begin(), path.end());
    if (path.empty() || path[0] != start) return {};
    return path;
}

AlgoResult runDijkstra(const IGraph& graph, Node start, Node end, IAlgorithmObserver* observer) {
    auto startTime = chrono::high_resolution_clock::now();
    AlgoResult res = { {}, 0, 0, 0, false };

    map<Node, int> dist;
    map<Node, Node> parent;
    priority_queue<pair<int, Node>, vector<pair<int, Node>>, greater<pair<int, Node>>> pq;

    dist[start] = 0;
    parent[start] = { -1 };
    pq.push({ 0, start });

    if (observer) observer->onLog("Core: Starting Dijkstra...");

    while (!pq.empty()) {
        int d = pq.top().first;
        Node curr = pq.top().second;
        pq.pop();

        if (dist.count(curr) && d > dist[curr]) continue;
        
        res.visitedCount++;
        if (observer) {
            // Mark visited in the graph FOR VISUALIZATION (Windows app)
            // graph is const IGraph&, but we can cast it if we know it's a grid
            // or we add a non-const version. 
            // Better: use the observer to notify the owner to update.
            // But main.cpp's observer already knows the HWND. 
            // We need a way to tell the grid. 
            observer->onNodeVisited(curr);
        }

        if (curr == end) {
            res.success = true;
            break;
        }

        for (auto& edge : graph.getNeighbors(curr)) {
            int newDist = d + edge.weight;
            if (dist.find(edge.target) == dist.end() || newDist < dist[edge.target]) {
                dist[edge.target] = newDist;
                parent[edge.target] = curr;
                pq.push({ newDist, edge.target });
                if (observer) observer->onLog("Core: Node " + to_string(edge.target.id) + " reachable with distance " + to_string(newDist));
            }

        }
    }

    res.path = reconstructPathInternal(parent, start, end);
    if (res.success) res.totalCost = dist[end];
    
    auto endTime = chrono::high_resolution_clock::now();
    res.timeMs = chrono::duration<double, milli>(endTime - startTime).count();
    return res;
}

AlgoResult runBFS(const IGraph& graph, Node start, Node end, IAlgorithmObserver* observer) {
    auto startTime = chrono::high_resolution_clock::now();
    AlgoResult res = { {}, 0, 0, 0, false };

    map<Node, Node> parent;
    queue<Node> q;

    parent[start] = { -1 };
    q.push(start);

    if (observer) observer->onLog("Core: Starting Breadth-First Search (BFS)...");

    while (!q.empty()) {
        Node curr = q.front();
        q.pop();

        res.visitedCount++;
        if (observer) {
            observer->onNodeVisited(curr);
        }

        if (curr == end) {
            res.success = true;
            if (observer) observer->onLog("Core: Target node reached by BFS.");
            break;
        }

        for (auto& edge : graph.getNeighbors(curr)) {
            if (parent.find(edge.target) == parent.end()) {
                parent[edge.target] = curr;
                q.push(edge.target);
                if (observer) observer->onLog("Core: Enqueuing neighbor node " + to_string(edge.target.id));
            }
        }
    }

    res.path = reconstructPathInternal(parent, start, end);
    if (res.success) {
        res.totalCost = (int)res.path.size() - 1;
        if (observer) observer->onLog("Core: BFS finished. Path found.");
    } else {
        if (observer) observer->onLog("Core: BFS finished. No path found.");
    }

    auto endTime = chrono::high_resolution_clock::now();
    res.timeMs = chrono::duration<double, milli>(endTime - startTime).count();
    return res;
}

AlgoResult runAStar(const IGraph& graph, Node start, Node end, IAlgorithmObserver* observer) {
    auto startTime = chrono::high_resolution_clock::now();
    AlgoResult res = { {}, 0, 0, 0, false };

    map<Node, int> gScore;
    map<Node, Node> parent;
    priority_queue<pair<int, Node>, vector<pair<int, Node>>, greater<pair<int, Node>>> pq;

    gScore[start] = 0;
    parent[start] = { -1 };
    pq.push({ graph.getHeuristic(start, end), start });

    if (observer) observer->onLog("Core: Starting A*...");

    while (!pq.empty()) {
        Node curr = pq.top().second;
        pq.pop();

        res.visitedCount++;
        if (observer) {
            observer->onNodeVisited(curr);
        }

        if (curr == end) {
            res.success = true;
            break;
        }

        for (auto& edge : graph.getNeighbors(curr)) {
            int tentative_gScore = gScore[curr] + edge.weight;
            if (gScore.find(edge.target) == gScore.end() || tentative_gScore < gScore[edge.target]) {
                parent[edge.target] = curr;
                gScore[edge.target] = tentative_gScore;
                int fScore = tentative_gScore + graph.getHeuristic(edge.target, end);
                pq.push({ fScore, edge.target });
                if (observer) observer->onLog("Core: Node " + to_string(edge.target.id) + " fScore: " + to_string(fScore));
            }

        }
    }

    res.path = reconstructPathInternal(parent, start, end);
    if (res.success) {
        if (observer) observer->onLog("Path reconstruction complete.");
        res.totalCost = gScore[end];
    } else {
        if (observer) observer->onLog("Failure: No path could be found to target.");
    }

    auto endTime = chrono::high_resolution_clock::now();
    res.timeMs = chrono::duration<double, milli>(endTime - startTime).count();
    return res;
}
