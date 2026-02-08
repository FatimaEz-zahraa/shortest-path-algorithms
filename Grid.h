#pragma once
#include "IGraph.h"
#include <vector>
#include <iostream>

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Point& other) const { return !(*this == other); }
    bool operator<(const Point& other) const { return x < other.x || (x == other.x && y < other.y); }
};

class Grid : public IGraph {
public:
    Grid(int height, int width);
    void setObstacle(int x, int y);
    void setSource(int x, int y);
    void setDestination(int x, int y);
    void setWeight(int x, int y, int weight);
    void setEmpty(int x, int y);
    void setVisited(int x, int y); 
    void setCurrent(int x, int y); 
    void generateRandomMaze();     
    void clearPath(); 
    void markPath(const std::vector<Point>& path);
    void print() const;
    std::string serialize() const;
    bool load(const std::string& data);
    bool isValid(int x, int y) const;
    bool isObstacle(int x, int y) const;
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    Point getSource() const { return source; }
    Point getDestination() const { return destination; }
    char getChar(int x, int y) const { return isValid(x,y) ? map[x][y] : '#'; }
    int getWeight(int x, int y) const { return isValid(x,y) ? weights[x][y] : 9999; }

    void setAllowDiagonals(bool allow) { m_allowDiagonals = allow; }
    bool getAllowDiagonals() const { return m_allowDiagonals; }

    // IGraph Implementation
    std::vector<Edge> getNeighbors(Node n) const override;
    int getHeuristic(Node start, Node target) const override;

    // Helpers to convert between Node and Grid coordinates
    Node toNode(int x, int y) const { return { x * width + y }; }
    Point toPoint(Node n) const { return { n.id / width, n.id % width }; }

private:
    int width, height;
    std::vector<std::vector<char>> map;
    std::vector<std::vector<int>> weights;
    Point source;
    Point destination;
    bool m_allowDiagonals = false;
};
