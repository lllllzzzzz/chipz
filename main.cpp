#define WINVER 0x1000

#include <windows.h>
#include <stdio.h>
#include <commctrl.h>

#include "resource.h"
#include "ini.h"
#include "chip8.h"

#define LVM_SETEXTENDEDLISTVIEWSTYLE    (LVM_FIRST + 54)
#define LVS_EX_GRIDLINES                0x00000001
#define LVS_EX_SUBITEMIMAGES            0x00000002
#define LVS_EX_CHECKBOXES               0x00000004
#define LVS_EX_TRACKSELECT              0x00000008
#define LVS_EX_HEADERDRAGDROP           0x00000010
#define LVS_EX_FULLROWSELECT            0x00000020
#define LVS_EX_ONECLICKACTIVATE         0x00000040
#define LVS_EX_TWOCLICKACTIVATE         0x00000080
#define LVS_EX_DOUBLEBUFFER             0x00010000

#define DEBUG

#define INI_PATH                        "C:\\Users\\Luke\\Programming\\C++\\CHIP8Emu\\bin\\Debug\\settings.ini"

#define FRAMES_PER_SECOND               60
#define TIMER_INTERVAL                  17
#define LOW_CPU_INTERVAL                5

#define SHOW_ERROR(msg)                 MessageBox(NULL, msg, "CHIP8 Emulator", MB_ICONERROR | MB_OK);
#define SHOW_SUCCESS(msg)               MessageBox(NULL, msg, "CHIP8 Emulator", MB_ICONASTERISK | MB_OK);

//#define WIN_WIDTH           640
//#define WIN_HEIGHT          320
int WIN_WIDTH = 640;
int WIN_HEIGHT = 320;

#define Hz250                           250
#define Hz500                           500
#define Hz750                           750
#define Hz1000                          1000



/* Function Declarations */
LRESULT CALLBACK MainProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DebuggerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AboutProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void drawBitmap();
void debugStep();
void playBeep();
void updateInput();
void readIni();
void writeIni();
void updateDebugger(HWND hwndDebugger);

/* Global Variables */
Chip8 g_Chip8;

HDC         hDc             = NULL;
HINSTANCE   g_hInst         = NULL;
HBITMAP     g_hBmp          = NULL;
HBITMAP	    g_hBmpOld       = NULL;
HDC	    g_hMemDC        = NULL;
UINT32*	    g_pPixels       = NULL;

HWND        g_hWnd          = NULL;
HWND        g_hWndDebugger  = NULL;
HWND        g_hStatus       = NULL;
HMENU       g_hMenu         = NULL;

MMRESULT timerEvent;

char szClassName[] = "Chip8WindowClass";
char g_szWindowTitle[MAX_PATH] = "CHIP8 Emulator";
char g_szEmulatorRunning[MAX_PATH];
char g_szisGameRunning[MAX_PATH] = " [Paused]";

/* Emulator settings. Read .ini to retrieve values, write to .ini to save values. */
typedef struct emulatorSettings
{
    bool isGamePaused;
//    bool isLowCpuModeEnabled;
    bool isisTurboEnabled;
    bool isPauseInactiveEnabled;
    bool isSoundEnabled;
    bool isDebuggerRunning;
    bool isRomLoaded;

    unsigned int cpuSpeed;
    unsigned int customCpuSpeed;
    unsigned int bgColour, customBgColour;
    unsigned int fgColour, customFgColour;

    char recent0[MAX_PATH];
    char recent1[MAX_PATH];
    char recent2[MAX_PATH];
    char recent3[MAX_PATH];
    char recent4[MAX_PATH];
} emulatorSettings;
emulatorSettings g_emulatorSettings;

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow)
{
    MSG messages;
    WNDCLASSEX wincl;

    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = MainProc;
    wincl.style = CS_DBLCLKS;
    wincl.cbSize = sizeof (WNDCLASSEX);

    wincl.hIcon = LoadIcon (0, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (0, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (0, IDC_ARROW);
    wincl.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
    wincl.cbClsExtra = 0;
    wincl.cbWndExtra = 0;

    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    if (!RegisterClassEx(&wincl)) {
        SHOW_ERROR("Error registering window class!\r\nChip8 Emulator will now terminate.");
        return -1;
    }

    g_hWnd = CreateWindowEx(
           WS_EX_ACCEPTFILES,
           szClassName, g_szWindowTitle,
           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
           CW_USEDEFAULT, CW_USEDEFAULT, WIN_WIDTH + 6, WIN_HEIGHT + 71,
           HWND_DESKTOP, 0, hThisInstance, 0
           );

    g_hStatus = CreateWindowEx(
            0,
            STATUSCLASSNAME, NULL,
            WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0,
            g_hWnd, reinterpret_cast<HMENU>(IDC_MAIN_STATUS), GetModuleHandle(NULL), NULL
            );

    int statusWidths[] = {400, -1};

    SendMessage(g_hStatus, SB_SETPARTS, sizeof (statusWidths) / sizeof (int), reinterpret_cast<LPARAM>(statusWidths));
    SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>("No ROM loaded"));

    if (!g_hWnd) {
        SHOW_ERROR("Error creating window!\r\nChip8 Emulator will now terminate.");
        return -1;
    }

    g_hInst = hThisInstance;

    ShowWindow(g_hWnd, nCmdShow);

    BITMAPINFO bmi;
    bmi.bmiHeader.biSize = sizeof (BITMAPINFO);
    bmi.bmiHeader.biWidth = WIN_WIDTH;
    bmi.bmiHeader.biHeight = -WIN_HEIGHT;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;
    bmi.bmiColors[0].rgbBlue = 0;
    bmi.bmiColors[0].rgbGreen = 0;
    bmi.bmiColors[0].rgbRed = 0;
    bmi.bmiColors[0].rgbReserved = 0;

    HDC hdc = GetDC(g_hWnd);
    g_hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&g_pPixels), 0, 0);
    g_hMemDC = CreateCompatibleDC(hdc);
    g_hBmpOld = static_cast<HBITMAP>(SelectObject(g_hMemDC, g_hBmp));
    ReleaseDC(g_hWnd, hdc);

    while (GetMessage(&messages, 0, 0, 0)) {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    return messages.wParam;
}

