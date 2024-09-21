// csim-windows-launcher.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "csim-windows-launcher.h"
#include <iostream>
#include <sstream>

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_LOADSTRING 100
#define MAX_WINDOW_CHILDREN 256 //The max amount of children allowed per window.

typedef HWND(*cwexaCallback)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name


//String builder info
std::wstring AppName = L"csim.exe";
std::wstring Mode = L"GAME_OF_LIFE";
std::wstring WinX = L"1280";
std::wstring WinY = L"720";
std::wstring CanvasX = L"80";
std::wstring CanvasY = L"60";
std::wstring FilePath = L"";
TCHAR SelectedItem[256];

//The struct that stores information about a child window.
struct WindowChildElement {
    std::string name = "";
    HWND childHandle = 0;
};


int nChilds = 0; //Current size of the Child window handle list.
WindowChildElement ChildList[MAX_WINDOW_CHILDREN]; //List of child window handles.

TCHAR ModeOptions[6][256] = {
    TEXT("Conway's Game of Life\0"), TEXT("Brian's Brain\0"), TEXT("Life Without Death\0"),
    TEXT("Day and Night\0"), TEXT("Highlife\0"), TEXT("Seeds\0")
};

/*
Appends a child window handle to the child window list. 

Please use this if you don't want to break the program.
*/
void appendChildWindow(HWND handle, std::string winName) {
    if (nChilds >= MAX_WINDOW_CHILDREN-1) {
        std::cout << "MAX NUMBER OF WINDOW CHILDREN REACHED!!!\n\n";
        return;
    }

    ChildList[nChilds].childHandle = handle;
    ChildList[nChilds++].name = winName;
}


/*
Returns a child window handle based on the target name given.

In the case that there are multiple child window elements withe the same name, the first instance is returned

TODO: Change from linear search to binary search. Its fast enough for now...

Returns NULL if no child window was found. (So you probably check for that when calling...)
*/
HWND findChildByName(std::string target) {
    int n = 0;
    while (n < MAX_WINDOW_CHILDREN) {
        if (ChildList[n].name == target) {
            return ChildList[n].childHandle;
        }
        else {
            n++;
        }
    }
    return NULL; //Check for NULL to handle the error.
}

//Window dimension constants.
const int WINWIDTH = 480;
const int WINHEIGHT = 280;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

//The main functions. Read the Microsoft docs if you need more information on how WinMain works...
//https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{

#ifndef NDEBUG
    //Summon a console window for debugging purposes
    if (!AllocConsole()) {
        return -1;
    }
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout); // Attach stdout to the summoned console
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr); // Attach stderr to the summoned console
#endif
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CSIMWINDOWSLAUNCHER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CSIMWINDOWSLAUNCHER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
#ifndef NDEBUG
    FreeConsole();
