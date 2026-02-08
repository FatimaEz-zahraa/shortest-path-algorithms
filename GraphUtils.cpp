#include "GraphUtils.h"
#include <vector>
#include <iomanip>
#include <iostream>

using namespace std;

void printIncidenceMatrix(const Grid& grid) {
    int rows = grid.getHeight();
    int cols = grid.getWidth();
    int numNodes = rows * cols;
    
    // Collect edges
    vector<pair<int, int>> edges;
    
    // We treat every cell as a node. Obstacles are nodes with no connections.
    // Or we can treat obstacles as non-existent nodes.
    // Let's stick to: if a node or its neighbor is an obstacle, no edge exists.
    
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (grid.isObstacle(r, c)) continue;

            // Check Right
            if (c + 1 < cols && !grid.isObstacle(r, c + 1)) {
                edges.push_back({r * cols + c, r * cols + (c + 1)});
            }
            // Check Down
            if (r + 1 < rows && !grid.isObstacle(r + 1, c)) {
                edges.push_back({r * cols + c, (r + 1) * cols + c});
            }
        }
    }
    
    if (edges.empty()) {
        cout << "No edges in the graph.\n";
        return;
    }

    cout << "\nIncidence Matrix (" << numNodes << " nodes, " << edges.size() << " edges):\n";
    
    // Print Header
    cout << "Node\\Edge ";
    for(size_t j=0; j<edges.size(); ++j) {
        cout << "E" << j << " ";
        if (j < 10) cout << " "; 
    }
    cout << "\n";

    for(int i=0; i<numNodes; ++i) {
        cout << "N" << i;
        if (i < 10) cout << "        ";
        else if (i < 100) cout << "       ";
        else cout << "      ";

        for(size_t j=0; j<edges.size(); ++j) {
            int u = edges[j].first;
            int v = edges[j].second;
            
            if (i == u || i == v) cout << "1";
            else cout << "0";
            
            // Allocating space for next column
            cout << "   "; 
            if (j >= 10) cout << " ";
        }
        cout << "\n";
    }
}
