// nginx-manager/src/simple-main.cpp
// Nginx 管理器 - 简化的原生 Windows API 版本

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <string>
#include <cstdio>
#include <fstream>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <richedit.h>
#include "resource.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "shell32.lib")

// 应用程序常量
const wchar_t* APP_NAME = L"Nginx 管理器";
const wchar_t* CLASS_NAME = L"NginxManagerWindow";
const wchar_t* CONFIG_FILE = L"nginx-manager.ini";

// Control IDs
#define IDI_ICON1 101
#define ID_PATH_EDIT        1001
#define ID_BROWSE_BUTTON    1002
#define ID_START_BUTTON     1003
#define ID_STOP_BUTTON      1004
#define ID_RESTART_BUTTON   1005
#define ID_CONFIG_BUTTON    1006
#define ID_REFRESH_BUTTON   1007
#define ID_FONT_BUTTON      1008
#define ID_STATUS_TEXT      1009
#define ID_LOG_EDIT         1010

// 字体设置对话框控件ID
#define ID_NORMAL_FONT_EDIT    2001
#define ID_BUTTON_FONT_EDIT    2002
#define ID_LOG_FONT_EDIT       2003
#define ID_APPLY_FONT_BTN      2004
#define ID_CANCEL_FONT_BTN     2005
#define ID_PREVIEW_BTN         2006

// Global variables
HWND g_hMainWnd = NULL;
HWND g_hPathEdit = NULL;
HWND g_hStartBtn = NULL;
HWND g_hStopBtn = NULL;
HWND g_hRestartBtn = NULL;
HWND g_hConfigBtn = NULL;
HWND g_hStatusText = NULL;
HWND g_hLogEdit = NULL;

std::wstring g_nginxPath;

// 状态颜色
COLORREF g_statusColor = RGB(128, 128, 128); // 默认灰色

