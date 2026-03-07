/*
 * DP-05 "Measurement Device Dashboard" for Sharp Brain PW-SH2 (Windows
 * Embedded CE 6.0) Ported from Next.js project. Features: Borderless UI,
 * Settings Menu, Clock, Calendar, Time Remaining, Dictionary, Font Selection,
 * Accent Colors.
 */

#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif

#include "Dictionary.h"
#include <cstdlib>
#include <ctime>
#include <initguid.h>
#include <math.h>
#include <string>
#include <tchar.h>
#include <vector>
#include <wctype.h>
#include <windows.h>

// --- Constants ---
#define ID_TIMER_CLOCK 1
#define ID_TIMER_DRIFT 2
#define ID_TIMER_TRANSITION 4
#define ID_TIMER_VIEW_TRANSITION 8
#define TIMER_CLOCK_MS 1000
#define TIMER_DRIFT_MS 60000
const int TRANSITION_STEP_MS = 20;
const float TRANSITION_SPEED = 0.15f;

// --- UI Globals ---
RECT clientRect;
HFONT hFontClock = NULL;
HFONT hFontMain = NULL;
HFONT hFontSmall = NULL;
HFONT hFontTiny = NULL;
HFONT hFontUI = NULL;
HFONT hFontJp = NULL; // Shikakufuto for Japanese

TCHAR appPath[MAX_PATH];

// Colors
COLORREF COL_BG = RGB(242, 242, 242);
COLORREF COL_TEXT = RGB(17, 17, 17);
COLORREF COL_ACCENT = RGB(17, 17, 17);
COLORREF COL_PLATE_HI = RGB(255, 255, 255);
COLORREF COL_PLATE_SHADOW = RGB(180, 180, 180);

// State
bool nightMode = false;
bool burninGuard = true;
bool isSplashing = true;
DWORD splashStartTime = 0;
int fontWeight = FW_BOLD;

// Transition State
bool settingsOpen = false;
bool g_isTransitioning = false;
bool g_targetSettingsState = false;
float g_transitionPos = 0.0f;
float g_startTransitionPos = 0.0f;
DWORD g_animStartTime = 0;
const int ANIM_DURATION = 350; // ms

enum ViewMode { VIEW_DASHBOARD, VIEW_CLOCK, VIEW_TODO, VIEW_DICTIONARY };
ViewMode g_currentView = VIEW_DASHBOARD;
ViewMode g_targetView = VIEW_DASHBOARD;
float g_viewTransitionPos = 0.0f; // 0.0 to 1.0
int g_viewDirection = 1;          // 1 for Right, -1 for Left
bool g_isViewTransitioning = false;
DWORD g_viewAnimStartTime = 0;
int g_viewFocus = 0; // Focus within a view (e.g., todo index)

int driftX = 0;
int driftY = 0;
int driftCycle = 0;
int settingsFocus = 0;

HBITMAP hDashCache = NULL;
HBITMAP hSettCache = NULL;

// Accents
struct AccentInfo {
  const TCHAR *name;
  COLORREF color;
};

AccentInfo g_accents[] = {{_T("GREEN"), RGB(46, 219, 106)},
                          {_T("AMBER"), RGB(255, 176, 0)},
                          {_T("BLUE"), RGB(0, 163, 255)},
                          {_T("WHITE"), RGB(240, 240, 240)},
                          {_T("RED"), RGB(255, 69, 58)}};
int g_accentCount = sizeof(g_accents) / sizeof(g_accents[0]);
int g_selectedAccent = 0;

// Font Selection
struct FontInfo {
  const TCHAR *fileName;
  const TCHAR *faceName;
  const TCHAR *displayName;
};

FontInfo g_fonts[] = {
    {_T("teno3x6.ttf"), _T("TenoText 3x6"), _T("3x6")},
    {_T("teno8x10.ttf"), _T("TenoText 8x10"), _T("8x10")},
    {_T("teno8x11.ttf"), _T("TenoText 8x11(+Extended ASCII)"), _T("8x11")},
    {_T("cp_period.ttf"), _T("CP period"), _T("PERIOD")}};
const int g_fontCount = sizeof(g_fonts) / sizeof(g_fonts[0]);
int g_selectedClockFont = 1; // Default 8x10
int g_selectedMainFont = 3;  // Default CP period

struct TodoItem {
  std::wstring title;
  bool completed;
};

// Data
const TCHAR *monthNames[] = {_T("JAN"), _T("FEB"), _T("MAR"), _T("APR"),
                             _T("MAY"), _T("JUN"), _T("JUL"), _T("AUG"),
                             _T("SEP"), _T("OCT"), _T("NOV"), _T("DEC")};
std::vector<WordEntry> dictionary;
WordEntry currentWord;
std::vector<TodoItem> todos;

// --- Utils ---
HBITMAP LoadBMPFromFile(const TCHAR *filename) {
  HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    return NULL;
  BITMAPFILEHEADER bfh;
  DWORD read;
  ReadFile(hFile, &bfh, sizeof(bfh), &read, NULL);
  if (bfh.bfType != 0x4D42) {
    CloseHandle(hFile);
    return NULL;
  }
  BITMAPINFOHEADER bih;
  ReadFile(hFile, &bih, sizeof(bih), &read, NULL);
  DWORD size = bfh.bfSize - bfh.bfOffBits;
  if (size == 0) {
    CloseHandle(hFile);
    return NULL;
  }
  void *bits = malloc(size);
  if (!bits) {
    CloseHandle(hFile);
    return NULL;
  }
  SetFilePointer(hFile, bfh.bfOffBits, NULL, FILE_BEGIN);
  if (!ReadFile(hFile, bits, size, &read, NULL) || read != size) {
    free(bits);
    CloseHandle(hFile);
    return NULL;
  }
  CloseHandle(hFile);
  HDC hdc = GetDC(NULL);
  void *ppvBits;
  HBITMAP hbm = CreateDIBSection(hdc, (BITMAPINFO *)&bih, DIB_RGB_COLORS,
                                 &ppvBits, NULL, 0);
  if (hbm)
    memcpy(ppvBits, bits, size);
  ReleaseDC(NULL, hdc);
  free(bits);
  return hbm;
}

void PathJoin(TCHAR *out, const TCHAR *base, const TCHAR *file) {
  if (!out || !base || !file)
    return;
  _tcscpy(out, base);
  size_t len = _tcslen(out);
  if (len > 0 && out[len - 1] != _T('\\') && out[len - 1] != _T('/')) {
    _tcscat(out, _T("\\"));
  }
  _tcscat(out, file);
}

void GetDeviceString(TCHAR *out, int maxLen) {
#ifdef _WIN32_WCE
  TCHAR platform[64] = {0};
  if (SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(platform), platform,
                           0)) {
    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(osv);
    GetVersionEx(&osv);
    _sntprintf(out, maxLen, _T("%s (CE %d.%d)"), platform, osv.dwMajorVersion,
               osv.dwMinorVersion);
  } else {
    _sntprintf(out, maxLen, _T("WinCE Device"));
  }
