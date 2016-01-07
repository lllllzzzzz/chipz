#define WINVER 0x1000

#include <windows.h>
#include <stdio.h>
#include <commctrl.h>

#include "resource.h"
//#include "ini.h"
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

#define FRAMES_PER_SECOND               60
#define TIMER_INTERVAL                  17
#define LOW_CPU_INTERVAL                5

#define SHOW_ERROR(msg)                 MessageBox(NULL, msg, "chipz", MB_ICONERROR | MB_OK);
#define SHOW_SUCCESS(msg)               MessageBox(NULL, msg, "chipz", MB_ICONASTERISK | MB_OK);

#define Hz_100                          100
#define Hz_200                          200
#define Hz_300                          300
#define Hz_400                          400
#define Hz_500                          500
#define Hz_600                          600
#define Hz_700                          700
#define Hz_800                          800
#define Hz_900                          900
#define Hz_1000                         1000
#define Hz_1500                         1500
#define Hz_2000                         2000

#define DEFAULT_BG_COLOUR               0x00000000
#define DEFAULT_FG_COLOUR               0xFFFFFFFF

int WIN_WIDTH  = 640;
int WIN_HEIGHT = 320;

// Callback prototypes
LRESULT CALLBACK MainProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DebuggerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AboutProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
// Function prototypes
static void drawBitmap();
static void debugStep();
static void playBeep();
static void updateInput();
static void readIni();
static void writeIni();
static void updateDebugger(HWND hwndDebugger);

// Globals
Chip8 g_Chip8;

HDC         hDc             = NULL;
HINSTANCE   g_hInst         = NULL;
HBITMAP     g_hBmp          = NULL;
HBITMAP     g_hBmpOld       = NULL;
HDC         g_hMemDC        = NULL;
UINT32*     g_pPixels       = NULL;
HWND        g_hWnd          = NULL;
HWND        g_hWndDebugger  = NULL;
HWND        g_hStatus       = NULL;
HMENU       g_hMenu         = NULL;

MMRESULT timerEvent;

const char szClassName[]                  = "chipzWindowClass";
const char g_szWindowTitle[MAX_PATH]      = "chipz";

