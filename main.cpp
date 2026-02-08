#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <commdlg.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex> // Fix missing mutex header
#include <algorithm> // For min/max
#include "Grid.h"
#include "Algorithms.h"
#include "GraphUtils.h"

// Global Grid
Grid* g_grid = nullptr;
const int CELL_SIZE = 25;
const int GRID_OFFSET_X = 20;
const int GRID_OFFSET_Y = 80; // Lowered to make room for controls
HWND g_hWnd = NULL;
HWND g_hCombo = NULL; // Algorithm selection
HWND g_hEditWeight = NULL; // Weight input

// Scroll globals
int g_scrollX = 0;
int g_scrollY = 0;
int g_maxScrollX = 0;
int g_maxScrollY = 0;
int g_viewWidth = 0;
int g_viewHeight = 0; // Height of the grid view area


// Button IDs
#define ID_BTN_SET_SOURCE 1
#define ID_BTN_SET_DEST 2
#define ID_BTN_GEN_MAZE 3
#define ID_BTN_RESET 5
#define ID_BTN_RUN 6
#define ID_COMBO_ALGO 7
#define ID_BTN_SET_WEIGHT 8
#define ID_CHK_DIAGONAL 9
#define ID_BTN_SAVE 10
#define ID_BTN_LOAD 11
#define ID_EDIT_WEIGHT 12

// Log Buffer
std::string g_logBuffer = "";
const int LOG_HEIGHT = 150;

bool g_algoRunning = false;
std::mutex g_gridMutex;

enum InteractionMode {
    MODE_OBSTACLE,
    MODE_SET_SOURCE,
    MODE_SET_DEST,
    MODE_SET_WEIGHT
};

InteractionMode g_mode = MODE_OBSTACLE;
const COLORREF COL_WEIGHT = RGB(139, 69, 19); // Brown for weighted areas

void LogToConsole(const std::string& msg) {
    std::cout << "[LOG] " << msg << std::endl;
}

// Aesthetic Colors
const COLORREF COL_BG = RGB(30, 30, 30);       // Dark Background
const COLORREF COL_GRID_LINE = RGB(50, 50, 50);
const COLORREF COL_EMPTY = RGB(40, 40, 40);    // Darker Empty
const COLORREF COL_OBSTACLE = RGB(200, 200, 200); // Light Grey
const COLORREF COL_SOURCE = RGB(46, 204, 113); // Emerald Green
const COLORREF COL_DEST = RGB(231, 76, 60);    // Alizarin Red
const COLORREF COL_PATH = RGB(241, 196, 15);   // Sunflower Yellow
const COLORREF COL_VISITED = RGB(52, 152, 219); // Peter River Blue
const COLORREF COL_CURRENT = RGB(155, 89, 182); // Amethyst Purple

void DrawGrid(HDC hdc) {
    if (!g_grid) return;
    std::lock_guard<std::mutex> lock(g_gridMutex);
    int rows = g_grid->getHeight();
    int cols = g_grid->getWidth();

    // Fill background
    RECT clientRect;
    GetClientRect(g_hWnd, &clientRect);
    HBRUSH bgBrush = CreateSolidBrush(COL_BG);
    FillRect(hdc, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    // Define Grid Viewport (between controls and log)
    // We only draw the grid within this area visually, but for simplicity
    // we'll just draw safely. logic assumes full client for now, but we offset.
    
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int x = GRID_OFFSET_X + c * CELL_SIZE - g_scrollX;
            int y = GRID_OFFSET_Y + r * CELL_SIZE - g_scrollY;
            
            // Culling: Don't draw if out of visible view (rough check)
            if (x + CELL_SIZE < 0 || y + CELL_SIZE < 80 || x > clientRect.right || y > clientRect.bottom) 
                continue;
            
            RECT rect = {x, y, x + CELL_SIZE, y + CELL_SIZE};
            
            HBRUSH brush = NULL;
            char type = g_grid->getChar(r, c);
            
            int weight = g_grid->getWeight(r, c);
            if (type == '.' && weight > 1) {
                brush = CreateSolidBrush(COL_WEIGHT);
            } else {
                switch (type) {
                    case '#': brush = CreateSolidBrush(COL_OBSTACLE); break;
                    case 'S': brush = CreateSolidBrush(COL_SOURCE); break;
                    case 'D': brush = CreateSolidBrush(COL_DEST); break;
                    case '*': brush = CreateSolidBrush(COL_PATH); break;
                    case 'v': brush = CreateSolidBrush(COL_VISITED); break;
                    case 'c': brush = CreateSolidBrush(COL_CURRENT); break;
                    default: brush = CreateSolidBrush(COL_EMPTY); break;
                }
            }
            
            FillRect(hdc, &rect, brush);

            // Draw Weight Text
            if (g_grid->getWeight(r, c) > 1 && type != '#') {
                int w = g_grid->getWeight(r, c);
                std::string wStr = std::to_string(w);
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(255, 255, 255)); // White text
                DrawTextA(hdc, wStr.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            
            // Draw Grid Lines
            HPEN pen = CreatePen(PS_SOLID, 1, COL_GRID_LINE);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            MoveToEx(hdc, x, y, NULL);
            LineTo(hdc, x + CELL_SIZE, y);
            LineTo(hdc, x + CELL_SIZE, y + CELL_SIZE);
            LineTo(hdc, x, y + CELL_SIZE);
            LineTo(hdc, x, y);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);

            DeleteObject(brush);
        }
    }
}

