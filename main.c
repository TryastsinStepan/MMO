#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stdint.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 600
#define BITCOUNT 32
#define GAME_MEMORY_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT * (BITCOUNT / 8))
#define TARGET_FPS 100
#define TARGET_MICROSECONDS 16667

typedef struct {
    BITMAPINFO bmpInfo;
    void* memory;
} BitmapGame;

typedef struct {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char alpha;
} Pixel;

typedef struct {
    LARGE_INTEGER frequency;
    float fpsAverage;
    float fpsAverageCooked;
    int totalFrames;
    BOOL debuginfopress;
} FPS;

int isRunning = 1;
int monitorWidth = 0;
int monitorHeight = 0;

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ProcessInput(HWND hwnd);
void RenderFrame(HWND hwnd);
const wchar_t CLASS_NAME[] = L"SampleWindowClass";
BitmapGame bitmapGame = { 0 };
MONITORINFO monitorInfo = { 0 };
FPS fps = { 0 };
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    HWND hwnd = NULL;
    int64_t elapsedMicroseconds = 0;
    int64_t rawElapsedMicrosecondsAccumulator = 0;
    int64_t cookedElapsedMicrosecondsAccumulator = 0;
    int64_t startTime = 0;
    int64_t endTime = 0;
    WNDCLASSEXW windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = hInstance;
    windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = CreateSolidBrush(RGB(0, 255, 255));
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = CLASS_NAME;
    windowClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    fps.debuginfopress=FALSE;
    if (RegisterClassExW(&windowClass) == 0) {
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

    bitmapGame.bmpInfo.bmiHeader.biSize = sizeof(bitmapGame.bmpInfo.bmiHeader);
    bitmapGame.bmpInfo.bmiHeader.biWidth = SCREEN_WIDTH;
    bitmapGame.bmpInfo.bmiHeader.biHeight = -SCREEN_HEIGHT; // Top-down DIB
    bitmapGame.bmpInfo.bmiHeader.biPlanes = 1;
    bitmapGame.bmpInfo.bmiHeader.biBitCount = BITCOUNT;
    bitmapGame.bmpInfo.bmiHeader.biCompression = BI_RGB;

    bitmapGame.memory = VirtualAlloc(NULL, GAME_MEMORY_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (bitmapGame.memory == NULL) {
        MessageBoxA(hwnd, "Memory allocation failed!!!", "ERROR", MB_OK);
        goto error;
    }

    monitorInfo.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfoA(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &monitorInfo) == 0) {
        MessageBoxA(hwnd, "Can't find monitor!!!", "ERROR", MB_OK);
        goto error;
    }

    monitorWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    monitorHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

    if (SetWindowLongPtrA(hwnd, GWL_STYLE, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW) == 0) {
        MessageBoxA(hwnd, "Set window style failed!!!", "ERROR", MB_OK);
        goto error;
    }
    if (SetWindowPos(hwnd, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, monitorWidth, monitorHeight, SWP_NOZORDER | SWP_FRAMECHANGED) == 0) {
        MessageBoxA(hwnd, "Set window position failed!!!", "ERROR", MB_OK);
        goto error;
    }

    MSG msg = { 0 };
    QueryPerformanceFrequency(&fps.frequency);
    while (isRunning) {
        QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        ProcessInput(hwnd);
        RenderFrame(hwnd);
        QueryPerformanceCounter((LARGE_INTEGER*)&endTime);
        elapsedMicroseconds = endTime - startTime;
        elapsedMicroseconds *= 1000000;
        elapsedMicroseconds /= fps.frequency.QuadPart;
        fps.totalFrames++;
        rawElapsedMicrosecondsAccumulator += elapsedMicroseconds;
        while (elapsedMicroseconds <= TARGET_MICROSECONDS) {
            Sleep(0);
            elapsedMicroseconds = endTime - startTime;
            elapsedMicroseconds *= 1000000;
            elapsedMicroseconds /= fps.frequency.QuadPart;
            QueryPerformanceCounter((LARGE_INTEGER*)&endTime);
        }
        cookedElapsedMicrosecondsAccumulator += elapsedMicroseconds;
        if ((fps.totalFrames % TARGET_FPS) == 0) {
            float averageMicroseconds = rawElapsedMicrosecondsAccumulator / TARGET_FPS;
            int64_t averageMicrosecondsCooked = cookedElapsedMicrosecondsAccumulator / TARGET_FPS;
            fps.fpsAverage = 1.0f / ((averageMicroseconds / TARGET_FPS) * 0.000001f);
            fps.fpsAverageCooked = 1.0f / ((averageMicrosecondsCooked / TARGET_FPS) * 0.000001f);
            char str[256] = { 0 };
            snprintf(str, sizeof(str),
                "AVG millisec/frame: %.02f\tAVG FPS Cooked: %.01f\tAVG FPS Raw: %.01f\n",
                averageMicroseconds, fps.fpsAverageCooked, fps.fpsAverage);
            OutputDebugStringA(str);
            rawElapsedMicrosecondsAccumulator = 0;
            cookedElapsedMicrosecondsAccumulator = 0;
        }
    }

    if (bitmapGame.memory) {
        VirtualFree(bitmapGame.memory, 0, MEM_RELEASE);
    }
    return 0;

error:
    if (bitmapGame.memory) {
        VirtualFree(bitmapGame.memory, 0, MEM_RELEASE);
    }
    return -1;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

void ProcessInput(HWND hwnd) {
    short keyWasPress;
    short keyIsPress = GetAsyncKeyState(VK_F12);
    if (GetAsyncKeyState(VK_ESCAPE)) {
        SendMessageA(hwnd, WM_CLOSE, 0, 0);
    }
    if (keyIsPress ) {
        fps.debuginfopress = !fps.debuginfopress;
    }
}
void RenderFrame(HWND hwnd) {
    Pixel pixel = { 0xFF, 0, 0, 0 };
    Pixel* bitmapMemory = (Pixel*)bitmapGame.memory;
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
        bitmapMemory[i] = pixel;
    }

    HDC graphicsContext = GetDC(hwnd);
    StretchDIBits(graphicsContext, 0, 0, monitorWidth, monitorHeight, 0, 0,
        SCREEN_WIDTH, SCREEN_HEIGHT, bitmapGame.memory, &bitmapGame.bmpInfo, DIB_RGB_COLORS, SRCCOPY);
    if (fps.debuginfopress) {
        SetTextColor(graphicsContext, RGB(255, 255, 255)); // White text
        SetBkMode(graphicsContext, TRANSPARENT); // Transparent background

        char str[64] = { 0 };
        sprintf_s(str, sizeof(str), "AVG FPS Raw: %.01f\n", fps.fpsAverage);
        TextOutA(graphicsContext, 0, 0, str, (int)strlen(str));

        sprintf_s(str, sizeof(str), "FPS Cooked: %.01f\n", fps.fpsAverageCooked);
        TextOutA(graphicsContext, 0, 16, str, (int)strlen(str));
    }
  

    ReleaseDC(hwnd, graphicsContext);
}