void updateInput()
{
    char keyState[16];

//    unsigned char keyMappings[] =
//    {
//        '1', '2', '3', '4',
//        'Q', 'W', 'E', 'R',
//        'A', 'S', 'D', 'F',
//        'Z', 'X', 'C', 'V'
//    };

    int keyList[] =
    {
	0x31,   // '1'
	0x32,   // '2'
	0x33,   // '3'
	0x34,   // '4'
	0x51,   // 'Q'
	0x57,   // 'W'
	0x45,   // 'E'
	0x52,   // 'R'
	0x41,   // 'A'
	0x53,   // 'S'
	0x44,   // 'D'
	0x46,   // 'F'
	0x5A,   // 'Z'
	0x58,   // 'X'
	0x43,   // 'C'
	0x56,   // 'V'
	-1,
    };

    for (int i = 0; keyList[i] != -1; i++) {
	if ((GetAsyncKeyState(keyList[i]) & 0x8000) != 0) {
	    g_Chip8.setFlag(CPU_FLAG_KEYDOWN);
	}

        if (GetAsyncKeyState(keyList[i]) == -32767) {
            g_Chip8.resetFlag(CPU_FLAG_KEYDOWN);
        }

        keyState[i] = (GetAsyncKeyState(keyList[i]) & 0x8000) != 0;
    }

    g_Chip8.setKeys(keyState);
}

void drawBitmap()
{
    unsigned char* screen = new unsigned char[WIN_WIDTH * WIN_HEIGHT];
    unsigned char* gfx = g_Chip8.getVideoMemory();

    int gfx_w, gfx_h, gfx_pitch;

    if (g_Chip8.getFlag(CPU_FLAG_SCHIP)) {
        gfx_w       = SCHIP_VIDMEM_WIDTH;
	gfx_h       = SCHIP_VIDMEM_HEIGHT;
	gfx_pitch   = SCHIP_VIDMEM_WIDTH;
    } else {
	gfx_w       = CHIP8_VIDMEM_WIDTH;
	gfx_h       = CHIP8_VIDMEM_HEIGHT;
	gfx_pitch   = CHIP8_VIDMEM_WIDTH;
    }

    int x1, y1;

    // scale gfx from 128x64/64x32 to 640x320
    for (int y = 0; y < WIN_HEIGHT; ++y) {
	for (int x = 0; x < WIN_WIDTH; ++x) {
		x1 = (x * gfx_w) / WIN_WIDTH;
		y1 = (y * gfx_h) / WIN_HEIGHT;

		screen[(y * WIN_WIDTH) + x] = gfx[(y1 * gfx_pitch) + x1];
	}
    }

    // convert 640 x 320 x 1 gfx to 640 x 320 x 32 bitmap
    for (int y = 0; y < WIN_HEIGHT; ++y) {
    	for (int x = 0; x < WIN_WIDTH; ++x) {
	    if (screen[y * WIN_WIDTH + x] != 0) {
                g_pPixels[y * WIN_WIDTH + x] = g_emulatorSettings.fgColour;
            } else {
                g_pPixels[y * WIN_WIDTH + x] = g_emulatorSettings.bgColour;
            }
        }
    }

    InvalidateRect(g_hWnd, 0, false);
    delete[] screen;
}

void playBeep()
{
    PlaySound(MAKEINTRESOURCE(IDR_WAVE1), g_hInst, SND_RESOURCE | SND_ASYNC);
}

void CALLBACK timerCallback(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
    if (g_emulatorSettings.isGamePaused || g_emulatorSettings.isDebuggerRunning || (g_emulatorSettings.isPauseInactiveEnabled && GetForegroundWindow() != g_hWnd)) {
        return;
    }

    GUITHREADINFO gui;
    gui.cbSize = sizeof (GUITHREADINFO);
    GetGUIThreadInfo(0, &gui);

    if (gui.flags & GUI_INMOVESIZE || gui.flags & GUI_INMENUMODE) {
        return;
    }

//    if (g_emulatorSettings.isTurboEnabled)
//    {
//        g_Chip8.EmulateCycles(99999);
//    }
//    else
//    {
        g_Chip8.emulateCycles(g_emulatorSettings.cpuSpeed / FRAMES_PER_SECOND);
//    }


    if (g_Chip8.getDelayTimer() > 0) {
        g_Chip8.tickDelayTimer();
    }

    if (g_Chip8.getSoundTimer() > 0) {
        if (g_Chip8.getSoundTimer() == 1) {
            if (g_emulatorSettings.isSoundEnabled) {
                playBeep();
            }
        }
        g_Chip8.tickSoundTimer();
    }

    updateInput();

    if (g_Chip8.getFlag(CPU_FLAG_DRAW)) {
        drawBitmap();
        g_Chip8.resetFlag(CPU_FLAG_DRAW);
    }

//    if (g_emulatorSettings.lowCPUMode)
//    {
//        Sleep(LOW_CPU_INTERVAL);
//    }
}