// 字体配置
struct FontConfig {
    int titleSize = 24;
    int normalSize = 18;
    int buttonSize = 16;
    int logSize = 14;
} g_fontConfig;

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateControls(HWND hwnd);
void LoadConfiguration();
void SaveConfiguration();
void StartNginx();
void StopNginx();
void RestartNginx();
void OpenConfig();
void RefreshStatus();
void BrowseForPath();
void UpdateStatus();
void AddLogMessage(const wchar_t* message);
bool IsNginxRunning();
void ExecuteCommand(const wchar_t* command);
std::wstring StringToWString(const std::string& str);
std::string WStringToString(const std::wstring& wstr);
void SetButtonStyle(HWND hButton, COLORREF bgColor, COLORREF textColor);
void ApplyModernStyling();
void SetStatusColor(COLORREF color);
void AddColoredLogMessage(const wchar_t* message, COLORREF color);
void LoadFontConfiguration();
void SaveFontConfiguration();
void ShowFontSettings();
void ApplyFontSettings();
void RefreshAllFonts();
void SetStatusTextSafe(const wchar_t* text);
void UpdateFontPreview(HWND hDlg);
INT_PTR CALLBACK FontSettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Main program entry
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Set console code page to UTF-8
    SetConsoleOutputCP(CP_UTF8);

    // Register window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON)); // 加载自定义图标
    if (!wc.hIcon) {
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); // 如果失败则使用默认图标
    }

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"窗口注册失败！", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create main window with improved size and positioning
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = 520;  // 减小宽度以适应新的紧凑布局
    int windowHeight = 580; // 增加高度以适应两行按钮
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    g_hMainWnd = CreateWindowW(
        CLASS_NAME, APP_NAME,
        WS_OVERLAPPEDWINDOW,
        x, y, windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hMainWnd) {
        MessageBoxW(NULL, L"窗口创建失败！", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    LoadConfiguration();
    AddColoredLogMessage(L"Nginx 管理器已启动", RGB(0, 100, 200)); // 蓝色
    UpdateStatus();

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            CreateControls(hwnd);
            return 0;

        case WM_COMMAND: {
            WORD wmId = LOWORD(wParam);
            switch (wmId) {
                case ID_START_BUTTON:
                    StartNginx();
                    break;
                case ID_STOP_BUTTON:
                    StopNginx();
                    break;
                case ID_RESTART_BUTTON:
                    RestartNginx();
                    break;
                case ID_CONFIG_BUTTON:
                    OpenConfig();
                    break;
                case ID_REFRESH_BUTTON:
                    RefreshStatus();
                    break;
                case ID_FONT_BUTTON:
                    ShowFontSettings();
                    break;
                case ID_BROWSE_BUTTON:
                    BrowseForPath();
                    break;
                case ID_PATH_EDIT:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        wchar_t buffer[MAX_PATH];
                        GetWindowTextW(g_hPathEdit, buffer, MAX_PATH);
                        g_nginxPath = buffer;
                        SaveConfiguration();
                    }
                    break;
            }
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND hControl = (HWND)lParam;

            if (hControl == g_hStatusText) {
                SetTextColor(hdc, g_statusColor);
                SetBkMode(hdc, TRANSPARENT);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }
            break;
        }

        case WM_SIZE: {
            RECT rect;
            GetClientRect(hwnd, &rect);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            if (g_hPathEdit) {
                // 重新调整控件位置和大小以适应新的窗口尺寸
                SetWindowPos(g_hPathEdit, NULL, 20, 45, width - 120, 32, SWP_NOZORDER);
                SetWindowPos(GetDlgItem(hwnd, ID_BROWSE_BUTTON), NULL, width - 90, 45, 80, 32, SWP_NOZORDER);
                SetWindowPos(g_hStatusText, NULL, 110, 95, width - 130, 20, SWP_NOZORDER);

                // 日志区域自适应大小
                SetWindowPos(g_hLogEdit, NULL, 20, 215, width - 40, height - 235, SWP_NOZORDER);
            }
            return 0;
        }

        case WM_GETMINMAXINFO: {
            LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
            lpMMI->ptMinTrackSize.x = 520; // 最小宽度 - 适应新的按钮布局
            lpMMI->ptMinTrackSize.y = 580; // 最小高度 - 适应两行按钮
            return 0;
        }



        case WM_DESTROY:
            SaveConfiguration();
            SaveFontConfiguration();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 创建界面控件
void CreateControls(HWND hwnd) {
    // 加载字体配置
    LoadFontConfiguration();

    // 创建现代化字体 - 使用配置值
    HFONT hTitleFont = CreateFontW(
        g_fontConfig.titleSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
    );

    HFONT hNormalFont = CreateFontW(
        g_fontConfig.normalSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
    );

    HFONT hButtonFont = CreateFontW(
        g_fontConfig.buttonSize, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
    );

    // nginx 路径配置区域 (移除标题，向上移动)
    HWND hPathLabel = CreateWindowW(L"STATIC", L"Nginx 安装路径:",
                                   WS_CHILD | WS_VISIBLE,
                                   20, 20, 120, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
    SendMessage(hPathLabel, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    g_hPathEdit = CreateWindowW(L"EDIT", L"",
                               WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                               20, 45, 640, 32, hwnd, (HMENU)ID_PATH_EDIT, GetModuleHandle(NULL), NULL);
    SendMessage(g_hPathEdit, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    HWND hBrowseBtn = CreateWindowW(L"BUTTON", L"浏览...",
                                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                   670, 45, 80, 32, hwnd, (HMENU)ID_BROWSE_BUTTON, GetModuleHandle(NULL), NULL);
    SendMessage(hBrowseBtn, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

    // 状态显示区域
    HWND hStatusLabel = CreateWindowW(L"STATIC", L"服务状态:",
                                     WS_CHILD | WS_VISIBLE,
                                     20, 95, 80, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
    SendMessage(hStatusLabel, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    g_hStatusText = CreateWindowW(L"STATIC", L"未知",
                                 WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
                                 110, 95, 150, 20, hwnd, (HMENU)ID_STATUS_TEXT, GetModuleHandle(NULL), NULL);
    SendMessage(g_hStatusText, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // 控制按钮区域 - 优化布局为两行
    // 第一行：主要服务控制按钮
    g_hStartBtn = CreateWindowW(L"BUTTON", L"🚀 启动服务",
                               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                               20, 130, 110, 35, hwnd, (HMENU)ID_START_BUTTON, GetModuleHandle(NULL), NULL);
    SendMessage(g_hStartBtn, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

    g_hStopBtn = CreateWindowW(L"BUTTON", L"⏹️ 停止服务",
                              WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                              140, 130, 110, 35, hwnd, (HMENU)ID_STOP_BUTTON, GetModuleHandle(NULL), NULL);
    SendMessage(g_hStopBtn, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

    g_hRestartBtn = CreateWindowW(L"BUTTON", L"🔄 重启服务",
                                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                 260, 130, 110, 35, hwnd, (HMENU)ID_RESTART_BUTTON, GetModuleHandle(NULL), NULL);
    SendMessage(g_hRestartBtn, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

    HWND hRefreshBtn = CreateWindowW(L"BUTTON", L"🔍 刷新状态",
                                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                    380, 130, 110, 35, hwnd, (HMENU)ID_REFRESH_BUTTON, GetModuleHandle(NULL), NULL);
    SendMessage(hRefreshBtn, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

    // 第二行：配置和工具按钮
    g_hConfigBtn = CreateWindowW(L"BUTTON", L"⚙️ 打开配置",
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                20, 175, 110, 35, hwnd, (HMENU)ID_CONFIG_BUTTON, GetModuleHandle(NULL), NULL);
    SendMessage(g_hConfigBtn, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

    HWND hFontBtn = CreateWindowW(L"BUTTON", L"🎨 字体设置",
                                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                 140, 175, 110, 35, hwnd, (HMENU)ID_FONT_BUTTON, GetModuleHandle(NULL), NULL);
    SendMessage(hFontBtn, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

    // 日志区域 - 调整位置以适应两行按钮
    HWND hLogLabel = CreateWindowW(L"STATIC", L"操作日志:",
                                  WS_CHILD | WS_VISIBLE,
                                  20, 225, 100, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
    SendMessage(hLogLabel, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // 加载 Rich Edit 库
    LoadLibraryW(L"riched20.dll");

    g_hLogEdit = CreateWindowW(RICHEDIT_CLASSW, L"",
                              WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                              20, 250, 740, 285, hwnd, (HMENU)ID_LOG_EDIT, GetModuleHandle(NULL), NULL);

    // 设置日志字体为等宽字体 - 使用配置值
    HFONT hLogFont = CreateFontW(
        g_fontConfig.logSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas"
    );
    SendMessage(g_hLogEdit, WM_SETFONT, (WPARAM)hLogFont, TRUE);

    // 应用现代化样式
    ApplyModernStyling();
}

// 加载配置
void LoadConfiguration() {
    // 获取当前程序目录
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring configPath = exePath;
    size_t lastSlash = configPath.find_last_of(L"\\");
    if (lastSlash != std::wstring::npos) {
        configPath = configPath.substr(0, lastSlash + 1) + CONFIG_FILE;
    } else {
        configPath = CONFIG_FILE;
    }

    // 使用 Windows API 读取配置
    wchar_t buffer[MAX_PATH];
    DWORD result = GetPrivateProfileStringW(L"Settings", L"NginxPath", L"", buffer, MAX_PATH, configPath.c_str());

    if (result > 0) {
        g_nginxPath = buffer;
        if (g_hPathEdit) {
            SetWindowTextW(g_hPathEdit, g_nginxPath.c_str());
        }
        std::wstring logMsg = L"已加载配置: " + g_nginxPath;
        AddColoredLogMessage(logMsg.c_str(), RGB(0, 100, 200)); // 蓝色
    } else {
        AddColoredLogMessage(L"未找到配置文件，使用默认设置", RGB(128, 128, 128)); // 灰色
    }
}

// 保存配置
void SaveConfiguration() {
    // 获取当前程序目录
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring configPath = exePath;
    size_t lastSlash = configPath.find_last_of(L"\\");
    if (lastSlash != std::wstring::npos) {
        configPath = configPath.substr(0, lastSlash + 1) + CONFIG_FILE;
    } else {
        configPath = CONFIG_FILE;
    }

    // 使用 Windows API 保存配置
    BOOL result = WritePrivateProfileStringW(L"Settings", L"NginxPath", g_nginxPath.c_str(), configPath.c_str());
    if (result) {
        AddColoredLogMessage(L"配置已保存", RGB(0, 100, 200)); // 蓝色
    } else {
        AddColoredLogMessage(L"配置保存失败", RGB(220, 20, 60)); // 红色
    }
}

// 打开配置文件
void OpenConfig() {
    if (g_nginxPath.empty()) {
        MessageBoxW(g_hMainWnd, L"请先设置 nginx 路径", L"警告", MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring configPath = g_nginxPath + L"\\conf\\nginx.conf";

    // 检查配置文件是否存在
    DWORD attributes = GetFileAttributesW(configPath.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(g_hMainWnd, L"未找到 nginx.conf 配置文件", L"错误", MB_OK | MB_ICONERROR);
        AddColoredLogMessage(L"✗ 错误: 未找到配置文件", RGB(220, 20, 60)); // 红色
        return;
    }

    // 使用默认程序打开配置文件
    HINSTANCE result = ShellExecuteW(NULL, L"open", configPath.c_str(), NULL, NULL, SW_SHOWNORMAL);

    if ((INT_PTR)result <= 32) {
        // 如果默认程序打开失败，尝试用记事本打开
        std::wstring command = L"notepad.exe \"" + configPath + L"\"";
        STARTUPINFOW si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);

        if (CreateProcessW(NULL, (LPWSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            AddColoredLogMessage(L"✓ 已打开配置文件", RGB(34, 139, 34)); // 绿色
        } else {
            MessageBoxW(g_hMainWnd, L"无法打开配置文件", L"错误", MB_OK | MB_ICONERROR);
            AddColoredLogMessage(L"✗ 错误: 无法打开配置文件", RGB(220, 20, 60)); // 红色
        }
    } else {
        AddColoredLogMessage(L"✓ 已打开配置文件", RGB(34, 139, 34)); // 绿色
    }
}

// 刷新状态
void RefreshStatus() {
    UpdateStatus();
    AddColoredLogMessage(L"状态已刷新", RGB(0, 100, 200)); // 蓝色
}

// 浏览文件夹
void BrowseForPath() {
    wchar_t folderPath[MAX_PATH] = {0};

    BROWSEINFOW bi = {};
    bi.hwndOwner = g_hMainWnd;
    bi.lpszTitle = L"请选择 Nginx 安装目录";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        if (SHGetPathFromIDListW(pidl, folderPath)) {
            g_nginxPath = folderPath;
            SetWindowTextW(g_hPathEdit, g_nginxPath.c_str());
            SaveConfiguration();
            std::wstring logMsg = L"已设置 Nginx 路径: " + g_nginxPath;
            AddColoredLogMessage(logMsg.c_str(), RGB(0, 100, 200)); // 蓝色
            UpdateStatus();
        }
        CoTaskMemFree(pidl);
    }
}

// 启动 nginx
void StartNginx() {
    if (g_nginxPath.empty()) {
        MessageBoxW(g_hMainWnd, L"请先设置 nginx 路径", L"警告", MB_OK | MB_ICONWARNING);
        return;
    }

    if (IsNginxRunning()) {
        AddColoredLogMessage(L"Nginx 已经在运行中", RGB(255, 140, 0)); // 橙色
        return;
    }

    // 设置启动中状态
    SetStatusColor(RGB(255, 140, 0)); // 橙色
    SetStatusTextSafe(L"启动中...");

    AddColoredLogMessage(L"正在启动 nginx...", RGB(0, 100, 200)); // 蓝色

    std::wstring command = L"cd /d \"" + g_nginxPath + L"\" && start /B nginx.exe";
    ExecuteCommand(command.c_str());

    Sleep(2000);
    UpdateStatus();

    if (IsNginxRunning()) {
        AddColoredLogMessage(L"✓ Nginx 启动成功", RGB(34, 139, 34)); // 绿色
    } else {
        AddColoredLogMessage(L"✗ Nginx 启动失败", RGB(220, 20, 60)); // 红色
        MessageBoxW(g_hMainWnd, L"Nginx 启动失败，请检查配置和日志", L"错误", MB_OK | MB_ICONERROR);
    }
}

// 停止 nginx
void StopNginx() {
    if (!IsNginxRunning()) {
        AddColoredLogMessage(L"Nginx 未运行", RGB(255, 140, 0)); // 橙色
        return;
    }

    // 设置停止中状态
    SetStatusColor(RGB(255, 140, 0)); // 橙色
    SetStatusTextSafe(L"停止中...");

    AddColoredLogMessage(L"正在停止 nginx...", RGB(0, 100, 200)); // 蓝色
    ExecuteCommand(L"taskkill /F /IM nginx.exe");
    Sleep(1000);
    UpdateStatus();

    if (!IsNginxRunning()) {
        AddColoredLogMessage(L"✓ Nginx 停止成功", RGB(34, 139, 34)); // 绿色
    } else {
        AddColoredLogMessage(L"✗ Nginx 停止失败", RGB(220, 20, 60)); // 红色
        MessageBoxW(g_hMainWnd, L"Nginx 停止失败，可能需要管理员权限", L"错误", MB_OK | MB_ICONERROR);
    }
}

// 重启 nginx
void RestartNginx() {
    if (g_nginxPath.empty()) {
        MessageBoxW(g_hMainWnd, L"请先设置 nginx 路径", L"警告", MB_OK | MB_ICONWARNING);
        return;
    }

    // 设置重启中状态
    SetStatusColor(RGB(255, 140, 0)); // 橙色
    SetStatusTextSafe(L"重启中...");

    AddColoredLogMessage(L"正在重启 nginx...", RGB(0, 100, 200)); // 蓝色

    if (IsNginxRunning()) {
        ExecuteCommand(L"taskkill /F /IM nginx.exe");
        Sleep(1000);
    }

    std::wstring command = L"cd /d \"" + g_nginxPath + L"\" && start /B nginx.exe";
    ExecuteCommand(command.c_str());
    Sleep(2000);

    UpdateStatus();

    if (IsNginxRunning()) {
        AddColoredLogMessage(L"✓ Nginx 重启成功", RGB(34, 139, 34)); // 绿色
    } else {
        AddColoredLogMessage(L"✗ Nginx 重启失败", RGB(220, 20, 60)); // 红色
        MessageBoxW(g_hMainWnd, L"Nginx 重启失败，请检查配置和日志", L"错误", MB_OK | MB_ICONERROR);
    }
}

// 更新状态
void UpdateStatus() {
    bool isRunning = IsNginxRunning();
    std::wstring statusText;

    if (isRunning) {
        statusText = L"运行中        "; // 添加空格确保清除旧文本
        SetStatusColor(RGB(34, 139, 34)); // 绿色
    } else {
        statusText = L"已停止        "; // 添加空格确保清除旧文本
        SetStatusColor(RGB(220, 20, 60)); // 红色
    }

    // 使用安全的状态文本设置函数
    SetStatusTextSafe(statusText.c_str());
}

// 设置状态颜色
void SetStatusColor(COLORREF color) {
    g_statusColor = color;
    if (g_hStatusText) {
        InvalidateRect(g_hStatusText, NULL, TRUE);
    }
}

// 添加日志消息（默认黑色）
void AddLogMessage(const wchar_t* message) {
    AddColoredLogMessage(message, RGB(0, 0, 0)); // 黑色
}

// 添加彩色日志消息
void AddColoredLogMessage(const wchar_t* message, COLORREF color) {
    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t timeStr[32];
    swprintf(timeStr, 32, L"[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);

    // 移动到文本末尾
    SendMessage(g_hLogEdit, EM_SETSEL, -1, -1);

    // 设置时间戳颜色（灰色）
    CHARFORMAT2W cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = RGB(128, 128, 128);
    SendMessage(g_hLogEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessage(g_hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)timeStr);

    // 设置消息颜色
    cf.crTextColor = color;
    SendMessage(g_hLogEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessage(g_hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)message);

    // 添加换行
    SendMessage(g_hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");

    // 滚动到底部
    SendMessage(g_hLogEdit, EM_SCROLLCARET, 0, 0);
}

// 加载字体配置
void LoadFontConfiguration() {
    // 获取当前程序目录
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring configPath = exePath;
    size_t lastSlash = configPath.find_last_of(L"\\");
    if (lastSlash != std::wstring::npos) {
        configPath = configPath.substr(0, lastSlash + 1) + CONFIG_FILE;
    } else {
        configPath = CONFIG_FILE;
    }

    // 读取字体配置
    g_fontConfig.titleSize = GetPrivateProfileIntW(L"Fonts", L"TitleSize", 24, configPath.c_str());
    g_fontConfig.normalSize = GetPrivateProfileIntW(L"Fonts", L"NormalSize", 16, configPath.c_str());
    g_fontConfig.buttonSize = GetPrivateProfileIntW(L"Fonts", L"ButtonSize", 15, configPath.c_str());
    g_fontConfig.logSize = GetPrivateProfileIntW(L"Fonts", L"LogSize", 12, configPath.c_str());

    // 验证字体大小范围
    if (g_fontConfig.titleSize < 12 || g_fontConfig.titleSize > 48) g_fontConfig.titleSize = 24;
    if (g_fontConfig.normalSize < 10 || g_fontConfig.normalSize > 32) g_fontConfig.normalSize = 16;
    if (g_fontConfig.buttonSize < 10 || g_fontConfig.buttonSize > 32) g_fontConfig.buttonSize = 15;
    if (g_fontConfig.logSize < 8 || g_fontConfig.logSize > 24) g_fontConfig.logSize = 12;
}

// 保存字体配置
void SaveFontConfiguration() {
    // 获取当前程序目录
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring configPath = exePath;
    size_t lastSlash = configPath.find_last_of(L"\\");
    if (lastSlash != std::wstring::npos) {
        configPath = configPath.substr(0, lastSlash + 1) + CONFIG_FILE;
    } else {
        configPath = CONFIG_FILE;
    }

    // 保存字体配置
    wchar_t buffer[16];
    swprintf(buffer, 16, L"%d", g_fontConfig.titleSize);
    WritePrivateProfileStringW(L"Fonts", L"TitleSize", buffer, configPath.c_str());

    swprintf(buffer, 16, L"%d", g_fontConfig.normalSize);
    WritePrivateProfileStringW(L"Fonts", L"NormalSize", buffer, configPath.c_str());

    swprintf(buffer, 16, L"%d", g_fontConfig.buttonSize);
    WritePrivateProfileStringW(L"Fonts", L"ButtonSize", buffer, configPath.c_str());

    swprintf(buffer, 16, L"%d", g_fontConfig.logSize);
    WritePrivateProfileStringW(L"Fonts", L"LogSize", buffer, configPath.c_str());
}

// 检查 nginx 是否运行
bool IsNginxRunning() {
    STARTUPINFOW si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    wchar_t cmdLine[] = L"cmd /c tasklist | findstr /i nginx.exe";

    if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 5000);

        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return (exitCode == 0);
    }

    return false;
}

// 执行命令
void ExecuteCommand(const wchar_t* command) {
    STARTUPINFOW si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    size_t cmdLen = wcslen(command) + 10;
    wchar_t* cmdLine = new wchar_t[cmdLen];
    wcscpy(cmdLine, L"cmd /c ");
    wcscat(cmdLine, command);

    if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 5000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    delete[] cmdLine;
}

// 字符串转换辅助函数
std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// 应用现代化样式
void ApplyModernStyling() {
    // 设置窗口背景色为浅灰色
    SetClassLongPtr(g_hMainWnd, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(RGB(248, 249, 250)));

    // 设置按钮样式
    SetButtonStyle(g_hStartBtn, RGB(40, 167, 69), RGB(255, 255, 255));    // 绿色
    SetButtonStyle(g_hStopBtn, RGB(220, 53, 69), RGB(255, 255, 255));     // 红色
    SetButtonStyle(g_hRestartBtn, RGB(255, 193, 7), RGB(33, 37, 41));     // 黄色
    SetButtonStyle(g_hConfigBtn, RGB(0, 123, 255), RGB(255, 255, 255));   // 蓝色

    // 设置输入框样式
    SendMessage(g_hPathEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(8, 8));

    // 设置日志区域样式
    SendMessage(g_hLogEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(8, 8));
}

// 设置按钮样式（简化版本，实际效果有限）
void SetButtonStyle(HWND hButton, COLORREF bgColor, COLORREF textColor) {
    // 注意：原生 Windows 按钮样式定制有限
    // 这里主要是为了代码结构的完整性
    // 实际的视觉改进主要通过布局、字体和间距实现
}

// 刷新所有字体
void RefreshAllFonts() {
    // 重新创建控件以应用新字体
    // 这是一个简化的实现，实际上应该重新创建所有控件

    // 强制重绘主窗口以应用字体更改
    InvalidateRect(g_hMainWnd, NULL, TRUE);
    UpdateWindow(g_hMainWnd);

    // 添加日志消息
    AddColoredLogMessage(L"字体设置已更新，重启应用程序以完全生效", RGB(255, 140, 0));

}

// 显示字体设置对话框
void ShowFontSettings() {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FONT_SETTINGS), g_hMainWnd, FontSettingsDialogProc);
}

// 字体设置对话框处理函数
INT_PTR CALLBACK FontSettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static HFONT hPreviewNormalFont = NULL;
    static HFONT hPreviewButtonFont = NULL;
    static HFONT hPreviewLogFont = NULL;

    switch (message) {
        case WM_INITDIALOG: {
            // 设置对话框字体为支持中文的字体
            HFONT hDialogFont = CreateFontW(
                -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
            );

            // 应用字体到所有静态文本控件
            EnumChildWindows(hDlg, [](HWND hChild, LPARAM lParam) -> BOOL {
                wchar_t className[256];
                GetClassNameW(hChild, className, 256);
                if (wcscmp(className, L"Static") == 0 || wcscmp(className, L"Button") == 0) {
                    SendMessage(hChild, WM_SETFONT, (WPARAM)lParam, TRUE);
                }
                return TRUE;
            }, (LPARAM)hDialogFont);

            // 设置当前字体大小到编辑框
            SetDlgItemInt(hDlg, IDC_NORMAL_FONT_EDIT, g_fontConfig.normalSize, FALSE);
            SetDlgItemInt(hDlg, IDC_BUTTON_FONT_EDIT, g_fontConfig.buttonSize, FALSE);
            SetDlgItemInt(hDlg, IDC_LOG_FONT_EDIT, g_fontConfig.logSize, FALSE);

            // 设置编辑框输入限制
            SendDlgItemMessage(hDlg, IDC_NORMAL_FONT_EDIT, EM_SETLIMITTEXT, 2, 0);
            SendDlgItemMessage(hDlg, IDC_BUTTON_FONT_EDIT, EM_SETLIMITTEXT, 2, 0);
            SendDlgItemMessage(hDlg, IDC_LOG_FONT_EDIT, EM_SETLIMITTEXT, 2, 0);

            // 初始化预览
            UpdateFontPreview(hDlg);
            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_PREVIEW_BTN: {
                    UpdateFontPreview(hDlg);
                    break;
                }

                case IDOK: {
                    // 验证并应用字体设置
                    BOOL success;
                    int normalSize = GetDlgItemInt(hDlg, IDC_NORMAL_FONT_EDIT, &success, FALSE);
                    if (!success || normalSize < 8 || normalSize > 72) {
                        MessageBox(hDlg, L"普通文本字体大小必须在8-72之间", L"输入错误", MB_OK | MB_ICONERROR);
                        SetFocus(GetDlgItem(hDlg, IDC_NORMAL_FONT_EDIT));
                        break;
                    }

                    int buttonSize = GetDlgItemInt(hDlg, IDC_BUTTON_FONT_EDIT, &success, FALSE);
                    if (!success || buttonSize < 8 || buttonSize > 72) {
                        MessageBox(hDlg, L"按钮文本字体大小必须在8-72之间", L"输入错误", MB_OK | MB_ICONERROR);
                        SetFocus(GetDlgItem(hDlg, IDC_BUTTON_FONT_EDIT));
                        break;
                    }

                    int logSize = GetDlgItemInt(hDlg, IDC_LOG_FONT_EDIT, &success, FALSE);
                    if (!success || logSize < 8 || logSize > 72) {
                        MessageBox(hDlg, L"日志文本字体大小必须在8-72之间", L"输入错误", MB_OK | MB_ICONERROR);
                        SetFocus(GetDlgItem(hDlg, IDC_LOG_FONT_EDIT));
                        break;
                    }

                    // 保存新的字体设置
                    g_fontConfig.normalSize = normalSize;
                    g_fontConfig.buttonSize = buttonSize;
                    g_fontConfig.logSize = logSize;

                    SaveFontConfiguration();
                    AddColoredLogMessage(L"字体设置已保存，重启应用程序以完全生效", RGB(0, 100, 200));

                    // 清理预览字体
                    if (hPreviewNormalFont) DeleteObject(hPreviewNormalFont);
                    if (hPreviewButtonFont) DeleteObject(hPreviewButtonFont);
                    if (hPreviewLogFont) DeleteObject(hPreviewLogFont);

                    EndDialog(hDlg, IDOK);
                    break;
                }

                case IDCANCEL:
                    // 清理预览字体
                    if (hPreviewNormalFont) DeleteObject(hPreviewNormalFont);
                    if (hPreviewButtonFont) DeleteObject(hPreviewButtonFont);
                    if (hPreviewLogFont) DeleteObject(hPreviewLogFont);

                    EndDialog(hDlg, IDCANCEL);
                    break;
            }
            break;

        case WM_CLOSE:
            // 清理预览字体
            if (hPreviewNormalFont) DeleteObject(hPreviewNormalFont);
            if (hPreviewButtonFont) DeleteObject(hPreviewButtonFont);
            if (hPreviewLogFont) DeleteObject(hPreviewLogFont);

            EndDialog(hDlg, IDCANCEL);
            break;
    }

    return FALSE;
}

// 安全设置状态文本，避免重叠问题
void SetStatusTextSafe(const wchar_t* text) {
    if (!g_hStatusText || !text) return;

    // 彻底清除并重绘状态文本以避免重叠
    SetWindowTextW(g_hStatusText, L"");

    // 获取控件区域并强制重绘背景
    RECT rect;
    GetClientRect(g_hStatusText, &rect);

    // 使用父窗口的背景色填充控件区域
    HDC hdc = GetDC(g_hStatusText);
    HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);
    ReleaseDC(g_hStatusText, hdc);

    // 强制重绘控件
    InvalidateRect(g_hStatusText, NULL, TRUE);
    UpdateWindow(g_hStatusText);

    // 短暂延迟确保重绘完成
    Sleep(10);

    // 设置新文本
    SetWindowTextW(g_hStatusText, text);

    // 再次强制重绘确保新文本正确显示
    InvalidateRect(g_hStatusText, NULL, TRUE);
    UpdateWindow(g_hStatusText);
}

// 更新字体预览
void UpdateFontPreview(HWND hDlg) {
    static HFONT hPreviewNormalFont = NULL;
    static HFONT hPreviewButtonFont = NULL;
    static HFONT hPreviewLogFont = NULL;

    // 清理旧字体
    if (hPreviewNormalFont) DeleteObject(hPreviewNormalFont);
    if (hPreviewButtonFont) DeleteObject(hPreviewButtonFont);
    if (hPreviewLogFont) DeleteObject(hPreviewLogFont);

    // 获取输入的字体大小
    BOOL success;
    int normalSize = GetDlgItemInt(hDlg, IDC_NORMAL_FONT_EDIT, &success, FALSE);
    if (!success || normalSize < 8 || normalSize > 72) normalSize = g_fontConfig.normalSize;

    int buttonSize = GetDlgItemInt(hDlg, IDC_BUTTON_FONT_EDIT, &success, FALSE);
    if (!success || buttonSize < 8 || buttonSize > 72) buttonSize = g_fontConfig.buttonSize;

    int logSize = GetDlgItemInt(hDlg, IDC_LOG_FONT_EDIT, &success, FALSE);
    if (!success || logSize < 8 || logSize > 72) logSize = g_fontConfig.logSize;

    // 创建预览字体
    hPreviewNormalFont = CreateFontW(
        normalSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
    );

    hPreviewButtonFont = CreateFontW(
        buttonSize, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
    );

    hPreviewLogFont = CreateFontW(
        logSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas"
    );

    // 应用预览字体
    SendDlgItemMessage(hDlg, IDC_PREVIEW_NORMAL, WM_SETFONT, (WPARAM)hPreviewNormalFont, TRUE);
    SendDlgItemMessage(hDlg, IDC_PREVIEW_BUTTON, WM_SETFONT, (WPARAM)hPreviewButtonFont, TRUE);
    SendDlgItemMessage(hDlg, IDC_PREVIEW_LOG, WM_SETFONT, (WPARAM)hPreviewLogFont, TRUE);
}
