#include "stdafx.h"
#define APPLICATION_INSTANCE_MUTEX_NAME _TEXT("{ED7533BA-36BE-497C-86A7-4EAEBAF765AF}")

APPBARDATA msgData;

WINDOWPLACEMENT placement;

HWND maxWindow;

POINT cursor;

bool tryShowBar;

bool IsCursorOverTaskbar();

DWORD uwpPID;

DWORD windowPID;

bool SetuwpPID();

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (IsWindowVisible(hwnd) == FALSE)
        return TRUE;
    GetWindowPlacement(hwnd, &placement);
    if (placement.showCmd != SW_MAXIMIZE)
        return TRUE;
    GetWindowThreadProcessId(hwnd, &windowPID);
    if (uwpPID == windowPID)
        return TRUE;
    maxWindow = hwnd;
    return FALSE;
}


int main()
{
    //https://stackoverflow.com/questions/171213/how-to-block-running-two-instances-of-the-same-program
    HANDLE hMutexOneInstance(CreateMutex(NULL, TRUE, APPLICATION_INSTANCE_MUTEX_NAME));
    if (hMutexOneInstance == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutexOneInstance) {
            ReleaseMutex(hMutexOneInstance);
            CloseHandle(hMutexOneInstance);
        }
        return 0;
    }
    tryShowBar = true;
    msgData.cbSize = sizeof(APPBARDATA);
    placement.length = sizeof(WINDOWPLACEMENT);
    SHAppBarMessage(ABM_GETTASKBARPOS, &msgData);
    while (true) {
        while (IsCursorOverTaskbar())
            Sleep(250);
        EnumWindows(EnumWindowsProc, NULL);
        if (maxWindow == NULL) {
            if (tryShowBar == false) {
                Sleep(375);
                continue;
            }
            tryShowBar = false;
            msgData.lParam = ABS_ALWAYSONTOP;
            SHAppBarMessage(ABM_SETSTATE, &msgData);
            Sleep(500);
            continue;
        }
        if (uwpPID == NULL)
            if (SetuwpPID()) {
                maxWindow = NULL;
                continue;
            }
        SHAppBarMessage(ABM_GETTASKBARPOS, &msgData);
        msgData.lParam = ABS_AUTOHIDE;
        SHAppBarMessage(ABM_SETSTATE, &msgData);
        do {
            Sleep(500);
            if (IsWindowVisible(maxWindow) == FALSE)
                break;
            GetWindowPlacement(maxWindow, &placement);
        } while (placement.showCmd == SW_MAXIMIZE);
        tryShowBar = true;
        maxWindow = NULL;
    }
    return 0;
}

inline bool IsCursorOverTaskbar()
{
    GetCursorPos(&cursor);
    switch (msgData.uEdge)
    {
    case ABE_BOTTOM:
        if (cursor.y >= msgData.rc.top)
            return true;
        break;
    case ABE_LEFT:
        if (cursor.x <= msgData.rc.right)
            return true;
        break;
    case ABE_TOP:
        if (cursor.y <= msgData.rc.bottom)
            return true;
        break;
    default:
        if (cursor.x >= msgData.rc.left)
            return true;
        break;
    }
    return false;
}

bool SetuwpPID()
{
    //https://stackoverflow.com/questions/865152/how-can-i-get-a-process-handle-by-its-name-in-c
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry) == TRUE)
        while (Process32Next(snapshot, &entry) == TRUE)
            if (_tcsicmp(entry.szExeFile, TEXT("ApplicationFrameHost.exe")) == 0) {
                uwpPID = entry.th32ProcessID;
                CloseHandle(snapshot);
                return true;
            }
    CloseHandle(snapshot);
    return false;
}