// Update UI (menu, window status)
void updateUi(HMENU hMenu)
{
    EnableMenuItem(g_hMenu, IDM_CLOSE,                  (g_emulatorSettings.isRomLoaded)                ? MF_ENABLED : MF_DISABLED);
    EnableMenuItem(g_hMenu, IDM_RESET,                  (g_emulatorSettings.isRomLoaded)                ? MF_ENABLED : MF_DISABLED);
    EnableMenuItem(g_hMenu, IDM_SAVE_STATE,             (g_emulatorSettings.isRomLoaded)                ? MF_ENABLED : MF_DISABLED);
    EnableMenuItem(g_hMenu, IDM_LOAD_STATE,             (g_emulatorSettings.isRomLoaded)                ? MF_ENABLED : MF_DISABLED);
    EnableMenuItem(g_hMenu, IDM_DEBUGGER,               (g_emulatorSettings.isRomLoaded)                ? MF_ENABLED : MF_DISABLED);

    CheckMenuItem(hMenu, IDM_PAUSE,                     (g_emulatorSettings.isGamePaused)              ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_DEBUGGER,                  (g_emulatorSettings.isDebuggerRunning)          ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_PAUSE_WHEN_INACTIVE,       (g_emulatorSettings.isPauseInactiveEnabled)     ? MF_CHECKED : MF_UNCHECKED);
//    CheckMenuItem(hMenu, IDM_LOW_CPU_MODE,              (g_emulatorSettings.lowCPUMode)                 ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_SOUND_ON,                        (g_emulatorSettings.isSoundEnabled)             ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_DETECT_GAME_OVER,          (g_Chip8.getFlag(CPU_FLAG_DETECTGAMEOVER))      ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_DETECT_COLLISION,          (g_Chip8.getFlag(CPU_FLAG_DETECTCOLLISION))     ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_ENABLE_HORIZONTAL_WRAP,    (g_Chip8.getFlag(CPU_FLAG_HWRAP))               ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_ENABLE_VERTICAL_WRAP,      (g_Chip8.getFlag(CPU_FLAG_VWRAP))               ? MF_CHECKED : MF_UNCHECKED);

    CheckMenuItem(hMenu, IDM_250Hz,                     (g_emulatorSettings.cpuSpeed == Hz250)          ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_500Hz,                     (g_emulatorSettings.cpuSpeed == Hz500)          ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_750Hz,                     (g_emulatorSettings.cpuSpeed == Hz750)          ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_1000Hz,                    (g_emulatorSettings.cpuSpeed == Hz1000)         ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_250Hz,                     (g_emulatorSettings.customCpuSpeed != 0)        ? MF_CHECKED : MF_UNCHECKED);
//    CheckMenuItem(hMenu, IDM_250Hz,                     (g_emulatorSettings.isTurboEnabled)             ? MF_CHECKED : MF_UNCHECKED);
}

// Select filename of ROM to open
char* selectRomFilename()
{
    OPENFILENAME rom;
    char* romPath = new char[MAX_PATH]();

    ZeroMemory(&rom, sizeof (OPENFILENAME));
    rom.lStructSize = sizeof (OPENFILENAME);
    rom.hwndOwner = g_hWnd;
    rom.lpstrFilter = "All files (*.*)\0*.*\0";
    rom.lpstrFile = romPath;
    rom.lpstrTitle = "Open...";
    rom.nMaxFile = MAX_PATH;
    rom.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

    GetOpenFileName(&rom);

    if (romPath[0] == '\0') {
        delete[] romPath;
        return 0;
    }

    return romPath;
}

void setupDebugger(HWND hwndDebugger, unsigned int romSize)
{
    char addressTitle[] = "Address";
    char opcodeTitle[] = "Opcode";
    HWND hwndDisassembler = GetDlgItem(hwndDebugger, IDC_DISASSEMBLER);
//    HWND hwndStack = GetDlgItem(hwndDebugger, IDC_STACK);

    LVCOLUMN LvCol;
    ZeroMemory(&LvCol, sizeof (LVCOLUMN));
    LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    // Set up disassembler
    SendMessage(hwndDisassembler, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    LvCol.pszText = addressTitle;
    LvCol.cx = 50;
    SendMessage(hwndDisassembler, LVM_INSERTCOLUMN, 0,(LPARAM)&LvCol);
    LvCol.pszText = opcodeTitle;
    LvCol.cx = 50;
    SendMessage(hwndDisassembler, LVM_INSERTCOLUMN, 1,(LPARAM)&LvCol);

    // Set up stack
//    SendMessage(hwndStack, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
//    LvCol.pszText = "Offset";
//    LvCol.cx = 50;
//    SendMessage(hwndStack, LVM_INSERTCOLUMN, 0,(LPARAM)&LvCol);
//    LvCol.pszText = "Value";
//    LvCol.cx = 50;
//    SendMessage(hwndStack, LVM_INSERTCOLUMN, 1,(LPARAM)&LvCol);

    //LockWindowUpdate(hwnd);

    LVITEM LvItem;
    memset(&LvItem, 0, sizeof (LVITEM));

    LvItem.mask = LVIF_TEXT;
    LvItem.cchTextMax = 256;
    LvItem.iItem = 0;

    //SendMessage(hwnd, LVM_DELETEALLITEMS, 0, 0);

    char buffer[4];

    for (int i = (romSize + CHIP8_ROM_ADDRESS); i >= CHIP8_ROM_ADDRESS; i -= 2) {
        //printf("%04X: %04X\n", i, g_Chip8.getOpcode(i));

        LvItem.iSubItem = 0;
        sprintf(buffer, "%04X", i);
        LvItem.pszText = buffer;
        SendMessage(hwndDisassembler, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&LvItem));

        LvItem.iSubItem = 1;
        sprintf(buffer, "%04X", g_Chip8.getOpcode(i));
        LvItem.pszText = buffer;
        SendMessage(hwndDisassembler, LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&LvItem));
    }
}

