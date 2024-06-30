#include <windows.h>
#include <stdint.h>
#include <wchar.h>
#include <stdio.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 600
#define BITCOUNT 32
#define GAME_AREY_MEMORY (SCREEN_WIDTH * SCREEN_HEIGHT * (BITCOUNT / 8))
#define TARGET_FPS 60


typedef struct BITMAPGAME {
    BITMAPINFO bmp;
    void* mem;
} BITMAPGAME;

typedef struct Pixel {
    unsigned char Blue;
    unsigned char Green;
    unsigned char Red;
    unsigned char Alpha;
} Pixel;

typedef struct FPS {
    LARGE_INTEGER start_count;
    LARGE_INTEGER end_count;
    LARGE_INTEGER frequency;
    double elapsed_time;
    double frame_time;
    int frame_count;
    double fps;
} FPS;

int isRunning = 1;
int WidthMonitor = 0;
int HeightMonitor = 0;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Input(HWND hwnd);
void RenderGame(HWND hwnd);
void CalculateFrameTime(FPS fps);
const wchar_t CLASS_NAME[] = L"Sample Window Class";
BITMAPGAME bitmapgame = { 0 };
MONITORINFO monitor = { 0 };

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    HWND hwnd = NULL;
    WNDCLASSEXW wnd = { 0 };
    wnd.cbSize = sizeof(WNDCLASSEX);
    wnd.style = CS_HREDRAW | CS_VREDRAW;
    wnd.lpfnWndProc = WindowProc;
    wnd.cbClsExtra = 0;
    wnd.cbWndExtra = 0;
    wnd.hInstance = hInstance;
    wnd.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
    wnd.hbrBackground = CreateSolidBrush(RGB(0, 255, 255));
    wnd.lpszMenuName = NULL;
    wnd.lpszClassName = CLASS_NAME;
    wnd.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if ((RegisterClassExW(&wnd) == 0)) {
        MessageBoxA(NULL, "Register class failed!!!", "ERROR", MB_OK);
        goto error;
    }

    hwnd = CreateWindowExW(0, CLASS_NAME, L"Learn to Program Windows", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT,
        NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) {
        MessageBoxA(NULL, "Create window failed!!!", "ERROR", MB_OK);
        goto error;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    bitmapgame.bmp.bmiHeader.biSize = sizeof(bitmapgame.bmp.bmiHeader);
    bitmapgame.bmp.bmiHeader.biWidth = SCREEN_WIDTH;
    bitmapgame.bmp.bmiHeader.biHeight = -SCREEN_HEIGHT; // Top-down DIB
    bitmapgame.bmp.bmiHeader.biPlanes = 1;
    bitmapgame.bmp.bmiHeader.biBitCount = BITCOUNT;
    bitmapgame.bmp.bmiHeader.biCompression = BI_RGB;

    bitmapgame.mem = VirtualAlloc(NULL, GAME_AREY_MEMORY, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (bitmapgame.mem == NULL) {
        MessageBoxA(hwnd, "Memory allocation failed!!!", "ERROR", MB_OK);
        goto error;
    }

    monitor.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfoA(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &monitor) == 0) {
        MessageBoxA(hwnd, "Can't find monitor!!!", "ERROR", MB_OK);
        goto error;
    }

    WidthMonitor = monitor.rcMonitor.right - monitor.rcMonitor.left;
    HeightMonitor = monitor.rcMonitor.bottom - monitor.rcMonitor.top;

    if (SetWindowLongPtrA(hwnd, GWL_STYLE, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW) == 0) {
        MessageBoxA(hwnd, "Set window style failed!!!", "ERROR", MB_OK);
        goto error;
    }
    if (SetWindowPos(hwnd, HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top, WidthMonitor, HeightMonitor, SWP_NOZORDER | SWP_FRAMECHANGED) == 0) {
        MessageBoxA(hwnd, "Set window position failed!!!", "ERROR", MB_OK);
        goto error;
    }

    MSG msg = { 0 };
    FPS fps = { 0 };

    QueryPerformanceFrequency(&fps.frequency);
    QueryPerformanceCounter(&fps.start_count);
    fps.frame_count = 0;


    QueryPerformanceFrequency(&fps.frequency);
    QueryPerformanceCounter(&fps.start_count);
    fps.frame_count = 0;

    const double targetFrameTime = 1.0 / TARGET_FPS;

    while (isRunning) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        QueryPerformanceCounter(&fps.end_count);
        fps.elapsed_time = (double)(fps.end_count.QuadPart - fps.start_count.QuadPart) / fps.frequency.QuadPart;

        if (fps.elapsed_time >= targetFrameTime) {
            fps.frame_time = fps.elapsed_time * 1000.0; // Convert to milliseconds
            fps.fps = 1.0 / fps.elapsed_time;

            fps.start_count = fps.end_count;

            wchar_t buf[256];
            swprintf_s(buf, _countof(buf), L"FPS: %.2f | Frame Time: %.2f ms", fps.fps, fps.frame_time);
            SetWindowText(hwnd, buf);

            Input(hwnd);
            RenderGame(hwnd);
        }
        else {
            double sleepTime = (targetFrameTime - fps.elapsed_time) * 1000.0; // Convert to milliseconds
            if (sleepTime > 0) {
                Sleep((DWORD)sleepTime);
            }
        }
    }

    if (bitmapgame.mem) {
        VirtualFree(bitmapgame.mem, 0, MEM_RELEASE);
    }
    return 0;

error:
    if (bitmapgame.mem) {
        VirtualFree(bitmapgame.mem, 0, MEM_RELEASE);
    }
    return -1;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        isRunning = 0;
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void Input(HWND hwnd) {
    if (GetAsyncKeyState(VK_ESCAPE)) {
        SendMessageA(hwnd, WM_CLOSE, 0, 0);
    }
}

void RenderGame(HWND hwnd) {
    Pixel pixel = { 0xFF, 0, 0xFF, 0 };
    Pixel* bitmapMemory = (Pixel*)bitmapgame.mem;
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
        bitmapMemory[i] = pixel;
    }

    HDC graphic = GetDC(hwnd);
    StretchDIBits(graphic, 0, 0, WidthMonitor, HeightMonitor, 0, 0,
        SCREEN_WIDTH, SCREEN_HEIGHT, bitmapgame.mem, &bitmapgame.bmp, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(hwnd, graphic);
}
