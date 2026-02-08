#include "Grid.h"
#include <iomanip>

Grid::Grid(int height, int width) : width(width), height(height), source({0, 0}), destination({height-1, width-1}) {
    map.resize(height, std::vector<char>(width, '.'));
    weights.resize(height, std::vector<int>(width, 1)); // Default weight 1
    if (isValid(source.x, source.y)) map[source.x][source.y] = 'S';
    if (isValid(destination.x, destination.y)) map[destination.x][destination.y] = 'D';
}

void Grid::setWeight(int x, int y, int weight) {
    if (isValid(x, y)) {
        weights[x][y] = weight;
        // If it's a wall or visited, make it a normal path so weight applies
        if (map[x][y] == '#' || map[x][y] == '*' || map[x][y] == 'v') {
            map[x][y] = '.';
        }
    }
}

void Grid::setEmpty(int x, int y) {
    if (isValid(x, y) && map[x][y] != 'S' && map[x][y] != 'D') {
        map[x][y] = '.';
        weights[x][y] = 1;
    }
}

void Grid::setObstacle(int x, int y) {
    if (isValid(x, y)) {
        map[x][y] = '#';
    }
}

void Grid::setSource(int x, int y) {
    if (isValid(x, y)) {
        if (isValid(source.x, source.y)) map[source.x][source.y] = '.'; 
        source = {x, y};
        map[x][y] = 'S';
    }
}

void Grid::setDestination(int x, int y) {
    if (isValid(x, y)) {
        if (isValid(destination.x, destination.y)) map[destination.x][destination.y] = '.'; 
        destination = {x, y};
        map[x][y] = 'D';
    }
}

void Grid::clearPath() {
    for(int i=0; i<height; ++i) {
        for(int j=0; j<width; ++j) {
            // Clear path, visited, current
            if(map[i][j] == '*' || map[i][j] == 'v' || map[i][j] == 'c') map[i][j] = '.';
        }
    }
    if(isValid(source.x, source.y)) map[source.x][source.y] = 'S';
    if(isValid(destination.x, destination.y)) map[destination.x][destination.y] = 'D';
}

void Grid::markPath(const std::vector<Point>& path) {
    for (const auto& p : path) {
        if (map[p.x][p.y] != 'S' && map[p.x][p.y] != 'D') {
            map[p.x][p.y] = '*';
        }
    }
}

void Grid::print() const {
    std::cout << "  ";
    for(int j=0; j<width; ++j) std::cout << j % 10 << " ";
    std::cout << "\n";
    
    for (int i = 0; i < height; ++i) {
        std::cout << i % 10 << " ";
        for (int j = 0; j < width; ++j) {
            std::cout << map[i][j] << " ";
        }
        std::cout << "\n";
    }
}

bool Grid::isValid(int x, int y) const {
    return x >= 0 && x < height && y >= 0 && y < width;
}

bool Grid::isObstacle(int x, int y) const {
    if (!isValid(x, y)) return true;
    return map[x][y] == '#';
}

void Grid::setVisited(int x, int y) {
    if (isValid(x, y) && map[x][y] != 'S' && map[x][y] != 'D' && map[x][y] != '#') {
        map[x][y] = 'v'; // visited
    }
}

void Grid::setCurrent(int x, int y) {
    if (isValid(x, y) && map[x][y] != 'S' && map[x][y] != 'D') {
        map[x][y] = 'c'; // current head
    }
}

#include <cstdlib>
#include <ctime>

void Grid::generateRandomMaze() {
    // Simple random maze: 30% obstacles
    srand(time(0));
    for(int i=0; i<height; ++i) {
        for(int j=0; j<width; ++j) {
            if (map[i][j] != 'S' && map[i][j] != 'D') {
                if ((rand() % 100) < 30) {
                    map[i][j] = '#';
                } else {
                    map[i][j] = '.';
                }
            }
        }
    }
}

std::vector<Edge> Grid::getNeighbors(Node n) const {
    Point p = toPoint(n);
    std::vector<Edge> neighbors;
    
    // Orthogonal (Cost 10)
    const int dx[] = {-1, 1, 0, 0};
    const int dy[] = {0, 0, -1, 1};
    for (int i = 0; i < 4; ++i) {
        int nx = p.x + dx[i];
        int ny = p.y + dy[i];
        if (isValid(nx, ny) && !isObstacle(nx, ny)) {
            neighbors.push_back({ toNode(nx, ny), 10 * getWeight(nx, ny) });
        }
    }

    // Diagonal (Cost 14)
    if (m_allowDiagonals) {
        const int ddx[] = {-1, -1, 1, 1};
        const int ddy[] = {-1, 1, -1, 1};
        for (int i = 0; i < 4; ++i) {
            int nx = p.x + ddx[i];
            int ny = p.y + ddy[i];
            if (isValid(nx, ny) && !isObstacle(nx, ny)) {
                neighbors.push_back({ toNode(nx, ny), 14 * getWeight(nx, ny) });
            }
        }
    }
    return neighbors;
}

int Grid::getHeuristic(Node startNode, Node targetNode) const {
    Point s = toPoint(startNode);
    Point t = toPoint(targetNode);
    int dx = abs(s.x - t.x);
    int dy = abs(s.y - t.y);

    if (m_allowDiagonals) {
        // Octile distance (scaled by 10)
        return 10 * (dx + dy) + (14 - 2 * 10) * std::min(dx, dy);
    } else {
        // Manhattan distance (scaled by 10)
        return 10 * (dx + dy);
    }
}

#include <sstream>

std::string Grid::serialize() const {
    std::stringstream ss;
    ss << height << "," << width << "," 
       << source.x << "," << source.y << "," 
       << destination.x << "," << destination.y << "|";
    
    // Map data
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            ss << map[i][j];
        }
    }
    ss << "|";

    // Weight data
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            ss << weights[i][j] << " ";
        }
    }
    
    return ss.str();
}

bool Grid::load(const std::string& data) {
    try {
        size_t pipe1 = data.find('|');
        size_t pipe2 = data.find('|', pipe1 + 1);
        if (pipe1 == std::string::npos || pipe2 == std::string::npos) return false;

        std::string header = data.substr(0, pipe1);
        std::string mapData = data.substr(pipe1 + 1, pipe2 - pipe1 - 1);
        std::string weightData = data.substr(pipe2 + 1);

        std::stringstream ss(header);
        char comma;
        int h, w, sx, sy, dx, dy;
        if (!(ss >> h >> comma >> w >> comma >> sx >> comma >> sy >> comma >> dx >> comma >> dy)) return false;

        // Resize
        height = h;
        width = w;
        map.assign(height, std::vector<char>(width, '.'));
        weights.assign(height, std::vector<int>(width, 1));

        // Map
        if (mapData.length() < (size_t)(height * width)) return false;
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                map[i][j] = mapData[i * width + j];
            }
        }

        // Weights
        std::stringstream wss(weightData);
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                if (!(wss >> weights[i][j])) break;
            }
        }

        source = {sx, sy};
        destination = {dx, dy};
        return true;
    } catch (...) { return false; }
}