void updateDebugger(HWND hwndDebugger)
{
    HWND hwndDisassembler = GetDlgItem(hwndDebugger, IDC_DISASSEMBLER);
//    HWND hwndStack = GetDlgItem(hwndDebugger, IDC_STACK);
    registers cpuRegs = g_Chip8.getRegisters();
//    unsigned short* cpuStack = g_Chip8.getStack();
    char buffer1[5], buffer2[5];

    printf("%04X: %04X\n", g_Chip8.getProgramCounter(), g_Chip8.getOpcode(g_Chip8.getProgramCounter()));

    LVITEM LvItem;
    LvItem.mask = LVIF_TEXT;

    int nItem = SendMessage(hwndDisassembler, LVM_GETNEXTITEM, -1, 0);
    //printf("%d\n", nItem);
    while (nItem != -1) {
        nItem = SendMessage(hwndDisassembler, LVM_GETNEXTITEM, nItem, 0);

        LvItem.iItem = nItem;
        LvItem.iSubItem = 0;
        LvItem.pszText = buffer1;
        LvItem.cchTextMax = 5;

        sprintf(buffer2, "%04X", g_Chip8.getProgramCounter());

        SendMessage(hwndDisassembler, LVM_GETITEM, 0, reinterpret_cast<LPARAM>(&LvItem));

        //if (atoi(buffer1) == g_Chip8.getProgramCounter())
        if (!strcmp(buffer1, buffer2)) {
            SetFocus(hwndDisassembler);
            ListView_SetItemState(hwndDisassembler, nItem, LVIS_SELECTED | LVIS_FOCUSED, 0x000F);
            ListView_EnsureVisible(hwndDisassembler, nItem, TRUE);
            //ListView_Scroll(hwndDisassembler, 0, -10);
        }
    }

    //LVITEM LvItem;
//    memset(&LvItem, 0, sizeof (LvItem));
//
//    LvItem.mask = LVIF_TEXT;
//    LvItem.cchTextMax = 256;
//    LvItem.iItem = 0;

    LockWindowUpdate(hwndDebugger);

//    SendMessage(hwndStack, LVM_DELETEALLITEMS, 0, 0);
//
//    for (int i = 0; i < 16; i++)
//    {
//        printf("%4X\n", cpuStack[i]);
//
//        LvItem.iSubItem = 0;
//        sprintf(buffer2, "%04X", i*2);
//        LvItem.pszText = buffer2;
//        SendMessage(hwndStack, LVM_INSERTITEM, 0, (LPARAM)&LvItem);
//
//        LvItem.iSubItem = 1;
//        sprintf(buffer2, "%04X", cpuStack[i]);
//        LvItem.pszText = buffer2;
//        SendMessage(hwndStack, LVM_SETITEM, 0, (LPARAM)&LvItem);
//    }

    /*--------------------------------------
       Update registers on debugger window.
      --------------------------------------*/
    // Registers V0-VF
    sprintf(buffer1, "%02X", cpuRegs.V[0x0]);
    SetDlgItemText(hwndDebugger, IDC_V0, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0x1]);
    SetDlgItemText(hwndDebugger, IDC_V1, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0x2]);
    SetDlgItemText(hwndDebugger, IDC_V2, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0x3]);
    SetDlgItemText(hwndDebugger, IDC_V3, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0x4]);
    SetDlgItemText(hwndDebugger, IDC_V4, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0x5]);
    SetDlgItemText(hwndDebugger, IDC_V5, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0x6]);
    SetDlgItemText(hwndDebugger, IDC_V6, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0x7]);
    SetDlgItemText(hwndDebugger, IDC_V7, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0x8]);
    SetDlgItemText(hwndDebugger, IDC_V8, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0x9]);
    SetDlgItemText(hwndDebugger, IDC_V9, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0xA]);
    SetDlgItemText(hwndDebugger, IDC_VA, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0xB]);
    SetDlgItemText(hwndDebugger, IDC_VB, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0xC]);
    SetDlgItemText(hwndDebugger, IDC_VC, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0xD]);
    SetDlgItemText(hwndDebugger, IDC_VD, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0xE]);
    SetDlgItemText(hwndDebugger, IDC_VE, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.V[0xF]);
    SetDlgItemText(hwndDebugger, IDC_VF, buffer1);

    // CPU
    sprintf(buffer1, "%04X", cpuRegs.pc);
    SetDlgItemText(hwndDebugger, IDC_PC, buffer1);
    sprintf(buffer1, "%04X", cpuRegs.opcode);
    SetDlgItemText(hwndDebugger, IDC_OPCODE, buffer1);
    sprintf(buffer1, "%04X", cpuRegs.I);
    SetDlgItemText(hwndDebugger, IDC_I, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.sp);
    SetDlgItemText(hwndDebugger, IDC_SP, buffer1);

    // Timers
    sprintf(buffer1, "%02X", cpuRegs.delayTimer);
    SetDlgItemText(hwndDebugger, IDC_DT, buffer1);
    sprintf(buffer1, "%02X", cpuRegs.soundTimer);
    SetDlgItemText(hwndDebugger, IDC_ST, buffer1);

    LockWindowUpdate(0);
}

void debugStep(HWND hwndDebugger)
{
    g_Chip8.emulateCycles(1);

    g_Chip8.tickDelayTimer();
    g_Chip8.tickSoundTimer();

    if (g_Chip8.getFlag(CPU_FLAG_DRAW)) {
        drawBitmap();
        g_Chip8.resetFlag(CPU_FLAG_DRAW);
    }
}

//void associateFileTypes()
//{
//    HKEY hKey;
//    char szExtension[] = ".c8";
//    char szDescription[] = "Chip8 Rom";
//
//}

static int handler(void* user, const char* section, const char* name, const char* value)
{
    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("CHIP8 Emulator Settings", "customCPUSpeed")){
        g_emulatorSettings.cpuSpeed = g_emulatorSettings.customCpuSpeed = atoi(value);
//    } else if (MATCH("CHIP8 Emulator Settings", "lowCPUMode")) {
//        g_emulatorSettings.lowCPUMode = atoi(value) ? true : false;
//    } else if (MATCH("CHIP8 Emulator Settings", "isTurboEnabled")) {
//        g_emulatorSettings.isTurboEnabled = atoi(value) ? true : false;
    } else if (MATCH("CHIP8 Emulator Settings", "isPauseInactiveEnabled")) {
        g_emulatorSettings.isPauseInactiveEnabled = atoi(value) ? true : false;
    } else if (MATCH("CHIP8 Emulator Settings", "windowX")) {
        WIN_WIDTH = atoi(value);
    } else if (MATCH("CHIP8 Emulator Settings", "windowY")) {
        WIN_HEIGHT = atoi(value);
    } else if (MATCH("CHIP8 Emulator Settings", "backgroundColour")) {
        g_emulatorSettings.bgColour = g_emulatorSettings.customBgColour = strtol(value, 0, 0);
        printf("%X", g_emulatorSettings.bgColour);
    } else if (MATCH("CHIP8 Emulator Settings", "foregroundColour")) {
        g_emulatorSettings.fgColour = g_emulatorSettings.customFgColour = strtol(value, 0, 0);
        printf("%X", g_emulatorSettings.fgColour);
    } else if (MATCH("CHIP8 Emulator Settings", "isSoundEnabled")) {
        g_emulatorSettings.isSoundEnabled = atoi(value) ? true : false;
    } else if (MATCH("CHIP8 Emulator Settings", "recent0")) {
        memcpy(g_emulatorSettings.recent0, value, strlen(value));
//        puts(g_emulatorSettings.recent0);
    } else if (MATCH("CHIP8 Emulator Settings", "recent1")) {
        memcpy(g_emulatorSettings.recent1, value, strlen(value));
//        puts(g_emulatorSettings.recent1);
    } else if (MATCH("CHIP8 Emulator Settings", "recent2")) {
        memcpy(g_emulatorSettings.recent2, value, strlen(value));
//        puts(g_emulatorSettings.recent2);
    } else if (MATCH("CHIP8 Emulator Settings", "recent3")) {
        memcpy(g_emulatorSettings.recent3, value, strlen(value));
//        puts(g_emulatorSettings.recent3);
    } else if (MATCH("CHIP8 Emulator Settings", "recent4")) {
        memcpy(g_emulatorSettings.recent4, value, strlen(value));
//        puts(g_emulatorSettings.recent4);
//    } else if (MATCH("CHIP8 Emulator Settings", "horizontalWrap")) {
//        g_emulatorSettings. = atoi(value) ? true : false;
//    } else if (MATCH("CHIP8 Emulator Settings", "verticalWrap")) {
//        g_emulatorSettings.isPauseInactiveEnabled = atoi(value) ? true : false;
//    } else if (MATCH("CHIP8 Emulator Settings", "detectCollision")) {
//        g_emulatorSettings.isPauseInactiveEnabled = atoi(value) ? true : false;
//    } else if (MATCH("CHIP8 Emulator Settings", "detectGameOver")) {
//        g_emulatorSettings.isPauseInactiveEnabled = atoi(value) ? true : false;
//    } else if (MATCH("CHIP8 Emulator Settings", "debuggerEnabled")) {
//        g_emulatorSettings.isDebuggerRunning = atoi(value) ? true : false;
    } else {
        return 0;
    }
    return 1;
}