// Global emulator flags
typedef struct emulatorSettings
{
    bool isGamePaused;
    bool isPauseInactiveEnabled;
    bool isSoundEnabled;
    bool isDebuggerRunning;
    bool isRomLoaded;
    unsigned int cpuSpeed;
    unsigned int bgColour;
    unsigned int fgColour;
} emulatorSettings;
emulatorSettings g_emulatorSettings;

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow)
{
    MSG messages;
    WNDCLASSEX wincl;

    wincl.hInstance     = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc   = MainProc;
    wincl.style         = CS_DBLCLKS;
    wincl.cbSize        = sizeof (WNDCLASSEX);
    wincl.hIcon         = LoadIcon (0, IDI_APPLICATION);
    wincl.hIconSm       = LoadIcon (0, IDI_APPLICATION);
    wincl.hCursor       = LoadCursor (0, IDC_ARROW);
    wincl.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
    wincl.cbClsExtra    = 0;
    wincl.cbWndExtra    = 0;

    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    if (!RegisterClassEx(&wincl)) {
        SHOW_ERROR("Error registering window class!\r\nchipz will now terminate.");
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
        SHOW_ERROR("Error creating window!\r\nchipz will now terminate.");
        return -1;
    }

    g_hInst = hThisInstance;

    ShowWindow(g_hWnd, nCmdShow);

    BITMAPINFO bmi;
    bmi.bmiHeader.biSize          = sizeof (BITMAPINFO);
    bmi.bmiHeader.biWidth         = WIN_WIDTH;
    bmi.bmiHeader.biHeight        = -WIN_HEIGHT;
    bmi.bmiHeader.biPlanes        = 1;
    bmi.bmiHeader.biBitCount      = 32;
    bmi.bmiHeader.biCompression   = BI_RGB;
    bmi.bmiHeader.biSizeImage     = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed       = 0;
    bmi.bmiHeader.biClrImportant  = 0;
    bmi.bmiColors[0].rgbBlue      = 0;
    bmi.bmiColors[0].rgbGreen     = 0;
    bmi.bmiColors[0].rgbRed       = 0;
    bmi.bmiColors[0].rgbReserved  = 0;

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

static void updateInput()
{
    char keyState[16];
    int keyList[] = {
        0x31,  // '1'
        0x32,  // '2'
        0x33,  // '3'
        0x34,  // '4'
        0x51,  // 'Q'
        0x57,  // 'W'
        0x45,  // 'E'
        0x52,  // 'R'
        0x41,  // 'A'
        0x53,  // 'S'
        0x44,  // 'D'
        0x46,  // 'F'
        0x5A,  // 'Z'
        0x58,  // 'X'
        0x43,  // 'C'
        0x56,  // 'V'
        -1,
    };

    for (int i = 0; keyList[i] != -1; i++) {
        if ((GetAsyncKeyState(keyList[i]) & 0x8000) != 0) {
            g_Chip8.setFlag(CPU_FLAG_KEYDOWN);
        } else if (GetAsyncKeyState(keyList[i]) == -32767) {
            g_Chip8.resetFlag(CPU_FLAG_KEYDOWN);
        }

        keyState[i] = (GetAsyncKeyState(keyList[i]) & 0x8000) != 0;
    }

    g_Chip8.setKeys(keyState);
}

static void drawBitmap()
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

    // Scale graphics from 128x64/64x32 to 640x320
    for (int y = 0; y < WIN_HEIGHT; y++) {
    for (int x = 0; x < WIN_WIDTH; x++) {
        x1 = (x * gfx_w) / WIN_WIDTH;
        y1 = (y * gfx_h) / WIN_HEIGHT;

        screen[(y * WIN_WIDTH) + x] = gfx[(y1 * gfx_pitch) + x1];
    }
    }

    // Convert 640x320x1 graphics to 640x320x32 bitmap
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

static void playBeep()
{
    PlaySound(MAKEINTRESOURCE(IDR_WAVE1), g_hInst, SND_RESOURCE | SND_ASYNC);
}

void CALLBACK timerCallback(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
    if (g_emulatorSettings.isGamePaused || g_emulatorSettings.isDebuggerRunning
        || (g_emulatorSettings.isPauseInactiveEnabled && GetForegroundWindow() != g_hWnd)) {
        return;
    }

    GUITHREADINFO gui;
    gui.cbSize = sizeof (GUITHREADINFO);
    GetGUIThreadInfo(0, &gui);

    if (gui.flags & GUI_INMOVESIZE || gui.flags & GUI_INMENUMODE) {
        return;
    }

    g_Chip8.emulateCycles(g_emulatorSettings.cpuSpeed / FRAMES_PER_SECOND);

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
}

// Update UI
static void updateUi(HMENU hMenu)
{
    if (!hMenu) {
      return;
    }

    EnableMenuItem(g_hMenu, IDM_CLOSE,                  (g_emulatorSettings.isRomLoaded)                ? MF_ENABLED : MF_DISABLED);
    EnableMenuItem(g_hMenu, IDM_RESET,                  (g_emulatorSettings.isRomLoaded)                ? MF_ENABLED : MF_DISABLED);
    EnableMenuItem(g_hMenu, IDM_SAVE_STATE,             (g_emulatorSettings.isRomLoaded)                ? MF_ENABLED : MF_DISABLED);
    EnableMenuItem(g_hMenu, IDM_LOAD_STATE,             (g_emulatorSettings.isRomLoaded)                ? MF_ENABLED : MF_DISABLED);
    EnableMenuItem(g_hMenu, IDM_DEBUGGER,               (g_emulatorSettings.isRomLoaded)                ? MF_ENABLED : MF_DISABLED);

    CheckMenuItem(hMenu, IDM_PAUSE,                     (g_emulatorSettings.isGamePaused)               ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_DEBUGGER,                  (g_emulatorSettings.isDebuggerRunning)          ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_PAUSE_WHEN_INACTIVE,       (g_emulatorSettings.isPauseInactiveEnabled)     ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_SOUND_ON,                  (g_emulatorSettings.isSoundEnabled)             ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_DETECT_GAME_OVER,          (g_Chip8.getFlag(CPU_FLAG_DETECTGAMEOVER))      ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_DETECT_COLLISION,          (g_Chip8.getFlag(CPU_FLAG_DETECTCOLLISION))     ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_ENABLE_HORIZONTAL_WRAP,    (g_Chip8.getFlag(CPU_FLAG_HWRAP))               ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_ENABLE_VERTICAL_WRAP,      (g_Chip8.getFlag(CPU_FLAG_VWRAP))               ? MF_CHECKED : MF_UNCHECKED);

    CheckMenuItem(hMenu, IDM_0_1_KHZ1,                  (g_emulatorSettings.cpuSpeed == Hz_100)         ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_0_2_KHZ1,                  (g_emulatorSettings.cpuSpeed == Hz_200)         ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_0_3_KHZ1,                  (g_emulatorSettings.cpuSpeed == Hz_300)         ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_0_4_KHZ1,                  (g_emulatorSettings.cpuSpeed == Hz_400)         ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_0_5_KHZ1,                  (g_emulatorSettings.cpuSpeed == Hz_500)         ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_0_6_KHZ1,                  (g_emulatorSettings.cpuSpeed == Hz_600)         ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_0_7_KHZ1,                  (g_emulatorSettings.cpuSpeed == Hz_700)         ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_0_8_KHZ1,                  (g_emulatorSettings.cpuSpeed == Hz_800)         ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_0_9_KHZ1,                  (g_emulatorSettings.cpuSpeed == Hz_900)         ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_1_KHZ1,                    (g_emulatorSettings.cpuSpeed == Hz_1000)        ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_2_KHZ1,                    (g_emulatorSettings.cpuSpeed == Hz_1500)        ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_2_0_KHZ1,                  (g_emulatorSettings.cpuSpeed == Hz_2000)        ? MF_CHECKED : MF_UNCHECKED);
}

// Select filename of ROM to open
char* selectRomFilename()
{
    OPENFILENAME rom;
    char* romPath = new char[MAX_PATH]();

    ZeroMemory(&rom, sizeof (OPENFILENAME));
    rom.lStructSize = sizeof (OPENFILENAME);
    rom.hwndOwner   = g_hWnd;
    rom.lpstrFilter = "All files (*.*)\0*.*\0";
    rom.lpstrFile   = romPath;
    rom.lpstrTitle  = "Open...";
    rom.nMaxFile    = MAX_PATH;
    rom.Flags       = OFN_EXPLORER | OFN_FILEMUSTEXIST;

    GetOpenFileName(&rom);

    if (romPath[0] == '\0') {
        delete[] romPath;
        return 0;
    }

    return romPath;
}

static void setupDebugger(HWND hwndDebugger, unsigned romSize)
{
    if (!hwndDebugger || romSize == 0) {
        return;
    }

    char addressTitle[]   = "Address";
    char opcodeTitle[]    = "Opcode";
    HWND hwndDisassembler = GetDlgItem(hwndDebugger, IDC_DISASSEMBLER);

    LVCOLUMN LvCol;
    ZeroMemory(&LvCol, sizeof (LVCOLUMN));
    LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    // Set up disassembler
    SendMessage(hwndDisassembler, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    LvCol.pszText = addressTitle;
    LvCol.cx = 50;
    SendMessage(hwndDisassembler, LVM_INSERTCOLUMN, 0, reinterpret_cast<LPARAM>(&LvCol));
    LvCol.pszText = opcodeTitle;
    LvCol.cx = 50;
    SendMessage(hwndDisassembler, LVM_INSERTCOLUMN, 1, reinterpret_cast<LPARAM>(&LvCol));

    LVITEM LvItem;
    memset(&LvItem, 0, sizeof (LVITEM));

    LvItem.mask       = LVIF_TEXT;
    LvItem.cchTextMax = 256;
    LvItem.iItem      = 0;

    char buffer[4];

    for (int i = (romSize + CHIP8_ROM_ADDRESS); i >= CHIP8_ROM_ADDRESS; i -= 2) {
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

static void updateDebugger(HWND hwndDebugger)
{
    if (!hwndDebugger) {
        return;
    }

    HWND hwndDisassembler = GetDlgItem(hwndDebugger, IDC_DISASSEMBLER);
    registers cpuRegs = g_Chip8.getRegisters();
    char buffer1[5], buffer2[5];

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

    LockWindowUpdate(hwndDebugger);

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

static void debugStep(HWND hwndDebugger)
{
    if (!hwndDebugger) {
        return;
    }

    g_Chip8.emulateCycles(1);

    g_Chip8.tickDelayTimer();
    g_Chip8.tickSoundTimer();

    if (g_Chip8.getFlag(CPU_FLAG_DRAW)) {
        drawBitmap();
        g_Chip8.resetFlag(CPU_FLAG_DRAW);
    }
}

static bool openRom(char *romPath)
{
    if (!romPath || romPath[0] == '\0' || strlen(romPath) > MAX_PATH) {
        SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>("Error: cannot select filename"));
        return false;
    }

    FILE* pFile = fopen(romPath, "rb");

    fseek(pFile, 0, SEEK_END);
    size_t romSize = ftell(pFile);
    rewind(pFile);

    if (romSize > CHIP8_MAX_ROM_SIZE) {
        SendMessage(g_hStatus, SB_SETTEXT, 0, reinterpret_cast<LPARAM>("Error: ROM size greater than 3584 bytes"));
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
            // Initialise emulator flags to default values
            g_emulatorSettings.isGamePaused           = false;
            g_emulatorSettings.isDebuggerRunning      = false;
            g_emulatorSettings.isRomLoaded            = false;
            g_emulatorSettings.isPauseInactiveEnabled = true;
            g_emulatorSettings.isSoundEnabled         = false;
            g_emulatorSettings.cpuSpeed               = Hz_700;
            g_emulatorSettings.bgColour               = DEFAULT_BG_COLOUR;
            g_emulatorSettings.fgColour               = DEFAULT_FG_COLOUR;

            g_hMenu = GetMenu(hwnd);
            updateUi(g_hMenu);
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

            if (openRom(romPath)) {
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

                    if (openRom(romPath)) {
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

                    free(romPath);
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
                        } else {
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

               case IDM_0_1_KHZ1:
                   g_emulatorSettings.cpuSpeed = Hz_100;
                   updateUi(g_hMenu);
                   break;
               case IDM_0_2_KHZ1:
                   g_emulatorSettings.cpuSpeed = Hz_200;
                   updateUi(g_hMenu);
                   break;
               case IDM_0_3_KHZ1:
                   g_emulatorSettings.cpuSpeed = Hz_300;
                   updateUi(g_hMenu);
                   break;
               case IDM_0_4_KHZ1:
                   g_emulatorSettings.cpuSpeed = Hz_400;
                   updateUi(g_hMenu);
                   break;
               case IDM_0_5_KHZ1:
                   g_emulatorSettings.cpuSpeed = Hz_500;
                   updateUi(g_hMenu);
                   break;
               case IDM_0_6_KHZ1:
                   g_emulatorSettings.cpuSpeed = Hz_600;
                   updateUi(g_hMenu);
                   break;
               case IDM_0_7_KHZ1:
                   g_emulatorSettings.cpuSpeed = Hz_700;
                   updateUi(g_hMenu);
                   break;
               case IDM_0_8_KHZ1:
                   g_emulatorSettings.cpuSpeed = Hz_800;
                   updateUi(g_hMenu);
                   break;
               case IDM_0_9_KHZ1:
                   g_emulatorSettings.cpuSpeed = Hz_900;
                   updateUi(g_hMenu);
                   break;
               case IDM_1_KHZ1:
                   g_emulatorSettings.cpuSpeed = Hz_1000;
                   updateUi(g_hMenu);
                   break;
               case IDM_2_KHZ1:
                   g_emulatorSettings.cpuSpeed = Hz_1500;
                   updateUi(g_hMenu);
                   break;
               case IDM_2_0_KHZ1:
                   g_emulatorSettings.cpuSpeed = Hz_2000;
                   updateUi(g_hMenu);
                   break;

                case IDM_PAUSE_WHEN_INACTIVE:
                    g_emulatorSettings.isPauseInactiveEnabled = !g_emulatorSettings.isPauseInactiveEnabled;
//                    CheckMenuItem(g_hMenu, IDM_PAUSE_WHEN_INACTIVE, (g_emulatorSettings.isPauseInactiveEnabled) ? MF_CHECKED : MF_UNCHECKED);
                    updateUi(g_hMenu);
                    break;

                case IDM_X1:
                    WIN_WIDTH = 320;
                    WIN_HEIGHT = 160;
                    SetWindowPos(hwnd, HWND_TOP, 0, 0, 320, 160, SWP_NOMOVE);
                    break;

                case IDM_COLOR_DEFAULT:
                    g_emulatorSettings.bgColour = DEFAULT_BG_COLOUR;
                    g_emulatorSettings.fgColour = DEFAULT_FG_COLOUR;
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
                            MessageBox(NULL, "Error opening debugger!", "chipz", MB_OK | MB_ICONERROR);
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