#else
  TCHAR name[MAX_COMPUTERNAME_LENGTH + 1] = {0};
  DWORD nSize = MAX_COMPUTERNAME_LENGTH + 1;
  if (GetComputerName(name, &nSize)) {
    _sntprintf(out, maxLen, _T("%s"), name);
  } else {
    _sntprintf(out, maxLen, _T("Windows PC"));
  }
#endif
}

void SaveSettings() {
  TCHAR path[MAX_PATH];
  PathJoin(path, appPath, _T("DP05_Settings.cfg"));
  FILE *f = _tfopen(path, _T("w"));
  if (f) {
    _ftprintf(f, _T("nightMode=%d\n"), nightMode ? 1 : 0);
    _ftprintf(f, _T("burninGuard=%d\n"), burninGuard ? 1 : 0);
    _ftprintf(f, _T("selectedAccent=%d\n"), g_selectedAccent);
    _ftprintf(f, _T("selectedClockFont=%d\n"), g_selectedClockFont);
    _ftprintf(f, _T("selectedMainFont=%d\n"), g_selectedMainFont);
    fclose(f);
  }
}

void LoadSettings() {
  TCHAR path[MAX_PATH];
  PathJoin(path, appPath, _T("DP05_Settings.cfg"));
  FILE *f = _tfopen(path, _T("r"));
  if (f) {
    TCHAR line[256];
    while (_fgetts(line, 256, f)) {
      int val = 0;
      if (_tcsncmp(line, _T("nightMode="), 10) == 0) {
        _stscanf(line + 10, _T("%d"), &val);
        nightMode = (val != 0);
      } else if (_tcsncmp(line, _T("burninGuard="), 12) == 0) {
        _stscanf(line + 12, _T("%d"), &val);
        burninGuard = (val != 0);
      } else if (_tcsncmp(line, _T("selectedAccent="), 15) == 0) {
        _stscanf(line + 15, _T("%d"), &g_selectedAccent);
      } else if (_tcsncmp(line, _T("selectedClockFont="), 18) == 0) {
        _stscanf(line + 18, _T("%d"), &g_selectedClockFont);
      } else if (_tcsncmp(line, _T("selectedMainFont="), 17) == 0) {
        _stscanf(line + 17, _T("%d"), &g_selectedMainFont);
      }
    }
    fclose(f);
  }
}

void UpdateColors() {
  if (nightMode) {
    COL_BG = RGB(11, 11, 12);
    COL_TEXT = g_accents[g_selectedAccent].color;
    COL_ACCENT = g_accents[g_selectedAccent].color;
    COL_PLATE_HI = RGB(30, 30, 30);
    COL_PLATE_SHADOW = RGB(0, 0, 0);
  } else {
    COL_BG = RGB(242, 242, 242);
    COL_TEXT = RGB(17, 17, 17);
    COL_ACCENT = RGB(100, 100, 100);
    COL_PLATE_HI = RGB(255, 255, 255);
    COL_PLATE_SHADOW = RGB(180, 180, 180);
  }
}

HFONT CreateFontSimple(int height, int weight, const TCHAR *face) {
  LOGFONT lf;
  memset(&lf, 0, sizeof(LOGFONT));
  lf.lfHeight = height;
  lf.lfWeight = weight;
  lf.lfCharSet = DEFAULT_CHARSET;
  _tcsncpy(lf.lfFaceName, face, LF_FACESIZE);
  return CreateFontIndirect(&lf);
}

void RefreshFonts() {
  if (hFontClock)
    DeleteObject(hFontClock);
  if (hFontMain)
    DeleteObject(hFontMain);
  if (hFontSmall)
    DeleteObject(hFontSmall);
  if (hFontTiny)
    DeleteObject(hFontTiny);
  if (hFontUI)
    DeleteObject(hFontUI);
  if (hFontJp)
    DeleteObject(hFontJp);

  const TCHAR *clockFace = g_fonts[g_selectedClockFont].faceName;
  const TCHAR *mainFace = g_fonts[g_selectedMainFont].faceName;
  const TCHAR *jpFace = _T("CP period");

  hFontClock = CreateFontSimple(80, fontWeight, clockFace);
  hFontMain = CreateFontSimple(24, fontWeight, mainFace);
  hFontSmall = CreateFontSimple(16, FW_NORMAL, mainFace);
  hFontTiny = CreateFontSimple(12, FW_NORMAL, mainFace);
  hFontUI = CreateFontSimple(18, FW_BOLD, mainFace);
  hFontJp = CreateFontSimple(20, FW_NORMAL, jpFace);
}

void DrawPlate(HDC hdc, RECT r, bool convex, int radius = 12) {
  HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, CreateSolidBrush(COL_BG));
  HPEN hOldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
  int dx = burninGuard ? driftX : 0;
  int dy = burninGuard ? driftY : 0;
  RoundRect(hdc, r.left + dx, r.top + dy, r.right + dx, r.bottom + dy, radius,
            radius);
  HPEN hPenHi = CreatePen(PS_SOLID, 1, COL_PLATE_HI);
  HPEN hPenShadow = CreatePen(PS_SOLID, 1, COL_PLATE_SHADOW);
  if (convex) {
    SelectObject(hdc, hPenHi);
    MoveToEx(hdc, r.left + radius / 2 + dx, r.top + dy, NULL);
    LineTo(hdc, r.right - radius / 2 + dx, r.top + dy);
    MoveToEx(hdc, r.left + dx, r.top + radius / 2 + dy, NULL);
    LineTo(hdc, r.left + dx, r.bottom - radius / 2 + dy);
    SelectObject(hdc, hPenShadow);
    MoveToEx(hdc, r.left + radius / 2 + dx, r.bottom - 1 + dy, NULL);
    LineTo(hdc, r.right - radius / 2 + dx, r.bottom - 1 + dy);
    MoveToEx(hdc, r.right - 1 + dx, r.top + radius / 2 + dy, NULL);
    LineTo(hdc, r.right - 1 + dx, r.bottom - radius / 2 + dy);
  } else {
    SelectObject(hdc, hPenShadow);
    MoveToEx(hdc, r.left + radius / 2 + dx, r.top + dy, NULL);
    LineTo(hdc, r.right - radius / 2 + dx, r.top + dy);
    MoveToEx(hdc, r.left + dx, r.top + radius / 2 + dy, NULL);
    LineTo(hdc, r.left + dx, r.bottom - radius / 2 + dy);
    SelectObject(hdc, hPenHi);
    MoveToEx(hdc, r.left + radius / 2 + dx, r.bottom - 1 + dy, NULL);
    LineTo(hdc, r.right - radius / 2 + dx, r.bottom - 1 + dy);
    MoveToEx(hdc, r.right - 1 + dx, r.top + radius / 2 + dy, NULL);
    LineTo(hdc, r.right - 1 + dx, r.bottom - radius / 2 + dy);
  }
  DeleteObject(SelectObject(hdc, hOldBrush));
  SelectObject(hdc, hOldPen);
  DeleteObject(hPenHi);
  DeleteObject(hPenShadow);
}