/*-------------------------------------------------------------
  Read settings from .ini.
  Create new .ini with default settings if file does not exist.
  -------------------------------------------------------------*/
void readIni(FILE* pINI)
{
    if (ini_parse(INI_PATH, handler, &g_emulatorSettings) < 0) {
        fprintf(stderr, "Can't load 'test.ini'\n");
    }
}

void writeIni(/*FILE* pINI*/ char *romPath)
{
    puts("writing .ini\n");

    FILE* pFile = fopen("C:\\Users\\Luke\\Programming\\C++\\CHIP8Emu\\bin\\Debug\\settings.ini", "w+");

    char setting[MAX_PATH] = {0};

    fwrite("[CHIP8 Emulator Settings]\n\n", 27, 1, pFile);
    fwrite("customCPUSpeed=700\n", 19, 1, pFile);
    fwrite("lowCPUMode=0\n", 13, 1, pFile);
    fwrite("isPauseInactiveEnabled=1\n", 16, 1, pFile);
    fwrite("windowX=640\n", 12, 1, pFile);
    fwrite("windowY=320\n", 12, 1, pFile);
    fwrite("backgroundColour=0x00000000\n", 28, 1, pFile);
    fwrite("foregroundColour=0xFFFFFFFF\n", 28, 1, pFile);
//    fwrite("isSoundEnabled=1\n", 15, 1, pFile);
    sprintf(setting, "isSoundEnabled=%d\n", g_emulatorSettings.isSoundEnabled);
    fwrite(setting, strlen(setting), 1, pFile);
    fwrite("horizontalWrap=1\n", 17, 1, pFile);
    fwrite("verticalWrap=0\n", 15, 1, pFile);
    fwrite("detectCollision=1\n", 18, 1, pFile);
    fwrite("detectGameover=0\n", 17, 1, pFile);
    fwrite("debuggerEnabled=0\n", 18, 1, pFile);

    sprintf(setting, "recent0=%s", romPath);
    fwrite(setting, strlen(setting), 1, pFile);

    fclose(pFile);
}

void createIni(FILE* pINI)
{
    puts("writing .ini\n");

    FILE* pFile = fopen("C:\\Users\\Luke\\Programming\\C++\\CHIP8Emu\\bin\\Debug\\settings.ini", "w+");

    fwrite("[CHIP8 Emulator Settings]\n\n", 27, 1, pFile);
    fwrite("customCPUSpeed=700\n", 19, 1, pFile);
    fwrite("lowCPUMode=0\n", 13, 1, pFile);
    fwrite("isPauseInactiveEnabled=1\n", 16, 1, pFile);
    fwrite("windowX=640\n", 12, 1, pFile);
    fwrite("windowY=320\n", 12, 1, pFile);
    fwrite("backgroundColour=0x00000000\n", 28, 1, pFile);
    fwrite("foregroundColour=0xFFFFFFFF\n", 28, 1, pFile);
    fwrite("isSoundEnabled=1\n", 15, 1, pFile);
    fwrite("horizontalWrap=1\n", 17, 1, pFile);
    fwrite("verticalWrap=0\n", 15, 1, pFile);
    fwrite("detectCollision=1\n", 18, 1, pFile);
    fwrite("detectGameover=0\n", 17, 1, pFile);
    fwrite("debuggerEnabled=0\n", 18, 1, pFile);
    fwrite("recent0=\n", 9, 1, pFile);
    fwrite("recent1=\n", 9, 1, pFile);
    fwrite("recent2=\n", 9, 1, pFile);
    fwrite("recent3=\n", 9, 1, pFile);
    fwrite("recent4=\n", 9, 1, pFile);

    fclose(pFile);
}

bool openRom(char *romPath)
{
    if (!romPath || romPath[0] == '\0' || strlen(romPath) > MAX_PATH) {
        SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>("Error: cannot select filename"));
        return false;
    }

    FILE* pFile = fopen(romPath, "rb");

    fseek(pFile, 0, SEEK_END);
    size_t romSize_ = ftell(pFile);
    rewind(pFile);

    if(romSize_ > CHIP8_MAX_ROM_SIZE) {
        SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>("Error: ROM bigger than 3584 bytes"));
        fclose(pFile);
        return false;
    }

    if (!g_Chip8.initializeCpu()) {
        SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>("Error: cannot initialize emulator"));
        return false;
    } else if (!g_Chip8.loadRom(romPath)) {
        SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>("Error: cannot load ROM"));
        return false;
    }

    return true;
}

