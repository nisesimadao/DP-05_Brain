/*
 * TofuMental - Todo App for Sharp Brain PW-SH2 (Windows Embedded CE 6.0)
 * Philosophy: Nothing OS Industrial Minimalism + Apple HIG Precision.
 * Features: 8pt Grid, Dot-Matrix Headers, LiquidGlass Simulation.
 */

#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <vector>
#include <string>
#include <ctime>
#include <math.h>
#include <stdio.h>

// --- Windows CE Boilerplate ---
#ifndef WS_OVERLAPPEDWINDOW
#define WS_OVERLAPPEDWINDOW \
  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
#endif

#ifdef UNDER_CE
#if SW_MAXIMIZE != 12
#undef SW_MAXIMIZE
#define SW_MAXIMIZE 12
#endif
#ifndef _tWinMain
#define _tWinMain WinMain
#endif
#endif

#include <tchar.h>

// --- Design Constants (8pt Grid) ---
#define GRID_UNIT 8
#define MARGIN_X (GRID_UNIT * 3) // 24pt
#define MARGIN_Y (GRID_UNIT * 4) // 32pt
#define ITEM_HEIGHT (GRID_UNIT * 6) // 48pt
#define CORNER_RADIUS GRID_UNIT

// Colors
#define CLR_BG          RGB(0, 0, 0)
#define CLR_TEXT_PRI    RGB(255, 255, 255)
#define CLR_TEXT_SEC    RGB(160, 160, 160)
#define CLR_ACCENT      RGB(255, 255, 255)
#define CLR_GLASS_BORDER RGB(60, 60, 60)

struct Task {
    std::wstring title;
    bool completed;
    Task() : title(L""), completed(false) {}
    Task(const std::wstring& t) : title(t), completed(false) {}
};

enum AppMode { MODE_LIST, MODE_ADD };

// Globals
std::vector<Task> tasks;
int selectedIndex = 0;
AppMode currentMode = MODE_LIST;
RECT clientRect;
HFONT hFontMain = NULL;
HFONT hFontDot = NULL;

// --- Animation Globals ---
double visualScrollPos = 0.0;
double targetScrollPos = 0.0;
double startScrollPos = 0.0;
DWORD animStartTime = 0;
const int ANIM_DURATION = 350; // ms
bool isAnimating = false;

// --- Persistence ---

std::wstring GetAppDir() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring ws(path);
    size_t pos = ws.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        return ws.substr(0, pos + 1);
    }
    return L"";
}

void SaveTasks() {
    std::wstring path = GetAppDir() + L"tasks.txt";
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    // Write UTF-16LE BOM
    unsigned short bom = 0xFEFF;
    DWORD written;
    WriteFile(hFile, &bom, 2, &written, NULL);

    for (size_t i = 0; i < tasks.size(); ++i) {
        std::wstring line = tasks[i].title + L"|" + (tasks[i].completed ? L"1" : L"0") + L"\r\n";
        WriteFile(hFile, line.c_str(), (DWORD)(line.length() * sizeof(wchar_t)), &written, NULL);
    }
    CloseHandle(hFile);
}

void LoadTasks() {
    std::wstring path = GetAppDir() + L"tasks.txt";
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        // Default tasks
        tasks.push_back(Task(L"Eat Tofu"));
        tasks.push_back(Task(L"Stay Mental"));
        tasks.push_back(Task(L"Build PW-SH2 Apps"));
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize <= 2) {
        CloseHandle(hFile);
        return;
    }

    wchar_t* buffer = new wchar_t[fileSize / 2 + 1];
    DWORD read;
    ReadFile(hFile, buffer, fileSize, &read, NULL);
    CloseHandle(hFile);

    buffer[read / 2] = L'\0';
    wchar_t* start = buffer;
    if (*start == 0xFEFF) start++; // Skip BOM

    tasks.clear();
    wchar_t* line = wcstok(start, L"\r\n");
    while (line) {
        std::wstring ws(line);
        size_t sep = ws.find_last_of(L'|');
        if (sep != std::wstring::npos) {
            std::wstring title = ws.substr(0, sep);
            bool completed = (ws.substr(sep + 1, 1) == L"1");
            tasks.push_back(Task(title));
            tasks.back().completed = completed;
        }
        line = wcstok(NULL, L"\r\n");
    }
    delete[] buffer;
    targetScrollPos = visualScrollPos = 0.0;
}