// Win32 Implementation of the Algorithm Observer for Visualization
class WindowsObserver : public IAlgorithmObserver {
    HWND m_hwnd;
    int m_updateCounter = 0;
public:
    WindowsObserver(HWND hwnd) : m_hwnd(hwnd) {}
    void onNodeVisited(Node n) override {
        if (g_grid) {
            std::lock_guard<std::mutex> lock(g_gridMutex);
            Point p = g_grid->toPoint(n);
            g_grid->setVisited(p.x, p.y);
            g_grid->setCurrent(p.x, p.y);
        }
        
        m_updateCounter++;
        // Update only every 10 nodes to reduce flickering and increase speed
        if (m_updateCounter % 10 == 0) {
            InvalidateRect(m_hwnd, NULL, FALSE);
            UpdateWindow(m_hwnd);
            // Very short sleep to keep UI responsive but fast
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    void onNodeCurrent(Node n) override {
        // Not used
    }
    void onLog(const std::string& msg) override {
        // Post message or directly log (if thread safe enough for Richedit/Edit)
        // SendMessage is blocking and should be safe from thread if window handles it
        // But better to delegate to main thread if possible, or just use our helper which uses SendMessage
        LogToConsole(msg); 
    }
};

WindowsObserver g_observer(NULL); // Initialize with NULL, set later

void RenderFrame() {
    InvalidateRect(g_hWnd, NULL, FALSE);
    UpdateWindow(g_hWnd);
    Sleep(20);
}

void HandleClick(int mx, int my) {
    if (!g_grid || g_algoRunning) return; 
    
    // Adjust mouse coords for scroll
    int x = mx + g_scrollX;
    int y = my + g_scrollY;

    int c = (x - GRID_OFFSET_X) / CELL_SIZE;
    int r = (y - GRID_OFFSET_Y) / CELL_SIZE;
    
    if (g_grid->isValid(r, c)) {
        std::lock_guard<std::mutex> lock(g_gridMutex); // Lock grid for modification
        if (g_mode == MODE_SET_SOURCE) {
            g_grid->setSource(r, c);
            g_mode = MODE_OBSTACLE; 
            LogToConsole("Source set.");
        } else if (g_mode == MODE_SET_DEST) {
            g_grid->setDestination(r, c);
            g_mode = MODE_OBSTACLE;
            LogToConsole("Destination set.");
        } else if (g_mode == MODE_SET_WEIGHT) {
            
            // Get weight from Input
            int newWeight = 5;
            char buf[32];
            GetWindowTextA(g_hEditWeight, buf, 32);
            int val = atoi(buf);
            if (val > 0) newWeight = val;
            
            // Set weight logic
            g_grid->setWeight(r, c, newWeight);
            LogToConsole("Cell (" + std::to_string(r) + "," + std::to_string(c) + ") weight set to " + std::to_string(newWeight));

        } else {
            g_grid->setObstacle(r, c); 
            LogToConsole("Obstacle placed.");
        }
        InvalidateRect(g_hWnd, NULL, TRUE);
    }
}

void UpdateScrollBars(HWND hwnd) {
    if (!g_grid) return;
    
    RECT rc;
    GetClientRect(hwnd, &rc);
    
    // Grid total dimensions
    int totalW = g_grid->getWidth() * CELL_SIZE + GRID_OFFSET_X * 2; // + padding
    int totalH = g_grid->getHeight() * CELL_SIZE + GRID_OFFSET_Y + 50; // + padding
    
    // Viewport dimensions
    int viewW = rc.right;
    int viewH = rc.bottom; 
    if (viewH < 0) viewH = 0;
    
    g_viewWidth = viewW;
    g_viewHeight = viewH;

    // Horizontal
    g_maxScrollX = std::max(0, totalW - viewW);
    g_scrollX = std::min(g_scrollX, g_maxScrollX);
    SCROLLINFO si = { sizeof(si), SIF_ALL | SIF_DISABLENOSCROLL, 0, totalW, (UINT)viewW, g_scrollX, 0 };
    SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

    // Vertical
    g_maxScrollY = std::max(0, totalH - viewH);
    g_scrollY = std::min(g_scrollY, g_maxScrollY);
    si = { sizeof(si), SIF_ALL | SIF_DISABLENOSCROLL, 0, totalH, (UINT)viewH, g_scrollY, 0 };
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            // Set the HWND for the observer after the window is created
            g_observer = WindowsObserver(hwnd);

            CreateWindow(L"BUTTON", L"Set Source", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                20, 10, 100, 30, hwnd, (HMENU)ID_BTN_SET_SOURCE, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            CreateWindow(L"BUTTON", L"Set Dest", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                130, 10, 100, 30, hwnd, (HMENU)ID_BTN_SET_DEST, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            CreateWindow(L"BUTTON", L"Random Maze", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                240, 10, 120, 30, hwnd, (HMENU)ID_BTN_GEN_MAZE, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            
            // Dropdown
            g_hCombo = CreateWindow(L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
                380, 10, 120, 200, hwnd, (HMENU)ID_COMBO_ALGO, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            SendMessage(g_hCombo, CB_ADDSTRING, 0, (LPARAM)L"Dijkstra");
            SendMessage(g_hCombo, CB_ADDSTRING, 0, (LPARAM)L"BFS");
            SendMessage(g_hCombo, CB_ADDSTRING, 0, (LPARAM)L"A* (A-Star)");
            SendMessage(g_hCombo, CB_SETCURSEL, 0, 0); // Default Dijkstra

            CreateWindow(L"BUTTON", L"RUN", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                510, 10, 80, 30, hwnd, (HMENU)ID_BTN_RUN, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

            CreateWindow(L"BUTTON", L"Weights", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                690, 10, 80, 30, hwnd, (HMENU)ID_BTN_SET_WEIGHT, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

            // Weight Edit
            g_hEditWeight = CreateWindow(L"EDIT", L"5", WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
                690, 45, 80, 20, hwnd, (HMENU)ID_EDIT_WEIGHT, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

            UpdateScrollBars(hwnd);

            CreateWindow(L"BUTTON", L"Diagonals", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                780, 10, 100, 30, hwnd, (HMENU)ID_CHK_DIAGONAL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

            CreateWindow(L"BUTTON", L"Save Grid", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                20, 50, 100, 30, hwnd, (HMENU)ID_BTN_SAVE, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            CreateWindow(L"BUTTON", L"Load Grid", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                130, 50, 100, 30, hwnd, (HMENU)ID_BTN_LOAD, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            CreateWindow(L"BUTTON", L"Reset", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                240, 50, 100, 30, hwnd, (HMENU)ID_BTN_RESET, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            break;

        case WM_COMMAND:
            if (g_algoRunning && LOWORD(wParam) != ID_BTN_RUN) { // Prevent most interactions while algo is running
                LogToConsole("Algorithm is running, please wait.");
                break;
            }
            switch (LOWORD(wParam)) {
                case ID_BTN_SET_SOURCE:
                    g_mode = MODE_SET_SOURCE;
                    LogToConsole("Mode: Set Source");
                    break;
                case ID_BTN_SET_DEST:
                    g_mode = MODE_SET_DEST;
                    LogToConsole("Mode: Set Destination");
                    break;
                case ID_BTN_SET_WEIGHT:
                    g_mode = MODE_SET_WEIGHT;
                    LogToConsole("Mode: Set Weight (Click cells to toggle weight)");
                    break;
                case ID_BTN_GEN_MAZE:
                    {
                        std::lock_guard<std::mutex> lock(g_gridMutex);
                        g_grid->generateRandomMaze();
                    }
                    LogToConsole("Random Maze Generated.");
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_CHK_DIAGONAL:
                    if (g_grid) {
                        std::lock_guard<std::mutex> lock(g_gridMutex);
                        LRESULT chkState = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                        g_grid->setAllowDiagonals(chkState == BST_CHECKED);
                        LogToConsole(chkState == BST_CHECKED ? "Diagonals Enabled" : "Diagonals Disabled");
                    }
                    break;
                case ID_BTN_RESET:
                    {
                        std::lock_guard<std::mutex> lock(g_gridMutex);
                        delete g_grid;
                        g_grid = new Grid(20, 30); 
                    }
                    LogToConsole("Grid Reset.");
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_BTN_SAVE:
                    {
                        OPENFILENAME ofn;
                        wchar_t szFile[260] = {0};
                        ZeroMemory(&ofn, sizeof(ofn));
                        ofn.lStructSize = sizeof(ofn);
                        ofn.hwndOwner = hwnd;
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = sizeof(szFile);
                        ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
                        ofn.nFilterIndex = 1;
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;
                        if (GetSaveFileName(&ofn)) {
                            std::lock_guard<std::mutex> lock(g_gridMutex);
                            std::string data = g_grid->serialize();
                            std::ofstream out(szFile);
                            out << data;
                            LogToConsole("Grid Saved.");
                        }
                    }
                    break;
                case ID_BTN_LOAD:
                    {
                        OPENFILENAME ofn;
                        wchar_t szFile[260] = {0};
                        ZeroMemory(&ofn, sizeof(ofn));
                        ofn.lStructSize = sizeof(ofn);
                        ofn.hwndOwner = hwnd;
                        ofn.lpstrFile = szFile;
                        ofn.nMaxFile = sizeof(szFile);
                        ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
                        ofn.nFilterIndex = 1;
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                        if (GetOpenFileName(&ofn)) {
                            std::ifstream in(szFile);
                            std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                            std::lock_guard<std::mutex> lock(g_gridMutex);
                            if (g_grid->load(data)) {
                                LogToConsole("Grid Loaded.");
                                InvalidateRect(hwnd, NULL, TRUE);
                            } else {
                                LogToConsole("Error loading grid file.");
                            }
                        }
                    }
                    break;
                case ID_BTN_RUN:
                    if (g_algoRunning) {
                        LogToConsole("Algorithm is already running.");
                        break;
                    }
                    {
                        std::lock_guard<std::mutex> lock(g_gridMutex); // Lock grid for initial clear
                        g_grid->clearPath();
                    }
                    InvalidateRect(hwnd, NULL, TRUE); // Clear path visually
                    
                    int algoIdx = SendMessage(g_hCombo, CB_GETCURSEL, 0, 0);
                    
                        std::thread([algoIdx, hwnd]() {
                            g_algoRunning = true;
                            Node startNode, endNode;
                            {
                                std::lock_guard<std::mutex> lock(g_gridMutex);
                                startNode = g_grid->toNode(g_grid->getSource().x, g_grid->getSource().y);
                                endNode = g_grid->toNode(g_grid->getDestination().x, g_grid->getDestination().y);
                            }
                            
                            AlgoResult res;
                            if (algoIdx == 0) res = runDijkstra(*g_grid, startNode, endNode, &g_observer);
                            else if (algoIdx == 1) res = runBFS(*g_grid, startNode, endNode, &g_observer);
                            else res = runAStar(*g_grid, startNode, endNode, &g_observer);
                            
                            {
                                std::lock_guard<std::mutex> lock(g_gridMutex);
                                if (res.success) {
                                    // Make sure to convert Node path to Point path for Grid
                                    std::vector<Point> points;
                                    for(const auto& n : res.path) {
                                        points.push_back(g_grid->toPoint(n));
                                    }
                                    g_grid->markPath(points);
                                }
                            }
                            g_algoRunning = false;
                            InvalidateRect(hwnd, NULL, TRUE);
                            
                            if (res.success) {
                                std::string msg = "Path Found! Cost: " + std::to_string(res.totalCost);
                                MessageBoxA(hwnd, msg.c_str(), "Done", MB_OK | MB_ICONINFORMATION);
                            } else {
                                MessageBoxA(hwnd, "No path found.", "Done", MB_OK | MB_ICONWARNING);
                            }
                        }).detach();
                    break;
            }
            break;

        case WM_LBUTTONDOWN:
            HandleClick(LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_ERASEBKGND:
            return 1; // Prevent flickering handling in Paint

        case WM_SIZE:
             {
                 // Update Log Window Position
                 // Removed Log Window
                 UpdateScrollBars(hwnd);
             }
             break;

        case WM_VSCROLL:
            {
                SCROLLINFO si = { sizeof(si), SIF_ALL };
                GetScrollInfo(hwnd, SB_VERT, &si);
                int oldY = g_scrollY;
                switch (LOWORD(wParam)) {
                    case SB_TOP: g_scrollY = 0; break;
                    case SB_BOTTOM: g_scrollY = g_maxScrollY; break;
                    case SB_LINEUP: g_scrollY -= 10; break;
                    case SB_LINEDOWN: g_scrollY += 10; break;
                    case SB_PAGEUP: g_scrollY -= g_viewHeight; break;
                    case SB_PAGEDOWN: g_scrollY += g_viewHeight; break;
                    case SB_THUMBTRACK: g_scrollY = si.nTrackPos; break;
                }
                g_scrollY = std::max(0, std::min(g_scrollY, g_maxScrollY));
                if (g_scrollY != oldY) {
                    SetScrollPos(hwnd, SB_VERT, g_scrollY, TRUE);
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            break;

        case WM_HSCROLL:
             {
                SCROLLINFO si = { sizeof(si), SIF_ALL };
                GetScrollInfo(hwnd, SB_HORZ, &si);
                int oldX = g_scrollX;
                switch (LOWORD(wParam)) {
                    case SB_LEFT: g_scrollX = 0; break;
                    case SB_RIGHT: g_scrollX = g_maxScrollX; break;
                    case SB_LINELEFT: g_scrollX -= 10; break;
                    case SB_LINERIGHT: g_scrollX += 10; break;
                    case SB_PAGELEFT: g_scrollX -= g_viewWidth; break;
                    case SB_PAGERIGHT: g_scrollX += g_viewWidth; break;
                    case SB_THUMBTRACK: g_scrollX = si.nTrackPos; break;
                }
                g_scrollX = std::max(0, std::min(g_scrollX, g_maxScrollX));
                if (g_scrollX != oldX) {
                    SetScrollPos(hwnd, SB_HORZ, g_scrollX, TRUE);
                    InvalidateRect(hwnd, NULL, TRUE);
                }
             }
             break;

        case WM_MOUSEWHEEL:
            {
                int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                int scrollLines = 3; 
                int amount = (zDelta / 120) * CELL_SIZE * scrollLines;
                
                g_scrollY -= amount;
                g_scrollY = std::max(0, std::min(g_scrollY, g_maxScrollY));
                
                SetScrollPos(hwnd, SB_VERT, g_scrollY, TRUE);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                
                // Double buffering to reduce flicker
                RECT rc;
                GetClientRect(hwnd, &rc);
                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
                SelectObject(memDC, memBM);
                
                DrawGrid(memDC);
                
                BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
                
                DeleteObject(memBM);
                DeleteDC(memDC);
                
                EndPaint(hwnd, &ps);
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main() {
    std::cout << "Console Initialized. Actions will be logged here.\n";
    HINSTANCE hInstance = GetModuleHandle(NULL);

    g_grid = new Grid(20, 30); // Default size 20x30

    const wchar_t CLASS_NAME[] = L"DijkstraGridClass";
    
    WNDCLASS wc = { };
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // Dark background default

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Dijkstra & BFS Visualization - High Performance",
        WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        std::cout << "Window Creation Failed: " << GetLastError() << std::endl;
        return 0;
    }

    g_hWnd = hwnd;
    ShowWindow(hwnd, SW_SHOW);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    delete g_grid;
    return 0;
}