// Callback procedure for Debugger window
LRESULT CALLBACK DebuggerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
//    HFONT hFont;
    g_hWndDebugger = hwnd;

    switch (message)
    {
        case WM_INITDIALOG:
            setupDebugger(hwnd, g_Chip8.getRomSize());
            updateDebugger(hwnd);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_STEP:
                    debugStep(hwnd);
                    updateDebugger(g_hWndDebugger);
                    break;

                case IDC_CLOSE:
                    g_emulatorSettings.isDebuggerRunning = false;
                    CheckMenuItem(g_hMenu, IDM_DEBUGGER, MF_UNCHECKED);
                    EndDialog(hwnd, 0);
                    CheckMenuItem(g_hMenu, IDM_CLOSE, MF_DISABLED);
                    break;
            }
            break;

        case WM_KEYDOWN:
        {
            switch(LOWORD(wParam))
            {
                case VK_TAB:
                    g_emulatorSettings.isDebuggerRunning = false;
                    CheckMenuItem(g_hMenu, IDM_DEBUGGER, MF_UNCHECKED);
                    EndDialog(hwnd, 0);
                    break;
            }
            break;
        }
        break;

        case WM_CLOSE:
            g_emulatorSettings.isDebuggerRunning = false;
            updateUi(g_hMenu);
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            g_emulatorSettings.isDebuggerRunning = false;
            updateUi(g_hMenu);
            PostQuitMessage(0);
            break;

        default:
            return FALSE;
    }

    //default:
    //    return DefWindowProc (hwnd, message, wParam, lParam);

    return TRUE;
}

// Callback procedure for About window
LRESULT CALLBACK AboutProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_CLOSE:
                    EndDialog(hwnd, 0);
                    break;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return FALSE;
    }

    //default:
    //    return DefWindowProc (hwnd, message, wParam, lParam);

    return TRUE;
}

// Callback procedure for main window
LRESULT CALLBACK MainProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //HMENU hMenu;
    PAINTSTRUCT ps;
