/*
 * DP-05 "Measurement Device OS" Dashboard for Sharp Brain PW-SH2 (Windows
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
#include <string>
#include <tchar.h>
#include <vector>
#include <wctype.h>
#include <windows.h>

// --- Constants ---
#define ID_TIMER_CLOCK 1
#define ID_TIMER_DRIFT 2
#define TIMER_CLOCK_MS 1000
#define TIMER_DRIFT_MS 60000

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
bool settingsOpen = false;
bool burninGuard = true;
bool isSplashing = true;
DWORD splashStartTime = 0;
int fontWeight = FW_BOLD;

int driftX = 0;
int driftY = 0;
int driftCycle = 0;
int settingsFocus = 0;

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
int g_selectedClockFont = 1; // Default 8x10 (index 1 now)
int g_selectedMainFont = 3;  // Default CP period for Japanese (index 3 now)

// Data
std::vector<WordEntry> dictionary;
WordEntry currentWord;
std::vector<std::wstring> todos;

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
  void *bits = malloc(size);
  SetFilePointer(hFile, bfh.bfOffBits, NULL, FILE_BEGIN);
  ReadFile(hFile, bits, size, &read, NULL);
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
  _tcscpy(out, base);
  size_t len = _tcslen(out);
  if (len > 0 && out[len - 1] != _T('\\') && out[len - 1] != _T('/')) {
    _tcscat(out, _T("\\"));
  }
  _tcscat(out, file);
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
    int x = r.left + (r.right - r.left - bm.bmWidth) / 2;
    int y = r.top + (r.bottom - r.top - bm.bmHeight) / 2;
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

    // Draw a simpler placeholder to avoid "overlapping" confusion
    RECT rBox = {cx - 60, cy - 60, cx + 60, cy + 60};
    DrawPlate(hdc, rBox, true, 20);

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, COL_TEXT);
    RECT rText = {r.left, cy - 20, r.right, cy + 20};
    DrawText(hdc, _T("BOOTING..."), -1, &rText,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hFontTiny);
    SetTextColor(hdc, RGB(255, 0, 0));
    RECT rErr = {r.left, r.bottom - 30, r.right, r.bottom - 10};
    DrawText(hdc, _T("ICON.BMP NOT FOUND OR INVALID"), -1, &rErr,
             DT_CENTER | DT_TOP | DT_SINGLELINE);
  }
}

void InitDashboardData() {
  InitDictionary(dictionary);
  if (!dictionary.empty())
    currentWord = dictionary[rand() % dictionary.size()];
  todos.clear();
  todos.push_back(L"System Check");
  todos.push_back(L"Calibrate Device");
  todos.push_back(L"Synchronize Time");
  todos.push_back(L"Update Dictionary");
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
  const TCHAR *monthNames[] = {_T("JAN"), _T("FEB"), _T("MAR"), _T("APR"),
                               _T("MAY"), _T("JUN"), _T("JUL"), _T("AUG"),
                               _T("SEP"), _T("OCT"), _T("NOV"), _T("DEC")};
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
  for (size_t i = 0; i < todos.size(); i++) {
    RECT rTodo = {r.left + 10, r.top + 35 + (int)i * 15, r.right - 10,
                  r.top + 50 + (int)i * 15};
    TCHAR tBuf[128];
    _stprintf(tBuf, _T("- %s"), todos[i].c_str());
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
  _stprintf(buf, _T("DEVICE: DP-05 OS"));
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

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  switch (uMsg) {
  case WM_CREATE:
    SetTimer(hWnd, ID_TIMER_CLOCK, TIMER_CLOCK_MS, NULL);
    SetTimer(hWnd, ID_TIMER_DRIFT, TIMER_DRIFT_MS, NULL);
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
      if (!g_hSplashBm) {
        g_hSplashBm = LoadBMPFromFile(_T("icon.bmp"));
      }
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
    }
    break;
  case WM_KEYDOWN:
    if (wParam == VK_ESCAPE)
      PostQuitMessage(0);
    else if (wParam == VK_RETURN) {
      settingsOpen = !settingsOpen;
      InvalidateRect(hWnd, NULL, TRUE);
    }
    if (settingsOpen) {
      if (wParam == VK_UP) {
        settingsFocus = (settingsFocus + 6) % 7;
        InvalidateRect(hWnd, NULL, TRUE);
      } else if (wParam == VK_DOWN) {
        settingsFocus = (settingsFocus + 1) % 7;
        InvalidateRect(hWnd, NULL, TRUE);
      } else if (wParam == VK_LEFT || wParam == VK_RIGHT ||
                 wParam == VK_SPACE) {
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
        } else if (settingsFocus == 6)
          settingsOpen = false;
        if (needsColor)
          UpdateColors();
        if (needsFont)
          RefreshFonts();
        InvalidateRect(hWnd, NULL, TRUE);
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

    if (isSplashing) {
      DrawSplash(hdcMem, clientRect);
    } else if (settingsOpen) {
      DrawSettings(hdcMem, clientRect);
    } else {
      RECT rUpper = {10, 10, clientRect.right - 10, clientRect.bottom / 2 - 5};
      DrawPlate(hdcMem, rUpper, false);
      RECT rClockRect = {rUpper.left + 10, rUpper.top + 10, rUpper.right - 160,
                         rUpper.bottom - 10};
      DrawClockModule(hdcMem, rClockRect);
      RECT rCalRect = {rUpper.right - 150, rUpper.top + 10, rUpper.right - 10,
                       rUpper.bottom - 10};
      DrawCalendarModule(hdcMem, rCalRect);
      int midX = clientRect.right / 2, midY = clientRect.bottom / 2,
          hBound = clientRect.bottom - 10;
      RECT rL1 = {10, midY + 5, midX - 5, midY + (hBound - midY) / 2};
      DrawPlate(hdcMem, rL1, true);
      DrawTimeModule(hdcMem, rL1);
      RECT rL2 = {midX + 5, midY + 5, clientRect.right - 10,
                  midY + (hBound - midY) / 2};
      DrawPlate(hdcMem, rL2, true);
      DrawTodoModule(hdcMem, rL2);
      RECT rL3 = {10, midY + (hBound - midY) / 2 + 5, midX - 5, hBound};
      DrawPlate(hdcMem, rL3, true);
      DrawDictionaryModule(hdcMem, rL3);
      RECT rL4 = {midX + 5, midY + (hBound - midY) / 2 + 5,
                  clientRect.right - 10, hBound};
      DrawPlate(hdcMem, rL4, true);
      DrawSystemModule(hdcMem, rL4);
      SetTextColor(hdcMem, RGB(128, 128, 128));
      SelectObject(hdcMem, hFontTiny);
      RECT rLogo = {clientRect.right - 100, clientRect.bottom - 20,
                    clientRect.right - 10, clientRect.bottom - 5};
      DrawText(hdcMem, _T("DP-05 V2"), -1, &rLogo,
               DT_RIGHT | DT_BOTTOM | DT_SINGLELINE);
    }
    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0,
           SRCCOPY);
    SelectObject(hdcMem, hOldBm);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hWnd, &ps);
    break;
  }
  case WM_LBUTTONDOWN:
    if (!settingsOpen && !dictionary.empty()) {
      currentWord = dictionary[rand() % dictionary.size()];
      InvalidateRect(hWnd, NULL, FALSE);
    }
    break;
  case WM_DESTROY:
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

  int failCount = 0;
  TCHAR failMsg[1024] = _T("Font Load Errors:\n");

  for (int i = 0; i < g_fontCount; i++) {
    TCHAR fPath[MAX_PATH];
    PathJoin(fPath, appPath, g_fonts[i].fileName);
    int res = AddFontResource(fPath);
    if (res == 0) {
      // Try local fallback
      res = AddFontResource(g_fonts[i].fileName);
    }
    if (res == 0) {
      _stprintf(failMsg + _tcslen(failMsg), _T("- %s FAILED (err %d)\n"),
                g_fonts[i].fileName, GetLastError());
      failCount++;
    }
  }

  if (failCount > 0) {
    // MessageBox(NULL, failMsg, _T("Font Debug"), MB_OK); // Removed for focus
  }
  SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
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