// --- UI Helpers ---

void DrawDotMatrixChar(HDC hdc, int x, int y, TCHAR c, COLORREF color) {
    UNREFERENCED_PARAMETER(hdc);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(c);
    UNREFERENCED_PARAMETER(color);
}

void DrawRoundedRect(HDC hdc, RECT r, int radius, COLORREF borderCol, COLORREF fillCol) {
    HBRUSH hBrush = CreateSolidBrush(fillCol);
    HPEN hPen = CreatePen(PS_SOLID, 1, borderCol);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    RoundRect(hdc, r.left, r.top, r.right, r.bottom, radius * 2, radius * 2);

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

void InitApp() {
    LoadTasks();
}

// --- Animation Helper ---

void StartScrollAnimation(int newIdx, HWND hWnd) {
    if (tasks.empty()) return;
    
    double startPos = visualScrollPos;
    double targetPos = (double)newIdx;

    // Shortest path logic for infinite loop
    double diff = targetPos - startPos;
    int n = (int)tasks.size();
    if (diff > n / 2.0) targetPos -= n;
    else if (diff < -n / 2.0) targetPos += n;

    targetScrollPos = targetPos;
    startScrollPos = startPos;
    animStartTime = GetTickCount();
    isAnimating = true;
    SetTimer(hWnd, 1, 16, NULL); // ~60fps
}

// --- Window Procedure ---

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            InitApp();
            GetClientRect(hWnd, &clientRect);
            LOGFONT lfMain;
            memset(&lfMain, 0, sizeof(lfMain));
            lfMain.lfHeight = 22;
            lfMain.lfWeight = FW_SEMIBOLD;
            lfMain.lfQuality = ANTIALIASED_QUALITY;
            lstrcpy(lfMain.lfFaceName, TEXT("MS PGothic"));
            hFontMain = CreateFontIndirect(&lfMain);

            LOGFONT lfDot;
            memset(&lfDot, 0, sizeof(lfDot));
            lfDot.lfHeight = 14;
            lfDot.lfWeight = FW_NORMAL;
            lfDot.lfQuality = NONANTIALIASED_QUALITY;
            lfDot.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
            lstrcpy(lfDot.lfFaceName, TEXT("Courier New"));
            hFontDot = CreateFontIndirect(&lfDot);
            break;
        }

        case WM_TIMER: {
            if (isAnimating) {
                DWORD elapsed = GetTickCount() - animStartTime;
                double t = (double)elapsed / ANIM_DURATION;
                if (t >= 1.0) {
                    t = 1.0;
                    isAnimating = false;
                    KillTimer(hWnd, 1);
                    visualScrollPos = targetScrollPos;
                    // Normalize only at the very end
                    double n = (double)tasks.size();
                    while (visualScrollPos < 0) visualScrollPos += n;
                    while (visualScrollPos >= n) visualScrollPos -= n;
                    targetScrollPos = visualScrollPos;
                    // Ensure selectedIndex matches
                    selectedIndex = (int)round(visualScrollPos);
                    while (selectedIndex < 0) selectedIndex += (int)tasks.size();
                    while (selectedIndex >= (int)tasks.size()) selectedIndex -= (int)tasks.size();
                } else {
                    double easedT = 1.0 - pow(1.0 - t, 3);
                    visualScrollPos = startScrollPos + (targetScrollPos - startScrollPos) * easedT;
                }
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        }

        case WM_SIZE:
            GetClientRect(hWnd, &clientRect);
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case WM_LBUTTONDOWN: {
            if (tasks.empty()) break;
            int x = (int)(short)LOWORD(lParam);
            int y = (int)(short)HIWORD(lParam);
            
            RECT rect;
            GetClientRect(hWnd, &rect);
            int centerY = rect.bottom / 2;
            int spacing = ITEM_HEIGHT + 2;

            int dy = y - centerY;
            int slotOffset = (dy >= 0) ? (dy + spacing / 2) / spacing : (dy - spacing / 2) / spacing;
            
            // Logic: target exactly what was tapped visually (including lap)
            double newVisualTarget = visualScrollPos + slotOffset;
            int newIdx = ((int)round(newVisualTarget) % (int)tasks.size() + (int)tasks.size()) % (int)tasks.size();
            
            if (slotOffset == 0 && x >= MARGIN_X && x < MARGIN_X + GRID_UNIT * 5) {
                tasks[newIdx].completed = !tasks[newIdx].completed;
                SaveTasks();
                InvalidateRect(hWnd, NULL, FALSE);
            } else {
                selectedIndex = newIdx;
                // Directly animate to the visual location tapped
                targetScrollPos = newVisualTarget;
                startScrollPos = visualScrollPos;
                animStartTime = GetTickCount();
                isAnimating = true;
                SetTimer(hWnd, 1, 16, NULL);
            }
            return 0;
        }

        case WM_CHAR:
            if (currentMode == MODE_ADD) {
                if (wParam == VK_RETURN) {
                    currentMode = MODE_LIST;
                    SaveTasks();
                } else if (wParam == VK_BACK) {
                    if (!tasks[selectedIndex].title.empty()) {
                        tasks[selectedIndex].title.erase(tasks[selectedIndex].title.size() - 1);
                    }
                } else if (wParam >= 32) { // Printable characters
                    tasks[selectedIndex].title += (wchar_t)wParam;
                }
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            break;

        case WM_KEYDOWN:
            if (currentMode == MODE_LIST) {
                if (tasks.empty() && wParam != 'A' && wParam != VK_ESCAPE) break;
                switch (wParam) {
                    case VK_UP:
                        selectedIndex = (selectedIndex <= 0) ? (int)tasks.size() - 1 : selectedIndex - 1;
                        StartScrollAnimation(selectedIndex, hWnd);
                        break;
                    case VK_DOWN:
                        selectedIndex = (selectedIndex >= (int)tasks.size() - 1) ? 0 : selectedIndex + 1;
                        StartScrollAnimation(selectedIndex, hWnd);
                        break;
                    case VK_RETURN:
                        if (selectedIndex >= 0 && selectedIndex < (int)tasks.size()) {
                            tasks[selectedIndex].completed = !tasks[selectedIndex].completed;
                            SaveTasks();
                        }
                        InvalidateRect(hWnd, NULL, FALSE);
                        break;
                    case 'A': // Add
                        tasks.push_back(Task(L""));
                        selectedIndex = (int)tasks.size() - 1;
                        currentMode = MODE_ADD;
                        visualScrollPos = targetScrollPos = (double)selectedIndex; // Snap for add
                        InvalidateRect(hWnd, NULL, FALSE);
                        break;
                    case 'D': // Delete
                    case VK_BACK:
                        if (selectedIndex >= 0 && selectedIndex < (int)tasks.size()) {
                            tasks.erase(tasks.begin() + selectedIndex);
                            if (selectedIndex >= (int)tasks.size()) selectedIndex = (int)tasks.size() - 1;
                            if (selectedIndex < 0) selectedIndex = 0; // Handle case where all tasks are deleted
                            visualScrollPos = targetScrollPos = (double)selectedIndex;
                            SaveTasks();
                            InvalidateRect(hWnd, NULL, TRUE);
                        }
                        break;
                    case VK_ESCAPE:
                        PostQuitMessage(0);
                        break;
                }
            } else if (currentMode == MODE_ADD) {
                if (wParam == VK_ESCAPE) {
                    currentMode = MODE_LIST;
                    if (tasks[selectedIndex].title.empty()) {
                        tasks.erase(tasks.begin() + selectedIndex);
                        selectedIndex = (int)tasks.size() - 1;
                        if (selectedIndex < 0) selectedIndex = 0;
                    }
                    visualScrollPos = targetScrollPos = (double)selectedIndex;
                    InvalidateRect(hWnd, NULL, TRUE);
                }
            }
            break;

        case WM_ERASEBKGND:
            return 1; // Handled

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rect;
            GetClientRect(hWnd, &rect);
            if (rect.right == 0 || rect.bottom == 0) {
                EndPaint(hWnd, &ps);
                return 0;
            }

            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

            HBRUSH bgBrush = CreateSolidBrush(CLR_BG);
            FillRect(hdcMem, &rect, bgBrush);
            DeleteObject(bgBrush);

            SetBkMode(hdcMem, TRANSPARENT);

            if (!tasks.empty()) {
                int centerY = rect.bottom / 2;
                int spacing = ITEM_HEIGHT + 2;
                int halfItem = ITEM_HEIGHT / 2;
                
                double rangeJ = (double)rect.bottom / spacing / 2.0 + 1.5;
                int startJ = (int)floor(visualScrollPos - rangeJ);
                int endJ = (int)ceil(visualScrollPos + rangeJ);
                int n = (int)tasks.size();

                // Draw Stationary Focus Frame
                RECT focusRect = { MARGIN_X, centerY - halfItem, rect.right - MARGIN_X, centerY - halfItem + ITEM_HEIGHT };
                COLORREF highlightCol = (currentMode == MODE_ADD) ? RGB(60,20,20) : RGB(30,30,30);
                DrawRoundedRect(hdcMem, focusRect, CORNER_RADIUS, CLR_GLASS_BORDER, highlightCol);

                for (int j = startJ; j <= endJ; ++j) {
                    int i = (j % n + n) % n;
                    int itemTop = (int)(centerY - halfItem + (j - visualScrollPos) * spacing);
                    RECT itemRect = { MARGIN_X, itemTop, rect.right - MARGIN_X, itemTop + ITEM_HEIGHT };

                    // Seam Separator (between j=k*n-1 and j=k*n)
                    if ((j % n == n - 1) && n > 1) {
                        int seamY = itemTop + ITEM_HEIGHT + 1;
                        if (seamY > MARGIN_Y && seamY < rect.bottom - MARGIN_Y) {
                            HPEN hSeamPen = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));
                            HPEN hOldP = (HPEN)SelectObject(hdcMem, hSeamPen);
                            MoveToEx(hdcMem, MARGIN_X, seamY, NULL); LineTo(hdcMem, rect.right - MARGIN_X, seamY);
                            for (int dx = MARGIN_X; dx < rect.right - MARGIN_X; dx += 8) {
                                SetPixel(hdcMem, dx, seamY - 2, CLR_TEXT_SEC); SetPixel(hdcMem, dx, seamY + 2, CLR_TEXT_SEC);
                            }
                            SelectObject(hdcMem, hOldP); DeleteObject(hSeamPen);
                        }
                    }



                    if (itemTop + ITEM_HEIGHT < 0 || itemTop > rect.bottom) continue;

                    // Indicator
                    RECT checkRect = { itemRect.left + GRID_UNIT, itemRect.top + GRID_UNIT, 
                                       itemRect.left + GRID_UNIT * 3, itemRect.top + ITEM_HEIGHT - GRID_UNIT };
                    
                    if (tasks[i].completed) {
                        HBRUSH hBr = CreateSolidBrush(CLR_TEXT_PRI);
                        HBRUSH hOldB = (HBRUSH)SelectObject(hdcMem, hBr);
                        Ellipse(hdcMem, checkRect.left + 4, checkRect.top + 12, checkRect.right - 4, checkRect.bottom - 12);
                        SelectObject(hdcMem, hOldB);
                        DeleteObject(hBr);
                    } else {
                        HPEN hPen = CreatePen(PS_SOLID, 1, CLR_TEXT_SEC);
                        HPEN hOldP = (HPEN)SelectObject(hdcMem, hPen);
                        SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
                        Ellipse(hdcMem, checkRect.left + 4, checkRect.top + 12, checkRect.right - 4, checkRect.bottom - 12);
                        SelectObject(hdcMem, hOldP);
                        DeleteObject(hPen);
                    }

                    // Text
                    SelectObject(hdcMem, hFontMain);
                    // Fade based on distance from screen center
                    int distFromCenter = abs(itemTop + halfItem - centerY);
                    int alpha = 255 - (distFromCenter * 255 / centerY);
                    if (alpha < 40) alpha = 40;
                    if (alpha > 255) alpha = 255;

                    COLORREF baseCol = tasks[i].completed ? CLR_TEXT_SEC : CLR_TEXT_PRI;
                    COLORREF fadedCol = RGB(GetRValue(baseCol) * alpha / 255, GetGValue(baseCol) * alpha / 255, GetBValue(baseCol) * alpha / 255);
                    
                    // Focused: virtual index j is the one closest to visualScrollPos
                    bool isFocused = (j == (int)floor(visualScrollPos + 0.5));
                    SetTextColor(hdcMem, isFocused ? baseCol : fadedCol);

                    std::wstring displayText = tasks[i].title;
                    if (isFocused && currentMode == MODE_ADD && (GetTickCount() / 500) % 2 == 0) {
                        displayText += L"_";
                    }
                    RECT textRect = { itemRect.left + GRID_UNIT * 5, itemRect.top, itemRect.right, itemRect.bottom };
                    DrawText(hdcMem, displayText.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                }
            }

            // Header
            SelectObject(hdcMem, hFontDot);
            SetTextColor(hdcMem, CLR_TEXT_PRI);
            SetBkMode(hdcMem, TRANSPARENT);
            RECT headerRect = { MARGIN_X, MARGIN_Y / 2, rect.right - MARGIN_X, MARGIN_Y };
            DrawText(hdcMem, (currentMode == MODE_ADD) ? TEXT("::: NEW TASK :::") : TEXT("::: TOFU MENTAL :::"), -1, &headerRect, DT_LEFT | DT_BOTTOM);

            // Footer
            SelectObject(hdcMem, hFontDot);
            SetTextColor(hdcMem, CLR_TEXT_SEC);
            TCHAR footerText[64];
            wsprintf(footerText, TEXT("%s | ITEMS: %02d"), 
                (currentMode == MODE_ADD) ? TEXT("INPUT") : TEXT("DEFAULT"), tasks.size());
            RECT footerRect = { MARGIN_X, rect.bottom - 20, rect.right - MARGIN_X, rect.bottom - 5 };
            DrawText(hdcMem, footerText, -1, &footerRect, DT_RIGHT | DT_SINGLELINE);

            BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
            EndPaint(hWnd, &ps);
            break;
        }

        case WM_DESTROY:
            if (hFontMain) DeleteObject(hFontMain);
            if (hFontDot) DeleteObject(hFontDot);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    WNDCLASS wcl;
    wcl.hInstance = hInstance;
    wcl.lpszClassName = TEXT("TofuMental");
    wcl.lpfnWndProc = WindowProc;
    wcl.style = CS_HREDRAW | CS_VREDRAW;
    wcl.hIcon = NULL;
    wcl.hCursor = NULL;
    wcl.lpszMenuName = NULL;
    wcl.cbClsExtra = 0;
    wcl.cbWndExtra = 0;
    wcl.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    if (!RegisterClass(&wcl)) return FALSE;

#ifdef UNDER_CE
    HWND hWnd = CreateWindowEx(0, TEXT("TofuMental"), TEXT("TofuMental"), WS_POPUP | WS_VISIBLE,
                               0, 0, CW_USEDEFAULT, CW_USEDEFAULT, 
                               NULL, NULL, hInstance, NULL);
#else
    HWND hWnd = CreateWindowEx(0, TEXT("TofuMental"), TEXT("TofuMental"), WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, 480, 272, // Typical Sharp Brain resolution for testing
                               NULL, NULL, hInstance, NULL);
#endif

    if (!hWnd) return FALSE;

#ifdef UNDER_CE
    ShowWindow(hWnd, SW_MAXIMIZE);
#else
    ShowWindow(hWnd, nShowCmd);
#endif
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