//    char* romPath = (char*) calloc(MAX_PATH, sizeof (char));

    switch (message)
    {
        case WM_CREATE:
        {
            // Set default values for emulator
            g_emulatorSettings.isGamePaused = false;
            g_emulatorSettings.isDebuggerRunning = false;
            g_emulatorSettings.isRomLoaded = false;

            FILE* pIni = fopen("C:\\Users\\Luke\\Programming\\C++\\CHIP8Emu\\bin\\Debug\\settings.ini", "r");

            if (pIni) {
                readIni(pIni);
            } else {
                createIni(pIni);
            }

            fclose(pIni);

//                setupRecentList(GetDlgItem(hwndDlg, IDM_FI));

            g_hMenu = GetMenu(hwnd);
            updateUi(g_hMenu);

//            SetWindowText(hwnd, g_emulatorSettings.g_szNoRomLoaded);

        }
            break;

        case WM_PAINT:
            hDc = BeginPaint(hwnd, &ps);

            if (g_hMemDC != 0) {
                BitBlt(hDc, 0, 0, WIN_WIDTH, WIN_HEIGHT, g_hMemDC, 0, 0, SRCCOPY);
//                StretchBlt(hDc, 0, 0, WIN_WIDTH, WIN_HEIGHT, g_hMemDC, 0, 0, 320, 160, SRCCOPY);
            }

            EndPaint(hwnd, &ps);
            break;

//        case WM_TIMER:
//            drawBitmap();
//            break;

        case WM_DROPFILES:
        {
            HDROP hDrop = reinterpret_cast<HDROP>(wParam);
            char* romPath = new char[MAX_PATH]();

            DragQueryFile(hDrop, 0, static_cast<LPSTR>(romPath), MAX_PATH);

            if(openRom(romPath)) {
                g_emulatorSettings.isRomLoaded = true;

                // Get rom name from path
                char romFilename[MAX_PATH] = {0};
                for (int i = 0, j = strlen(romPath) - 1; romPath[j] != '\\'; i++, j--) {
                    romFilename[i] += romPath[j];
                }
                
                // Rom name is backwards, so reverse string
                char romFilename2[MAX_PATH] = {0};
                for (int i = 0, j = strlen(romFilename) - 1; j >= 0; i++, j--) {
                    romFilename2[i] = romFilename[j];
                }

//                sprintf(g_szWindowTitle, "CHIP8 Emulator - %s", romFilename2);
//                SetWindowText(hwnd, g_szWindowTitle);
                char buffer[128];
                int romSize = g_Chip8.getRomSize();
                sprintf(buffer, "ROM loaded: %d bytes at 0x200", romSize);
                SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
                SendMessage(g_hStatus, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(romFilename2));
                updateUi(g_hMenu);

                //KillTimer(reinterpret_cast<HWND>(timerEvent), TRUE);
                timeKillEvent(timerEvent);
                timerEvent = timeSetEvent(TIMER_INTERVAL, 0, timerCallback, 0, TIME_PERIODIC);
            }

            DragFinish(hDrop);
            delete[] romPath;
        }
            break;

        case WM_COMMAND:
    	    switch(LOWORD(wParam))
            {
                case IDM_OPEN:
                {
                    char *romPath = selectRomFilename();

                    if(openRom(romPath)) {
                        g_emulatorSettings.isRomLoaded = true;

			// Get rom name from path
                        char romFilename[MAX_PATH] = {0};
                        for (int i = 0, j = strlen(romPath) - 1; romPath[j] != '\\'; i++, j--) {
                            romFilename[i] += romPath[j];
                        }
                        
                        // Rom name is backwards, so reverse string
                        char romFilename2[MAX_PATH] = {0};
                        for (int i = 0, j = strlen(romFilename) - 1; j >= 0; i++, j--) {
                            romFilename2[i] = romFilename[j];
                        }

//                        sprintf(g_szWindowTitle, "CHIP8 Emulator - %s", romFilename2);
//                        SetWindowText(hwnd, g_szWindowTitle);
                        char buffer[32];
                        int romSize = g_Chip8.getRomSize();
                        sprintf(buffer, "ROM loaded: %d bytes at 0x200", romSize);
                        SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
                        SendMessage(g_hStatus, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(romFilename2));
                        updateUi(g_hMenu);

                        //KillTimer(reinterpret_cast<HWND>(timerEvent), TRUE);
                        timeKillEvent(timerEvent);
                        timerEvent = timeSetEvent(TIMER_INTERVAL, 0, timerCallback, 0, TIME_PERIODIC);
                    }

//                  writeIni(romPath);
                  free(romPath);
//                    delete[] romPath;
                }
                    break;

                case IDM_CLOSE:
                    //KillTimer(reinterpret_cast<HWND>(timerEvent), TRUE);
                    timeKillEvent(timerEvent);

                    EndDialog(g_hWndDebugger, 0);
                    g_Chip8.initializeCpu();
                    drawBitmap();

                    g_emulatorSettings.isRomLoaded = false;
                    updateUi(g_hMenu);

//                    SetWindowText(hwnd, g_szNoRomLoaded);
                    break;

                case IDM_SAVE_STATE:
                    g_Chip8.saveCpuState();
                    if (g_emulatorSettings.isDebuggerRunning) {
                        updateDebugger(g_hWndDebugger);
                        drawBitmap();
                    }
                    break;

                case IDM_LOAD_STATE:
                    if (g_emulatorSettings.isRomLoaded) {
                        g_Chip8.loadCpuState();

                        if (g_emulatorSettings.isDebuggerRunning) {
                            updateDebugger(g_hWndDebugger);
                            drawBitmap();
                        }
                    }
                    break;

                case IDM_EXIT:
                    ExitProcess(0);
                    break;

                case IDM_PAUSE:
                    if (g_emulatorSettings.isRomLoaded) {
                        g_emulatorSettings.isGamePaused = !g_emulatorSettings.isGamePaused;

                        char buffer[64];
                        unsigned short pc = g_Chip8.getProgramCounter();

                        if (g_emulatorSettings.isGamePaused) {

                            sprintf(buffer, "Emulation paused at 0x%04X", pc);
                            SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
                            CheckMenuItem(g_hMenu, IDM_PAUSE, MF_CHECKED);
                        }
                        else {
                            char buffer[64];
                            sprintf(buffer, "Emulation resumed from 0x%04X", pc);
                            SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
                            CheckMenuItem(g_hMenu, IDM_PAUSE, MF_UNCHECKED);
                        }
                    }
                    break;

                case IDM_RESET:
                    if (g_emulatorSettings.isRomLoaded) {
                        g_Chip8.resetCpu();

                        if (g_emulatorSettings.isDebuggerRunning) {
                            updateDebugger(g_hWndDebugger);
                            drawBitmap();
                        }

                        char buffer[64];
                        sprintf(buffer, "Emulator reset to 0x200");
                        SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
                        CheckMenuItem(g_hMenu, IDM_PAUSE, MF_UNCHECKED);

                        updateUi(g_hMenu);
                    }
                    break;

                case IDM_250Hz:
                    g_emulatorSettings.cpuSpeed = Hz250;
                    updateUi(g_hMenu);
                    break;

                case IDM_500Hz:
                    g_emulatorSettings.cpuSpeed = Hz500;
                    updateUi(g_hMenu);
                    break;

                case IDM_750Hz:
                    g_emulatorSettings.cpuSpeed = Hz750;
                    updateUi(g_hMenu);
                    break;

                case IDM_1000Hz:
                    g_emulatorSettings.cpuSpeed = Hz1000;
                    updateUi(g_hMenu);
                    break;

                case IDM_CUSTOM:
                    g_emulatorSettings.cpuSpeed = g_emulatorSettings.customCpuSpeed;
                    updateUi(g_hMenu);
                    break;

//                case IDM_TURBO:
//                    g_emulatorSettings.isTurboEnabled = true;
//                    updateUi(g_hMenu);
//                break;

                case IDM_PAUSE_WHEN_INACTIVE:
                    g_emulatorSettings.isPauseInactiveEnabled = !g_emulatorSettings.isPauseInactiveEnabled;
//                    CheckMenuItem(g_hMenu, IDM_PAUSE_WHEN_INACTIVE, (g_emulatorSettings.isPauseInactiveEnabled) ? MF_CHECKED : MF_UNCHECKED);
                    updateUi(g_hMenu);
                    break;

//                case IDM_LOW_CPU_MODE:
//                    g_emulatorSettings.lowCPUMode = !g_emulatorSettings.lowCPUMode;
//                    CheckMenuItem(g_hMenu, IDM_LOW_CPU_MODE, (g_emulatorSettings.lowCPUMode) ? MF_CHECKED : MF_UNCHECKED);
//                break;

                case IDM_X1:
                    WIN_WIDTH = 320;
                    WIN_HEIGHT = 160;
                    SetWindowPos(hwnd, HWND_TOP, 0, 0, 320, 160, SWP_NOMOVE);
                    break;

                case IDM_COLOR_DEFAULT:
                    g_emulatorSettings.bgColour = g_emulatorSettings.customBgColour;
                    g_emulatorSettings.fgColour = g_emulatorSettings.customFgColour;
                    break;

                case IDM_COLOR_INVERT:
                {
                    unsigned int temp = g_emulatorSettings.bgColour;
                    g_emulatorSettings.bgColour = g_emulatorSettings.fgColour;
                    g_emulatorSettings.fgColour = temp;
                }
                    break;

                case IDM_COLOR_RANDOM:
                    g_emulatorSettings.bgColour = rand();
                    g_emulatorSettings.fgColour = rand();
                    break;

                case IDM_SOUND_ON:
                    g_emulatorSettings.isSoundEnabled = !g_emulatorSettings.isSoundEnabled;
//                    CheckMenuItem(g_hMenu, IDM_ON, (g_emulatorSettings.isSoundEnabled) ? MF_CHECKED : MF_UNCHECKED);
                    updateUi(g_hMenu);
//                    writeIni(romPath);
                    break;

                case IDM_DETECT_GAME_OVER:
                    g_Chip8.toggleFlag(CPU_FLAG_DETECTGAMEOVER);
                    CheckMenuItem(g_hMenu, IDM_DETECT_GAME_OVER, (g_Chip8.getFlag(CPU_FLAG_DETECTGAMEOVER)) ? MF_CHECKED : MF_UNCHECKED);
                    break;

                case IDM_DETECT_COLLISION:
                    g_Chip8.toggleFlag(CPU_FLAG_DETECTCOLLISION);
                    CheckMenuItem(g_hMenu, IDM_DETECT_COLLISION, (g_Chip8.getFlag(CPU_FLAG_DETECTCOLLISION)) ? MF_CHECKED : MF_UNCHECKED);
                    break;

                case IDM_ENABLE_HORIZONTAL_WRAP:
                    g_Chip8.toggleFlag(CPU_FLAG_HWRAP);
                    CheckMenuItem(g_hMenu, IDM_ENABLE_HORIZONTAL_WRAP, (g_Chip8.getFlag(CPU_FLAG_HWRAP)) ? MF_CHECKED : MF_UNCHECKED);
                    break;

                case IDM_ENABLE_VERTICAL_WRAP:
                    g_Chip8.toggleFlag(CPU_FLAG_VWRAP);
                    CheckMenuItem(g_hMenu, IDM_ENABLE_VERTICAL_WRAP, (g_Chip8.getFlag(CPU_FLAG_VWRAP)) ? MF_CHECKED : MF_UNCHECKED);
                    break;

                case IDM_DEBUGGER:
                    if (g_emulatorSettings.isRomLoaded && !g_emulatorSettings.isDebuggerRunning) {
                        g_hWndDebugger = CreateDialog(GetModuleHandle(0), MAKEINTRESOURCE(IDD_DEBUGGER), 0, reinterpret_cast<DLGPROC>(DebuggerProc));

                        if (!g_hWndDebugger) {
                            MessageBox(NULL, "Error opening debugger!", "CHIP8 Emulator", MB_OK | MB_ICONERROR);
                        } else {
                            g_emulatorSettings.isDebuggerRunning = true;
                            CheckMenuItem(g_hMenu, IDM_DEBUGGER, MF_CHECKED);
                        }
                    } else {
                        EndDialog(g_hWndDebugger, 0);
                        g_emulatorSettings.isDebuggerRunning = false;
                        CheckMenuItem(g_hMenu, IDM_DEBUGGER, MF_UNCHECKED);
                    }
                    break;

                case IDM_ABOUT:
                    //DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_ABOUT), 0, (DLGPROC)AboutProc);
                    CreateDialog(GetModuleHandle(0), MAKEINTRESOURCE(IDD_ABOUT), 0, (DLGPROC)AboutProc);
                    break;
            }
            break;

            case WM_KEYDOWN:
                switch(LOWORD(wParam))
                {
                    case VK_F1:
                        if (g_emulatorSettings.isRomLoaded) {
                            g_emulatorSettings.isGamePaused = !g_emulatorSettings.isGamePaused;

                            char buffer[64];
                            unsigned short pc = g_Chip8.getProgramCounter();

                            if (g_emulatorSettings.isGamePaused) {

                                sprintf(buffer, "Emulation paused at 0x%04X", pc);
                                SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
                                CheckMenuItem(g_hMenu, IDM_PAUSE, MF_CHECKED);
                            } else {
                                char buffer[64];
                                sprintf(buffer, "Emulation resumed from 0x%04X", pc);
                                SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
                                CheckMenuItem(g_hMenu, IDM_PAUSE, MF_UNCHECKED);
                            }
                        }
                        break;

                    case VK_F2:
                        if (g_emulatorSettings.isRomLoaded) {
                            g_Chip8.resetCpu();

                            //updateDebugger();
                            //SendDlgItemMessage(hwndDebugger, ID_LB_DISASSEMBLEROUTPUT, LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

                            updateUi(g_hMenu);

                            if (g_emulatorSettings.isDebuggerRunning) {
                                updateDebugger(g_hWndDebugger);
                                drawBitmap();
                            }

                            char buffer[64];
                            sprintf(buffer, "Emulator reset to 0x200");
                            SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
                        }
                        break;

                    case VK_F3:
                        if (g_emulatorSettings.isRomLoaded) {
                            g_Chip8.saveCpuState();

                            char buffer[64];
                            unsigned short pc = g_Chip8.getProgramCounter();
                            sprintf(buffer, "State saved at 0x%04X", pc);
                            SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
                        }
                        break;

                    case VK_F4:
                        if (g_emulatorSettings.isRomLoaded) {
                            g_Chip8.loadCpuState();

                            if (g_emulatorSettings.isDebuggerRunning) {
                                updateDebugger(g_hWndDebugger);
                                drawBitmap();
                            }

                            if (g_emulatorSettings.isGamePaused) {
                                drawBitmap();
                            }

                            char buffer[64];
                            unsigned short pc = g_Chip8.getProgramCounter();
                            sprintf(buffer, "State loaded at 0x%04X", pc);
                            SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
                        }
                        break;

                    case VK_F5:
                        g_emulatorSettings.cpuSpeed = 0;
                        updateUi(g_hMenu);
                        break;

                    case VK_F6:
                        g_emulatorSettings.cpuSpeed = 1;
                        updateUi(g_hMenu);
                        break;

                    case VK_F7:
                        g_emulatorSettings.cpuSpeed = 2;
                        updateUi(g_hMenu);
                        break;

                    case VK_F8:
                        g_emulatorSettings.cpuSpeed = 3;
                        updateUi(g_hMenu);
                        break;

                    case VK_TAB:
                        if (g_emulatorSettings.isRomLoaded && !g_emulatorSettings.isDebuggerRunning) {
                            g_hWndDebugger = CreateDialog(GetModuleHandle(0), MAKEINTRESOURCE(IDD_DEBUGGER), 0, reinterpret_cast<DLGPROC>(DebuggerProc));

                            if (!g_hWndDebugger) {
                                SHOW_ERROR("Error opening debugger!");
                            } else {
                                g_emulatorSettings.isDebuggerRunning = true;
                                CheckMenuItem(g_hMenu, IDM_DEBUGGER, MF_CHECKED);
                            }
                        } else {
                            EndDialog(g_hWndDebugger, 0);
                            g_emulatorSettings.isDebuggerRunning = false;
                            CheckMenuItem(g_hMenu, IDM_DEBUGGER, MF_UNCHECKED);
                        }
                        break;

                    case VK_ESCAPE:
                        PostQuitMessage(0);
                        break;
                }
                break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}