#endif
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CSIMWINDOWSLAUNCHER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CSIMWINDOWSLAUNCHER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   //Summon the main window.

   //"WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME" is a cheesy hack to make the window somewhat not resizeable.
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,
      CW_USEDEFAULT, 0, WINWIDTH, WINHEIGHT, nullptr, nullptr, hInstance, nullptr);

   //Disable minimizing and miximizing buttons on the window
   SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) & ~WS_MINIMIZEBOX & ~WS_MAXIMIZEBOX);

   //Disable the menubar
   SetMenu(hWnd, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   //Center the window on the user's screen before showing it.
   SetWindowPos(hWnd, nullptr, GetSystemMetrics(SM_CXSCREEN) / 2 - WINWIDTH/2, GetSystemMetrics(SM_CYSCREEN) / 2 - WINHEIGHT/2, WINWIDTH, WINHEIGHT, SWP_NOSIZE | SWP_NOZORDER);

   //Show the window to the user...
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HBRUSH hbrBkgnd{}; //Allocate a hardware brush to get rid of the ugly grey background behind the static child windows

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);

            if (HIWORD(wParam) == CBN_SELCHANGE) {
                int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                
                (TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)SelectedItem);
            }
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case BN_CLICKED:
                if ((HWND)lParam == findChildByName("LaunchButton"))
                {
                    //std::cout << "I CLIKED A BUTTON!!!\n";
                    //Build the cmd string and launch the application
                    STARTUPINFO sinfo;
                    PROCESS_INFORMATION pinfo;

                    GetWindowText(findChildByName("WindowResX"), (LPWSTR)WinX.c_str(), 256);
                    GetWindowText(findChildByName("WindowResY"), (LPWSTR)WinY.c_str(), 256);
                    GetWindowText(findChildByName("CanvasResX"), (LPWSTR)CanvasX.c_str(), 256);
                    GetWindowText(findChildByName("CanvasResY"), (LPWSTR)CanvasY.c_str(), 256);

                    if (!lstrcmpW(SelectedItem, ModeOptions[0])) {
                        Mode = L"GAME_OF_LIFE";
                    }
                    else if (!lstrcmpW(SelectedItem, ModeOptions[1])) {
                        Mode = L"BRIANS_BRAIN";
                    }
                    else if (!lstrcmpW(SelectedItem, ModeOptions[2])) {
                        Mode = L"LIFE_WITHOUT_DEATH";
                    }
                    else if (!lstrcmpW(SelectedItem, ModeOptions[3])) {
                        Mode = L"DAY_AND_NIGHT";
                    }
                    else if (!lstrcmpW(SelectedItem, ModeOptions[4])) {
                        Mode = L"HIGHLIFE";
                    }
                    else if (!lstrcmpW(SelectedItem, ModeOptions[5])) {
                        Mode = L"SEEDS";
                    }

                    ZeroMemory(&sinfo, sizeof(sinfo));
                    sinfo.cb = sizeof(sinfo);
                    ZeroMemory(&pinfo, sizeof(pinfo));
                    std::wstringstream cmdStream;
                    //Do not include file path for now.
                    cmdStream << AppName << L" " << Mode << L" " << L"-ww" << L" " << WinX << L" " << L"-wh" << L" " << WinY << L" " << L"-cw" << L" " << CanvasX << L" " << L"-ch" << L" " << CanvasY;
                    std::wcout << cmdStream.str() << std::endl;

                    /*
                        Attempt to spawn a new csim instance. If failure occurs, print the appropriate error message/summon error dialog box
                    */
                    if (!CreateProcess(NULL, (LPWSTR)cmdStream.str().c_str(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &sinfo, &pinfo))
                    {
                        #ifndef NDEBUG
                        std::wcerr << "ERROR: Failed to open csim instance!! Check Arguements!!!\n";
                        #else
                        //open message box here...
                        #endif
                    }
                    //Wait for the csim instance to finish...
                    PostQuitMessage(0);
                    CloseHandle(pinfo.hProcess);
                    CloseHandle(pinfo.hThread);
                }
                else if ((HWND)lParam == findChildByName("CanvasFileOpen")) {
                    //Open a open file dialog, and populate the edit box with the returning file path.
                    #ifndef NDEBUG
                    std::cout << "WARN: The currently selected file\n will not be loaded" << 
                        " into the program as no file loading features\n have been" <<
                        " impemented into csim at this time...\n" <<
                        "Apologies for the inconvience...\n";
                    #endif

                    OPENFILENAMEW ofn;

                    wchar_t szFile[256];

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = NULL;
                    ofn.lpstrFile = szFile;
                    ofn.lpstrFile[0] = '\0';
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = L"CSim File Format (.csim)\0*.CSIM\0";
                    ofn.nFilterIndex = 1;
                    ofn.lpstrFileTitle = NULL;
                    ofn.nMaxFileTitle = 0;
                    ofn.lpstrInitialDir = NULL;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                    GetOpenFileNameW(&ofn);
                    SendMessage(findChildByName("CanvasFilePath"), WM_SETTEXT, (WPARAM)nullptr, (LPARAM)ofn.lpstrFile);
                }
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_CTLCOLORSTATIC:
    {
        //Remove the ugly greyish color background from the static labels.
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkColor(hdcStatic, RGB(255, 255, 255));
        if (hbrBkgnd == NULL) {
            hbrBkgnd = CreateSolidBrush(RGB(255, 255, 255));
        }
        return (INT_PTR)hbrBkgnd;
    }
    case WM_CREATE:
    {
        //Create a default font so the window elements dont look like they came straight out of win 3.1.
        HFONT defaultFont;
        defaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);


        //Populate the child window list with the ui elements.
        appendChildWindow(CreateWindowExA(0, "Static", "CSim Launcher", WS_CHILD | WS_VISIBLE, WINWIDTH / 2 - 65, 10, 100, 20, hWnd, nullptr, nullptr, nullptr), "MainTitle");
        appendChildWindow(CreateWindowExA(0, "Static", "Rule Set", WS_CHILD | WS_VISIBLE, 305, 30, 100, 20, hWnd, nullptr, nullptr, nullptr), "ModeSelectLabel");
        appendChildWindow(CreateWindowExA(0, "ComboBox", "", WS_BORDER | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS, 305, 50, 150, 180, hWnd, nullptr, nullptr, nullptr), "ModeSelect");
        appendChildWindow(CreateWindowExA(0, "Button", "Launch!", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_CENTER, WINWIDTH / 2 - 55, WINHEIGHT - 80, 75, 25, hWnd, nullptr, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), nullptr), "LaunchButton");
        appendChildWindow(CreateWindowExA(0, "Static", "Window Resolution", WS_CHILD | WS_VISIBLE, 20, 30, 120, 20, hWnd, nullptr, nullptr, nullptr), "ResolutionLabel");
        appendChildWindow(CreateWindowExA(0, "EDIT", "1280", WS_BORDER | WS_CHILD | WS_VISIBLE, 15, 50, 50, 20, hWnd, nullptr, nullptr, nullptr), "WindowResX");
        appendChildWindow(CreateWindowExA(0, "Static", "X", WS_CHILD | WS_VISIBLE, 70, 50, 20, 20, hWnd, nullptr, nullptr, nullptr), "x");
        appendChildWindow(CreateWindowExA(0, "EDIT", "720", WS_BORDER | WS_CHILD | WS_VISIBLE, 85, 50, 50, 20, hWnd, nullptr, nullptr, nullptr), "WindowResY");
        appendChildWindow(CreateWindowExA(0, "Static", "Canvas Size", WS_CHILD | WS_VISIBLE, 20, 75, 100, 20, hWnd, nullptr, nullptr, nullptr), "CanvasSizeLabel");
        appendChildWindow(CreateWindowExA(0, "EDIT", "80", WS_BORDER | WS_CHILD | WS_VISIBLE, 15, 95, 50, 20, hWnd, nullptr, nullptr, nullptr), "CanvasResX");
        appendChildWindow(CreateWindowExA(0, "Static", "X", WS_CHILD | WS_VISIBLE, 70, 95, 20, 20, hWnd, nullptr, nullptr, nullptr), "x");
        appendChildWindow(CreateWindowExA(0, "EDIT", "60", WS_BORDER | WS_CHILD | WS_VISIBLE, 85, 95, 50, 20, hWnd, nullptr, nullptr, nullptr), "CanvasResY");
        appendChildWindow(CreateWindowExA(0, "Static", "Canvas File", WS_CHILD | WS_VISIBLE, 20, 125, 100, 20, hWnd, nullptr, nullptr, nullptr), "CanvasFileLabel");
        appendChildWindow(CreateWindowExA(0, "EDIT", "", WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 15, 145, 175, 20, hWnd, nullptr, nullptr, nullptr), "CanvasFilePath");
        appendChildWindow(CreateWindowExA(0, "Button", "...", WS_CHILD | WS_VISIBLE | BS_CENTER, 195, 145, 20, 20, hWnd, nullptr, nullptr, nullptr), "CanvasFileOpen");

        //Add string options to the combo box.
        TCHAR A[32];
        int i = 0;
        memset(&A, 0, sizeof(A));

        HWND ModeSelectHandle = findChildByName("ModeSelect");

        for (i = 0; i < 6; i++) {
            wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)ModeOptions[i]);

            SendMessage(ModeSelectHandle, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
        }
        SendMessage(ModeSelectHandle, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

        //Apply modern theming to all of the child widgets.
        for (int i = 0; i < nChilds; i++) {
            SendMessage(ChildList[i].childHandle, WM_SETFONT, WPARAM(defaultFont), TRUE);
        }
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.

//I have no idea what to do with this...
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}