HBITMAP g_hSplashBm = NULL;

void DrawSplash(HDC hdc, RECT r) {
  if (g_hSplashBm) {
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hOldBm = (HBITMAP)SelectObject(hdcMem, g_hSplashBm);
    BITMAP bm;
    GetObject(g_hSplashBm, sizeof(bm), &bm);
    DWORD elapsed = GetTickCount() - splashStartTime;
    float progress = (float)elapsed / 2000.0f;
    if (progress > 1.0f)
      progress = 1.0f;
    float easedProgress = 1.0f - (1.0f - progress) * (1.0f - progress);
    int targetY = r.top + (r.bottom - r.top - bm.bmHeight) / 2;
    int startY = targetY + 50;
    int y = (int)(startY - (startY - targetY) * easedProgress);
    int x = r.left + (r.right - r.left - bm.bmWidth) / 2;
    BitBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOldBm);
    DeleteDC(hdcMem);
    SelectObject(hdc, hFontTiny);
    SetTextColor(hdc, COL_ACCENT);
    RECT rSub = {r.left, y + bm.bmHeight + 10, r.right, y + bm.bmHeight + 30};
    DrawText(hdc, _T("SYSTEM LOADING..."), -1, &rSub,
             DT_CENTER | DT_TOP | DT_SINGLELINE);
  } else {
    HBRUSH hbg = CreateSolidBrush(COL_BG);
    FillRect(hdc, &r, hbg);
    DeleteObject(hbg);
    int cx = (r.left + r.right) / 2;
    int cy = (r.top + r.bottom) / 2;
    RECT rBox = {cx - 60, cy - 60, cx + 60, cy + 60};
    DrawPlate(hdc, rBox, true, 20);
    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, COL_TEXT);
    RECT rText = {r.left, cy - 20, r.right, cy + 20};
    DrawText(hdc, _T("BOOTING..."), -1, &rText,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  }
}

void LoadTofuTasks() {
  todos.clear();
  const TCHAR *path = _T("\\Storage Card\\アプリ\\TofuMental\\tasks.txt");
  HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    TCHAR localPath[MAX_PATH];
    PathJoin(localPath, appPath, _T("TofuMental\\tasks.txt"));
    hFile = CreateFile(localPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  }
  if (hFile == INVALID_HANDLE_VALUE)
    return;
  DWORD fileSize = GetFileSize(hFile, NULL);
  if (fileSize <= 2) {
    CloseHandle(hFile);
    return;
  }
  BYTE *buffer = (BYTE *)malloc(fileSize + 2);
  DWORD read;
  ReadFile(hFile, buffer, fileSize, &read, NULL);
  CloseHandle(hFile);
  buffer[read] = 0;
  buffer[read + 1] = 0;
  wchar_t *start = (wchar_t *)buffer;
  if (*start == 0xFEFF)
    start++;
  wchar_t *line = wcstok(start, L"\r\n");
  while (line) {
    std::wstring ws(line);
    size_t sep = ws.find_last_of(L'|');
    if (sep != std::wstring::npos) {
      TodoItem item;
      item.title = ws.substr(0, sep);
      item.completed = (ws.substr(sep + 1, 1) == L"1");
      if (!item.title.empty())
        todos.push_back(item);
    }
    line = wcstok(NULL, L"\r\n");
  }
  free(buffer);
}

void SaveTofuTasks() {
  const TCHAR *path = _T("\\Storage Card\\アプリ\\TofuMental\\tasks.txt");
  HANDLE hFile = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    TCHAR localPath[MAX_PATH];
    PathJoin(localPath, appPath, _T("TofuMental\\tasks.txt"));
    hFile = CreateFile(localPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
  }
  if (hFile == INVALID_HANDLE_VALUE)
    return;

  // Write BOM
  unsigned short bom = 0xFEFF;
  DWORD written;
  WriteFile(hFile, &bom, 2, &written, NULL);

  for (size_t i = 0; i < todos.size(); i++) {
    std::wstring line =
        todos[i].title + L"|" + (todos[i].completed ? L"1" : L"0") + L"\r\n";
    WriteFile(hFile, line.c_str(), (DWORD)line.length() * 2, &written, NULL);
  }
  CloseHandle(hFile);
}

void InitDashboardData() {
  InitDictionary(dictionary);
  if (!dictionary.empty())
    currentWord = dictionary[rand() % dictionary.size()];
  LoadTofuTasks();
}

// --- Modules ---
void DrawClockModule(HDC hdc, RECT r) {
  SYSTEMTIME st;
  GetLocalTime(&st);
  TCHAR buf[64];
  _stprintf(buf, _T("%02d:%02d:%02d"), st.wHour, st.wMinute, st.wSecond);
  SetTextColor(hdc, COL_TEXT);
  SetBkMode(hdc, TRANSPARENT);
  SelectObject(hdc, hFontClock);
  DrawText(hdc, buf, -1, &r,
           DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
}

void DrawCalendarModule(HDC hdc, RECT r) {
  SYSTEMTIME st;
  GetLocalTime(&st);
  TCHAR header[32];
  _stprintf(header, _T("%s %d"), monthNames[st.wMonth - 1], st.wYear);
  SelectObject(hdc, hFontSmall);
  SetTextColor(hdc, COL_TEXT);
  RECT rHeader = {r.left, r.top + 5, r.right, r.top + 25};
  DrawText(hdc, header, -1, &rHeader, DT_CENTER | DT_TOP | DT_SINGLELINE);
  const TCHAR *days[] = {_T("S"), _T("M"), _T("T"), _T("W"),
                         _T("T"), _T("F"), _T("S")};
  SelectObject(hdc, hFontTiny);
  int colWidth = (r.right - r.left) / 7;
  int rowHeight = (r.bottom - r.top - 30) / 6;
  if (rowHeight < 12)
    rowHeight = 12;
  for (int i = 0; i < 7; i++) {
    RECT rDayLabels = {r.left + i * colWidth, r.top + 25,
                       r.left + (i + 1) * colWidth, r.top + 40};
    DrawText(hdc, days[i], -1, &rDayLabels, DT_CENTER | DT_TOP | DT_SINGLELINE);
  }
  SYSTEMTIME stFirst = st;
  stFirst.wDay = 1;
  FILETIME ft;
  SystemTimeToFileTime(&stFirst, &ft);
  FileTimeToSystemTime(&ft, &stFirst);
  int startDay = stFirst.wDayOfWeek, daysInMonth = 31;
  if (st.wMonth == 2)
    daysInMonth = (st.wYear % 4 == 0) ? 29 : 28;
  else if (st.wMonth == 4 || st.wMonth == 6 || st.wMonth == 9 ||
           st.wMonth == 11)
    daysInMonth = 30;
  int row = 0, col = startDay;
  for (int d = 1; d <= daysInMonth; d++) {
    RECT rD = {r.left + col * colWidth, r.top + 40 + row * rowHeight,
               r.left + (col + 1) * colWidth,
               r.top + 40 + (row + 1) * rowHeight};
    TCHAR dBuf[4];
    _stprintf(dBuf, _T("%d"), d);
    if (d == st.wDay)
      SetTextColor(hdc, RGB(255, 0, 0));
    else
      SetTextColor(hdc, COL_TEXT);
    DrawText(hdc, dBuf, -1, &rD, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    col++;
    if (col >= 7) {
      col = 0;
      row++;
    }
  }
}

void DrawTimeModule(HDC hdc, RECT r) {
  SYSTEMTIME st;
  GetLocalTime(&st);
  int elapsedSec = st.wHour * 3600 + st.wMinute * 60 + st.wSecond;
  int totalSec = 24 * 3600;
  int remainSec = totalSec - elapsedSec;
  int donePct = (elapsedSec * 100) / totalSec;
  int remainPct = 100 - donePct;
  TCHAR buf[64];
  _stprintf(buf, _T("%02d:%02d:%02d"), remainSec / 3600,
            (remainSec % 3600) / 60, remainSec % 60);
  SelectObject(hdc, hFontSmall);
  SetTextColor(hdc, COL_ACCENT);
  RECT rLabel = {r.left + 10, r.top + 10, r.right, r.top + 25};
  DrawText(hdc, _T("TIME REMAINING"), -1, &rLabel,
           DT_LEFT | DT_TOP | DT_SINGLELINE);
  SetTextColor(hdc, COL_TEXT);
  SelectObject(hdc, hFontMain);
  RECT rTime = {r.left + 10, r.top + 30, r.right - 100, r.top + 60};
  DrawText(hdc, buf, -1, &rTime, DT_LEFT | DT_TOP | DT_SINGLELINE);
  TCHAR pBuf[64];
  _stprintf(pBuf, _T("%d%% DONE / %d%% REMAIN"), donePct, remainPct);
  SelectObject(hdc, hFontTiny);
  RECT rPct = {r.right - 200, r.top + 40, r.right - 10, r.top + 55};
  DrawText(hdc, pBuf, -1, &rPct, DT_RIGHT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
  RECT rBarBase = {r.left + 10, r.top + 70, r.right - 10, r.top + 85};
  DrawPlate(hdc, rBarBase, false, 4);
  int fillWidth = (rBarBase.right - rBarBase.left) * elapsedSec / totalSec;
  if (fillWidth > 0) {
    RECT rBarFill = {rBarBase.left, rBarBase.top, rBarBase.left + fillWidth,
                     rBarBase.bottom};
    DrawPlate(hdc, rBarFill, true, 4);
  }
}

void DrawTodoModule(HDC hdc, RECT r) {
  SelectObject(hdc, hFontSmall);
  SetTextColor(hdc, COL_ACCENT);
  RECT rLabel = {r.left + 10, r.top + 10, r.right, r.top + 25};
  DrawText(hdc, _T("TODO LIST"), -1, &rLabel, DT_LEFT | DT_TOP | DT_SINGLELINE);
  SetTextColor(hdc, COL_TEXT);
  SelectObject(hdc, hFontTiny);
  for (size_t i = 0; i < todos.size() && i < 5; i++) {
    RECT rTodo = {r.left + 10, r.top + 35 + (int)i * 15, r.right - 10,
                  r.top + 50 + (int)i * 15};
    TCHAR tBuf[128];
    if (todos[i].completed) {
      SetTextColor(hdc, COL_ACCENT);
      _stprintf(tBuf, _T("[x] %s"), todos[i].title.c_str());
    } else {
      SetTextColor(hdc, COL_TEXT);
      _stprintf(tBuf, _T("[ ] %s"), todos[i].title.c_str());
    }
    DrawText(hdc, tBuf, -1, &rTodo,
             DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
  }
}

void DrawDictionaryModule(HDC hdc, RECT r) {
  SelectObject(hdc, hFontSmall);
  SetTextColor(hdc, COL_ACCENT);
  RECT rLabel = {r.left + 10, r.top + 10, r.right, r.top + 25};
  DrawText(hdc, _T("WORD OF THE DAY"), -1, &rLabel,
           DT_LEFT | DT_TOP | DT_SINGLELINE);
  SetTextColor(hdc, COL_TEXT);
  SelectObject(hdc, hFontMain);
  RECT rWord = {r.left + 10, r.top + 30, r.right - 10, r.top + 55};
  DrawText(hdc, currentWord.word.c_str(), -1, &rWord,
           DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
  SelectObject(hdc, hFontJp);
  SetTextColor(hdc, COL_TEXT);
  RECT rJp = {r.left + 10, r.top + 60, r.right - 10, r.top + 95};
  DrawText(hdc, currentWord.jp.c_str(), -1, &rJp,
           DT_LEFT | DT_TOP | DT_WORDBREAK);
}

void DrawSystemModule(HDC hdc, RECT r) {
  SelectObject(hdc, hFontSmall);
  SetTextColor(hdc, COL_ACCENT);
  RECT rLabel = {r.left + 10, r.top + 10, r.right, r.top + 25};
  DrawText(hdc, _T("SYSTEM STATUS"), -1, &rLabel,
           DT_LEFT | DT_TOP | DT_SINGLELINE);
  DWORD uptime = GetTickCount() / 1000;
  TCHAR buf[64];
  _stprintf(buf, _T("UPTIME: %dh %dm %ds"), uptime / 3600, (uptime % 3600) / 60,
            uptime % 60);
  SetTextColor(hdc, COL_TEXT);
  SelectObject(hdc, hFontTiny);
  RECT rStat1 = {r.left + 10, r.top + 35, r.right - 10, r.top + 50};
  DrawText(hdc, buf, -1, &rStat1, DT_LEFT | DT_TOP | DT_SINGLELINE);
  TCHAR devInfo[128];
  GetDeviceString(devInfo, 128);
  _stprintf(buf, _T("DEVICE: %s"), devInfo);
  RECT rStat2 = {r.left + 10, r.top + 55, r.right - 10, r.top + 70};
  DrawText(hdc, buf, -1, &rStat2, DT_LEFT | DT_TOP | DT_SINGLELINE);
}

void DrawSettings(HDC hdc, RECT r) {
  HBRUSH hbg = CreateSolidBrush(COL_BG);
  FillRect(hdc, &r, hbg);
  DeleteObject(hbg);
  const TCHAR *menu[] = {_T("NIGHT MODE"),    _T("ACCENT COLOR"),
                         _T("BURN-IN GUARD"), _T("FONT WEIGHT"),
                         _T("CLOCK FONT"),    _T("MAIN FONT"),
                         _T("CLOSE")};
  const TCHAR *values[] = {nightMode ? _T("ON") : _T("OFF"),
                           g_accents[g_selectedAccent].name,
                           burninGuard ? _T("ON") : _T("OFF"),
                           fontWeight == FW_BOLD ? _T("BOLD") : _T("NORMAL"),
                           g_fonts[g_selectedClockFont].displayName,
                           g_fonts[g_selectedMainFont].displayName,
                           _T("")};
  SetBkMode(hdc, TRANSPARENT);
  SelectObject(hdc, hFontMain);
  SetTextColor(hdc, COL_TEXT);
  RECT rTitle = {r.left, r.top + 10, r.right, r.top + 50};
  DrawText(hdc, _T("SETTINGS"), -1, &rTitle, DT_CENTER | DT_TOP);
  SelectObject(hdc, hFontUI);
  for (int i = 0; i < 7; i++) {
    RECT rItem = {r.left + 50, r.top + 60 + i * 32, r.right - 50,
                  r.top + 92 + i * 32};
    if (i == settingsFocus)
      SetTextColor(hdc, RGB(255, 0, 0));
    else
      SetTextColor(hdc, COL_TEXT);
    DrawText(hdc, menu[i], -1, &rItem, DT_LEFT | DT_TOP);
    DrawText(hdc, values[i], -1, &rItem, DT_RIGHT | DT_TOP);
  }
}

void DrawCalendarLarge(HDC hdc, RECT r) {
  SYSTEMTIME st;
  GetLocalTime(&st);

  // Draw month and year header
  TCHAR header[32];
  _stprintf(header, _T("%s %d"), monthNames[st.wMonth - 1], st.wYear);
  SelectObject(hdc, hFontMain);
  SetTextColor(hdc, COL_ACCENT);
  RECT rHeader = {r.left, r.top + 10, r.right, r.top + 40};
  DrawText(hdc, header, -1, &rHeader, DT_CENTER | DT_TOP | DT_SINGLELINE);

  // Draw day labels (S, M, T, W, T, F, S)
  const TCHAR *days[] = {_T("SUN"), _T("MON"), _T("TUE"), _T("WED"),
                         _T("THU"), _T("FRI"), _T("SAT")};
  SelectObject(hdc, hFontSmall);
  SetTextColor(hdc, COL_TEXT);
  int colWidth = (r.right - r.left) / 7;
  int startY = r.top + 50;
  int dayLabelHeight = 30;

  for (int i = 0; i < 7; i++) {
    RECT rDayLabels = {r.left + i * colWidth, startY,
                       r.left + (i + 1) * colWidth, startY + dayLabelHeight};
    DrawText(hdc, days[i], -1, &rDayLabels, DT_CENTER | DT_TOP | DT_SINGLELINE);
  }

  // Calculate first day of the month and days in month
  SYSTEMTIME stFirst = st;
  stFirst.wDay = 1;
  FILETIME ft;
  SystemTimeToFileTime(&stFirst, &ft);
  FileTimeToSystemTime(&ft, &stFirst);
  int startDay = stFirst.wDayOfWeek; // 0=Sunday, 1=Monday, etc.

  int daysInMonth = 31;
  if (st.wMonth == 2) {
    daysInMonth =
        (st.wYear % 4 == 0 && (st.wYear % 100 != 0 || st.wYear % 400 == 0))
            ? 29
            : 28;
  } else if (st.wMonth == 4 || st.wMonth == 6 || st.wMonth == 9 ||
             st.wMonth == 11) {
    daysInMonth = 30;
  }

  // Draw days of the month
  SelectObject(hdc, hFontMain); // Use hFontMain for day numbers
  int rowHeight =
      (r.bottom - startY - dayLabelHeight) / 6; // Max 6 rows for days
  if (rowHeight < 30)
    rowHeight = 30; // Minimum row height

  int row = 0;
  int col = startDay;

  for (int d = 1; d <= daysInMonth; d++) {
    RECT rD = {r.left + col * colWidth,
               startY + dayLabelHeight + row * rowHeight,
               r.left + (col + 1) * colWidth,
               startY + dayLabelHeight + (row + 1) * rowHeight};
    TCHAR dBuf[4];
    _stprintf(dBuf, _T("%d"), d);

    if (d == st.wDay) {
      SetTextColor(hdc, RGB(255, 0, 0)); // Highlight current day
    } else {
      SetTextColor(hdc, COL_TEXT);
    }
    DrawText(hdc, dBuf, -1, &rD, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    col++;
    if (col >= 7) {
      col = 0;
      row++;
    }
  }
}

void DrawClockDetail(HDC hdc, RECT r) {
  HBRUSH hbg = CreateSolidBrush(COL_BG);
  FillRect(hdc, &r, hbg);
  DeleteObject(hbg);

  int midY = (r.top + r.bottom) / 2;
  RECT rTop = {r.left, r.top, r.right, midY};
  RECT rBottom = {r.left, midY, r.right, r.bottom};

  SYSTEMTIME st;
  GetLocalTime(&st);
  TCHAR buf[64];
  _stprintf(buf, _T("%02d:%02d:%02d"), st.wHour, st.wMinute, st.wSecond);

  SetTextColor(hdc, COL_TEXT);
  SetBkMode(hdc, TRANSPARENT);

  // Slightly smaller font than 120 to fit in top half better
  HFONT hfBig =
      CreateFontSimple(100, fontWeight, g_fonts[g_selectedClockFont].faceName);
  SelectObject(hdc, hfBig);

  RECT rClock = {rTop.left, rTop.top + 40, rTop.right, rTop.top + 140};
  DrawText(hdc, buf, -1, &rClock,
           DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
  DeleteObject(hfBig);

  TCHAR dateBuf[128];
  const TCHAR *days[] = {_T("SUNDAY"),    _T("MONDAY"),   _T("TUESDAY"),
                         _T("WEDNESDAY"), _T("THURSDAY"), _T("FRIDAY"),
                         _T("SATURDAY")};
  _stprintf(dateBuf, _T("%s, %s %d, %d"), days[st.wDayOfWeek],
            monthNames[st.wMonth - 1], st.wDay, st.wYear);
  SelectObject(hdc, hFontMain);
  RECT rDate = {rTop.left, rTop.top + 160, rTop.right, rTop.top + 200};
  DrawText(hdc, dateBuf, -1, &rDate, DT_CENTER | DT_TOP | DT_SINGLELINE);

  // Draw Large Calendar in bottom half
  DrawCalendarLarge(hdc, rBottom);
}

void DrawTodoDetail(HDC hdc, RECT r) {
  HBRUSH hbg = CreateSolidBrush(COL_BG);
  FillRect(hdc, &r, hbg);
  DeleteObject(hbg);
  SelectObject(hdc, hFontUI);
  SetTextColor(hdc, COL_ACCENT);
  RECT rTitle = {r.left + 20, r.top + 20, r.right - 20, r.top + 60};
  SetBkMode(hdc, TRANSPARENT);
  DrawText(hdc, _T("TODO LIST (TofuMental Sync)"), -1, &rTitle,
           DT_LEFT | DT_TOP);
  SelectObject(hdc, hFontMain);
  for (int i = 0; i < 10 && (size_t)i < todos.size(); i++) {
    RECT rItem = {r.left + 40, r.top + 80 + i * 35, r.right - 40,
                  r.top + 115 + i * 35};
    if (i == g_viewFocus)
      SetTextColor(hdc, RGB(255, 0, 0));
    else
      SetTextColor(hdc, COL_TEXT);
    TCHAR buf[256];
    _stprintf(buf, _T("%s %s"), todos[i].completed ? _T("[x]") : _T("[ ]"),
              todos[i].title.c_str());
    DrawText(hdc, buf, -1, &rItem,
             DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
  }
}

void DrawDictionaryDetail(HDC hdc, RECT r) {
  HBRUSH hbg = CreateSolidBrush(COL_BG);
  FillRect(hdc, &r, hbg);
  DeleteObject(hbg);
  SelectObject(hdc, hFontUI);
  SetTextColor(hdc, COL_ACCENT);
  RECT rTitle = {r.left + 20, r.top + 20, r.right - 20, r.top + 60};
  SetBkMode(hdc, TRANSPARENT);
  DrawText(hdc, _T("DICTIONARY DETAILS"), -1, &rTitle, DT_LEFT | DT_TOP);
  int total = (int)dictionary.size();
  if (total == 0)
    return;
  for (int i = 0; i < 3; i++) {
    int idx = (g_viewFocus + i) % total;
    RECT rBox = {r.left + 20, r.top + 80 + i * 115, r.right - 20,
                 r.top + 185 + i * 115};
    DrawPlate(hdc, rBox, true);
    RECT rWord = {rBox.left + 15, rBox.top + 15, rBox.right - 15,
                  rBox.top + 45};
    SelectObject(hdc, hFontMain);
    SetTextColor(hdc, COL_TEXT);
    DrawText(hdc, dictionary[idx].word.c_str(), -1, &rWord,
             DT_LEFT | DT_TOP | DT_SINGLELINE);
    RECT rJp = {rBox.left + 15, rBox.top + 50, rBox.right - 15,
                rBox.bottom - 10};
    SelectObject(hdc, hFontJp);
    DrawText(hdc, dictionary[idx].jp.c_str(), -1, &rJp,
             DT_LEFT | DT_TOP | DT_WORDBREAK);
  }
}

void DrawDashboard(HDC hdc, RECT rDash) {
  RECT rUpper = {rDash.left + 10, 10 + driftY, rDash.right - 10,
                 rDash.bottom / 2 - 5 + driftY};
  DrawPlate(hdc, rUpper, false);
  RECT rClockRect = {rUpper.left + 10, rUpper.top + 10, rUpper.right - 160,
                     rUpper.bottom - 10};
  DrawClockModule(hdc, rClockRect);
  RECT rCalRect = {rUpper.right - 150, rUpper.top + 10, rUpper.right - 10,
                   rUpper.bottom - 10};
  DrawCalendarModule(hdc, rCalRect);
  int midX = (rDash.left + rDash.right) / 2,
      midY = (rDash.top + rDash.bottom) / 2, hBound = rDash.bottom - 10;
  RECT rL1 = {rDash.left + 10, midY + 5, midX - 5, midY + (hBound - midY) / 2};
  DrawPlate(hdc, rL1, true);
  DrawTimeModule(hdc, rL1);
  RECT rL2 = {midX + 5, midY + 5, rDash.right - 10, midY + (hBound - midY) / 2};
  DrawPlate(hdc, rL2, true);
  DrawTodoModule(hdc, rL2);
  RECT rL3 = {rDash.left + 10, midY + (hBound - midY) / 2 + 5, midX - 5,
              hBound};
  DrawPlate(hdc, rL3, true);
  DrawDictionaryModule(hdc, rL3);
  RECT rL4 = {midX + 5, midY + (hBound - midY) / 2 + 5, rDash.right - 10,
              hBound};
  DrawPlate(hdc, rL4, true);
  DrawSystemModule(hdc, rL4);
  SetTextColor(hdc, RGB(128, 128, 128));
  SelectObject(hdc, hFontTiny);
  RECT rLogo = {rDash.right - 100, rDash.bottom - 20, rDash.right - 10,
                rDash.bottom - 5};
  DrawText(hdc, _T("DP-05 V2"), -1, &rLogo,
           DT_RIGHT | DT_BOTTOM | DT_SINGLELINE);
}

void DrawView(HDC hdc, RECT r, ViewMode mode) {
  switch (mode) {
  case VIEW_DASHBOARD:
    DrawDashboard(hdc, r);
    break;
  case VIEW_CLOCK:
    DrawClockDetail(hdc, r);
    break;
  case VIEW_TODO:
    DrawTodoDetail(hdc, r);
    break;
  case VIEW_DICTIONARY:
    DrawDictionaryDetail(hdc, r);
    break;
  }
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  switch (uMsg) {
  case WM_CREATE:
    SetTimer(hWnd, ID_TIMER_CLOCK, TIMER_CLOCK_MS, NULL);
    SetTimer(hWnd, ID_TIMER_DRIFT, TIMER_DRIFT_MS, NULL);
    g_transitionPos = 0.0f;
    splashStartTime = GetTickCount();
    {
      Sleep(100);
      TCHAR fPath[MAX_PATH];
      PathJoin(fPath, appPath, _T("icon.bmp"));
      g_hSplashBm = LoadBMPFromFile(fPath);
      if (!g_hSplashBm) {
        TCHAR fPath8[MAX_PATH];
        PathJoin(fPath8, appPath, _T("icon8.bmp"));
        g_hSplashBm = LoadBMPFromFile(fPath8);
      }
      if (!g_hSplashBm)
        g_hSplashBm = LoadBMPFromFile(_T("icon.bmp"));
    }
    {
      TCHAR sPath[MAX_PATH];
      PathJoin(sPath, appPath, _T("splash.wav"));
      PlaySound(sPath, NULL, SND_FILENAME | SND_ASYNC);
    }
    UpdateColors();
    RefreshFonts();
    InitDashboardData();
    break;
  case WM_SIZE:
    GetClientRect(hWnd, &clientRect);
    break;
  case WM_ERASEBKGND:
    return 1;
  case WM_TIMER:
    if (wParam == ID_TIMER_CLOCK) {
      if (isSplashing && (GetTickCount() - splashStartTime > 2000)) {
        isSplashing = false;
        InvalidateRect(hWnd, NULL, TRUE);
      }
      InvalidateRect(hWnd, NULL, FALSE);
    } else if (wParam == ID_TIMER_DRIFT && burninGuard) {
      driftCycle++;
      if (driftCycle % 2 == 0)
        driftX = (rand() % 7) - 3;
      else
        driftY = (rand() % 7) - 3;
    } else if (wParam == ID_TIMER_TRANSITION) {
      DWORD elapsed = GetTickCount() - g_animStartTime;
      float t = (float)elapsed / ANIM_DURATION;
      if (t >= 1.0f) {
        t = 1.0f;
        g_isTransitioning = false;
        g_transitionPos = g_targetSettingsState ? 1.0f : 0.0f;
        settingsOpen = g_targetSettingsState;
        if (hDashCache) {
          DeleteObject(hDashCache);
          hDashCache = NULL;
        }
        if (hSettCache) {
          DeleteObject(hSettCache);
          hSettCache = NULL;
        }
        KillTimer(hWnd, ID_TIMER_TRANSITION);
      } else {
        float easedT = 1.0f - pow(1.0f - t, 3.0f);
        float targetPos = g_targetSettingsState ? 1.0f : 0.0f;
        g_transitionPos =
            g_startTransitionPos + (targetPos - g_startTransitionPos) * easedT;
      }
      InvalidateRect(hWnd, NULL, FALSE);
    } else if (wParam == ID_TIMER_VIEW_TRANSITION) {
      DWORD elapsed = GetTickCount() - g_viewAnimStartTime;
      float t = (float)elapsed / ANIM_DURATION;
      if (t >= 1.0f) {
        t = 1.0f;
        g_isViewTransitioning = false;
        g_viewTransitionPos = 0.0f;
        g_currentView = g_targetView;
        KillTimer(hWnd, ID_TIMER_VIEW_TRANSITION);
      } else {
        float easedT = 1.0f - pow(1.0f - t, 3.0f);
        g_viewTransitionPos = easedT;
      }
      InvalidateRect(hWnd, NULL, FALSE);
    }
    break;
  case WM_KEYDOWN:
    if (settingsOpen) {
      if (wParam == VK_UP) {
        settingsFocus = (settingsFocus + 6) % 7;
        InvalidateRect(hWnd, NULL, TRUE);
      } else if (wParam == VK_DOWN) {
        settingsFocus = (settingsFocus + 1) % 7;
        InvalidateRect(hWnd, NULL, TRUE);
      } else if (wParam == VK_LEFT || wParam == VK_RIGHT ||
                 wParam == VK_SPACE || wParam == VK_RETURN) {
        bool needsColor = false, needsFont = false;
        if (settingsFocus == 0) {
          nightMode = !nightMode;
          needsColor = true;
        } else if (settingsFocus == 1) {
          if (wParam == VK_LEFT)
            g_selectedAccent =
                (g_selectedAccent + g_accentCount - 1) % g_accentCount;
          else
            g_selectedAccent = (g_selectedAccent + 1) % g_accentCount;
          needsColor = true;
        } else if (settingsFocus == 2)
          burninGuard = !burninGuard;
        else if (settingsFocus == 3) {
          fontWeight = (fontWeight == FW_BOLD) ? FW_NORMAL : FW_BOLD;
          needsFont = true;
        } else if (settingsFocus == 4) {
          if (wParam == VK_LEFT)
            g_selectedClockFont =
                (g_selectedClockFont + g_fontCount - 1) % g_fontCount;
          else
            g_selectedClockFont = (g_selectedClockFont + 1) % g_fontCount;
          needsFont = true;
        } else if (settingsFocus == 5) {
          if (wParam == VK_LEFT)
            g_selectedMainFont =
                (g_selectedMainFont + g_fontCount - 1) % g_fontCount;
          else
            g_selectedMainFont = (g_selectedMainFont + 1) % g_fontCount;
          needsFont = true;
        } else if (settingsFocus == 6 &&
                   (wParam == VK_RETURN || wParam == VK_SPACE)) {
          if (!g_isTransitioning) {
            HDC hdc = GetDC(hWnd);
            HDC hdcMem = CreateCompatibleDC(hdc);
            if (hDashCache)
              DeleteObject(hDashCache);
            if (hSettCache)
              DeleteObject(hSettCache);
            hDashCache = CreateCompatibleBitmap(hdc, clientRect.right,
                                                clientRect.bottom);
            hSettCache = CreateCompatibleBitmap(hdc, clientRect.right,
                                                clientRect.bottom);
            HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hDashCache);
            HBRUSH hbg = CreateSolidBrush(COL_BG);
            FillRect(hdcMem, &clientRect, hbg);
            DeleteObject(hbg);
            DrawView(hdcMem, clientRect, g_currentView);
            SelectObject(hdcMem, hSettCache);
            hbg = CreateSolidBrush(COL_BG);
            FillRect(hdcMem, &clientRect, hbg);
            DeleteObject(hbg);
            DrawSettings(hdcMem, clientRect);
            SelectObject(hdcMem, hOld);
            DeleteDC(hdcMem);
            ReleaseDC(hWnd, hdc);
            g_isTransitioning = true;
            g_targetSettingsState = false;
            g_startTransitionPos = g_transitionPos;
            g_animStartTime = GetTickCount();
            SetTimer(hWnd, ID_TIMER_TRANSITION, 16, NULL);
          }
        }
        if (needsColor)
          UpdateColors();
        if (needsFont)
          RefreshFonts();
        SaveSettings();
        InvalidateRect(hWnd, NULL, TRUE);
      } else if (wParam == VK_ESCAPE) {
        if (!g_isTransitioning) {
          HDC hdc = GetDC(hWnd);
          HDC hdcMem = CreateCompatibleDC(hdc);
          if (hDashCache)
            DeleteObject(hDashCache);
          if (hSettCache)
            DeleteObject(hSettCache);
          hDashCache =
              CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
          hSettCache =
              CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
          HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hDashCache);
          HBRUSH hbg = CreateSolidBrush(COL_BG);
          FillRect(hdcMem, &clientRect, hbg);
          DeleteObject(hbg);
          DrawView(hdcMem, clientRect, g_currentView);
          SelectObject(hdcMem, hSettCache);
          hbg = CreateSolidBrush(COL_BG);
          FillRect(hdcMem, &clientRect, hbg);
          DeleteObject(hbg);
          DrawSettings(hdcMem, clientRect);
          SelectObject(hdcMem, hOld);
          DeleteDC(hdcMem);
          ReleaseDC(hWnd, hdc);
          g_isTransitioning = true;
          g_targetSettingsState = false;
          g_startTransitionPos = g_transitionPos;
          g_animStartTime = GetTickCount();
          SetTimer(hWnd, ID_TIMER_TRANSITION, 16, NULL);
        }
      }
    } else {
      if (wParam == VK_ESCAPE)
        PostQuitMessage(0);
      else if (wParam == VK_RETURN) {
        if (!g_isTransitioning) {
          HDC hdc = GetDC(hWnd);
          HDC hdcMem = CreateCompatibleDC(hdc);
          if (hDashCache)
            DeleteObject(hDashCache);
          if (hSettCache)
            DeleteObject(hSettCache);
          hDashCache =
              CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
          hSettCache =
              CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
          HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hDashCache);
          HBRUSH hbg = CreateSolidBrush(COL_BG);
          FillRect(hdcMem, &clientRect, hbg);
          DeleteObject(hbg);
          DrawView(hdcMem, clientRect, g_currentView);
          SelectObject(hdcMem, hSettCache);
          hbg = CreateSolidBrush(COL_BG);
          FillRect(hdcMem, &clientRect, hbg);
          DeleteObject(hbg);
          DrawSettings(hdcMem, clientRect);
          SelectObject(hdcMem, hOld);
          DeleteDC(hdcMem);
          ReleaseDC(hWnd, hdc);
          g_isTransitioning = true;
          g_targetSettingsState = true;
          g_startTransitionPos = g_transitionPos;
          g_animStartTime = GetTickCount();
          SetTimer(hWnd, ID_TIMER_TRANSITION, 16, NULL);
        }
      } else if (wParam == VK_RIGHT || wParam == VK_LEFT) {
        if (!g_isViewTransitioning && !g_isTransitioning) {
          g_viewDirection = (wParam == VK_RIGHT) ? 1 : -1;
          if (g_viewDirection == 1)
            g_targetView = static_cast<ViewMode>((g_currentView + 1) % 4);
          else
            g_targetView = static_cast<ViewMode>((g_currentView + 3) % 4);
          g_isViewTransitioning = true;
          g_viewTransitionPos = 0.0f;
          g_viewAnimStartTime = GetTickCount();
          g_viewFocus = 0;
          SetTimer(hWnd, ID_TIMER_VIEW_TRANSITION, 16, NULL);
        }
      } else if (wParam == VK_UP || wParam == VK_DOWN) {
        if (g_currentView == VIEW_TODO && !todos.empty()) {
          if (wParam == VK_UP)
            g_viewFocus =
                (g_viewFocus + (int)todos.size() - 1) % (int)todos.size();
          else
            g_viewFocus = (g_viewFocus + 1) % (int)todos.size();
          InvalidateRect(hWnd, NULL, FALSE);
        } else if (g_currentView == VIEW_DICTIONARY && !dictionary.empty()) {
          if (wParam == VK_UP)
            g_viewFocus = (g_viewFocus + (int)dictionary.size() - 1) %
                          (int)dictionary.size();
          else
            g_viewFocus = (g_viewFocus + 1) % (int)dictionary.size();
          InvalidateRect(hWnd, NULL, FALSE);
        }
      } else if (wParam == VK_SPACE) {
        if (g_currentView == VIEW_TODO && !todos.empty()) {
          todos[g_viewFocus].completed = !todos[g_viewFocus].completed;
          SaveTofuTasks();
          InvalidateRect(hWnd, NULL, FALSE);
        }
      }
    }
    break;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem =
        CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
    HBITMAP hOldBm = (HBITMAP)SelectObject(hdcMem, hbmMem);
    HBRUSH hbg = CreateSolidBrush(COL_BG);
    FillRect(hdcMem, &clientRect, hbg);
    DeleteObject(hbg);
    if (isSplashing)
      DrawSplash(hdcMem, clientRect);
    else if (g_isTransitioning ||
             (g_transitionPos > 0.0f && g_transitionPos < 1.0f)) {
      int offsetY = (int)(g_transitionPos * clientRect.bottom);
      if (hDashCache) {
        HDC hTempDC = CreateCompatibleDC(hdcMem);
        HBITMAP hOldTemp = (HBITMAP)SelectObject(hTempDC, hDashCache);
        BitBlt(hdcMem, 0, -offsetY, clientRect.right, clientRect.bottom,
               hTempDC, 0, 0, SRCCOPY);
        SelectObject(hTempDC, hOldTemp);
        DeleteDC(hTempDC);
      } else {
        RECT rDash = {0, -offsetY, clientRect.right,
                      clientRect.bottom - offsetY};
        DrawView(hdcMem, rDash, g_currentView);
      }
      if (hSettCache) {
        HDC hTempDC = CreateCompatibleDC(hdcMem);
        HBITMAP hOldTemp = (HBITMAP)SelectObject(hTempDC, hSettCache);
        BitBlt(hdcMem, 0, clientRect.bottom - offsetY, clientRect.right,
               clientRect.bottom, hTempDC, 0, 0, SRCCOPY);
        SelectObject(hTempDC, hOldTemp);
        DeleteDC(hTempDC);
      } else {
        RECT rSett = {0, clientRect.bottom - offsetY, clientRect.right,
                      clientRect.bottom * 2 - offsetY};
        DrawSettings(hdcMem, rSett);
      }
    } else if (g_isViewTransitioning) {
      int offsetX =
          (int)(g_viewTransitionPos * clientRect.right * g_viewDirection);
      RECT r1 = {-offsetX, 0, clientRect.right - offsetX, clientRect.bottom};
      RECT r2;
      if (g_viewDirection == 1)
        r2 = {clientRect.right - offsetX, 0, clientRect.right * 2 - offsetX,
              clientRect.bottom};
      else
        r2 = {-clientRect.right - offsetX, 0, -offsetX, clientRect.bottom};
      DrawView(hdcMem, r1, g_currentView);
      DrawView(hdcMem, r2, g_targetView);
    } else if (settingsOpen)
      DrawSettings(hdcMem, clientRect);
    else
      DrawView(hdcMem, clientRect, g_currentView);
    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0,
           SRCCOPY);
    SelectObject(hdcMem, hOldBm);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hWnd, &ps);
  } break;
  case WM_LBUTTONDOWN:
    if (!settingsOpen && !dictionary.empty()) {
      currentWord = dictionary[rand() % dictionary.size()];
      InvalidateRect(hWnd, NULL, FALSE);
    }
    break;
  case WM_DESTROY:
    SaveSettings();
    if (g_hSplashBm)
      DeleteObject(g_hSplashBm);
    for (int i = 0; i < g_fontCount; i++) {
      TCHAR fPath[MAX_PATH];
      PathJoin(fPath, appPath, g_fonts[i].fileName);
      RemoveFontResource(fPath);
    }
    SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
  return 0;
}

#ifdef UNDER_CE
int main(int argc, char **argv) {
  HINSTANCE hInstance = GetModuleHandle(NULL);
  int nCmdShow = SW_SHOW;
  GetModuleFileName(NULL, appPath, MAX_PATH);
  TCHAR *lastSlash = _tcsrchr(appPath, _T('\\'));
  if (lastSlash)
    *(lastSlash + 1) = _T('\0');
  for (int i = 0; i < g_fontCount; i++) {
    TCHAR fPath[MAX_PATH];
    PathJoin(fPath, appPath, g_fonts[i].fileName);
    if (GetFileAttributes(fPath) != INVALID_FILE_ATTRIBUTES)
      AddFontResource(fPath);
  }
  SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
  LoadSettings();
#else
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine, int nCmdShow) {
  GetModuleFileName(NULL, appPath, MAX_PATH);
  TCHAR *lastSlash = _tcsrchr(appPath, _T('\\'));
  if (!lastSlash)
    lastSlash = _tcsrchr(appPath, _T('/'));
  if (lastSlash)
    *(lastSlash + 1) = _T('\0');
  for (int i = 0; i < g_fontCount; i++) {
    TCHAR fPath[MAX_PATH];
    _tcscpy(fPath, appPath);
    _tcscat(fPath, g_fonts[i].fileName);
    AddFontResource(fPath);
  }
  SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
  LoadSettings();
#endif
  WNDCLASS wc;
  memset(&wc, 0, sizeof(WNDCLASS));
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszClassName = _T("DP05_Dashboard");
  RegisterClass(&wc);
  HWND hWnd =
      CreateWindowEx(WS_EX_TOPMOST, _T("DP05_Dashboard"), _T("DP-05 Dashboard"),
                     WS_POPUP, 0, 0, 800, 480, NULL, NULL, hInstance, NULL);
  if (!hWnd)
    return 0;
  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return (int)msg.wParam;
}
