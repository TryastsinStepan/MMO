#include"Define.h"
#include <wchar.h>
#include <stdio.h>
#include <stdint.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    HWND hwnd = NULL;
    int64_t ElapsedMicroseconds = 0;
    int64_t RawElapsedMicrosecondsAccumulator = 0;
    int64_t CookedElapsedMicrosecondsAccumulator = 0;
    int64_t StartTimeFrame = 0;
    int64_t EndTimeFrame = 0;
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
    if (RegisterClassExW(&windowClass) == 0) {
        MessageBoxA(NULL, "Register class failed!!!", "ERROR", MB_OK);
        goto error;
    }

    hwnd = CreateWindowExW(0, 
                  CLASS_NAME, 
 L"Learn to Program Windows", 
         WS_OVERLAPPEDWINDOW,
               CW_USEDEFAULT,
               CW_USEDEFAULT,         
                SCREEN_WIDTH,                
               SCREEN_HEIGHT,
                        NULL,          
                        NULL,       
                   hInstance,             
                       NULL);
    if (hwnd == NULL) {
        MessageBoxA(NULL, "Create window failed!!!", "ERROR", MB_OK);
        goto error;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    //Get NtQueryTimerResolution from ntdll
    if ((NtDll = GetModuleHandleA("ntdll.dll")) == NULL) {
        MessageBoxA(NULL, "ntdll file cant find!!!", "ERROR", MB_OK);
        goto error;
    }
    if ((NtQueryTimerResolution = (NTQUERYTIMERRESOLUTION)GetProcAddress(NtDll, "NtQueryTimerResolution")) == NULL)
    {
        MessageBoxA(NULL, "NtQueryTimerResolution cant find!!!", "ERROR", MB_OK);
        goto error;
    }
    NtQueryTimerResolution(&MonitorData.MinimumTimerResolution,
                           &MonitorData.MaximumTimerResolution,
                           &MonitorData.CurrentTimerResolution);


    Bitmap.bmpInfo.bmiHeader.biSize = sizeof(Bitmap.bmpInfo.bmiHeader);
    Bitmap.bmpInfo.bmiHeader.biWidth = SCREEN_WIDTH;
    Bitmap.bmpInfo.bmiHeader.biHeight = -SCREEN_HEIGHT;
    Bitmap.bmpInfo.bmiHeader.biPlanes = 1;
    Bitmap.bmpInfo.bmiHeader.biBitCount = BITCOUNT;
    Bitmap.bmpInfo.bmiHeader.biCompression = BI_RGB;
    Bitmap.Memory = VirtualAlloc(NULL, GAME_MEMORY_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (Bitmap.Memory == NULL) {
        MessageBoxA(hwnd, "Memory allocation failed!!!", "ERROR", MB_OK);
        goto error;
    }

    MonitorInfo.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfoA(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo) == 0) {
        MessageBoxA(hwnd, "Can't find monitor!!!", "ERROR", MB_OK);
        goto error;
    }

    MonitorData.MonitorWidth = MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left;
    MonitorData.MonitorHeight= MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;

    if (SetWindowLongPtrA(hwnd, GWL_STYLE, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW) == 0) {
        MessageBoxA(hwnd, "Set window style failed!!!", "ERROR", MB_OK);
        goto error;
    }
    if (SetWindowPos(hwnd, HWND_TOP, MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top, MonitorData.MonitorWidth, MonitorData.MonitorWidth, SWP_NOZORDER | SWP_FRAMECHANGED) == 0) {
        MessageBoxA(hwnd, "Set window position failed!!!", "ERROR", MB_OK);
        goto error;
    }
    QueryPerformanceFrequency(&MonitorData.Frequency);
    while (isRunning == TRUE) {
        QueryPerformanceCounter((LARGE_INTEGER*)&StartTimeFrame);
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        ProcessInput(hwnd);
        RenderFrame(hwnd);
        QueryPerformanceCounter((LARGE_INTEGER*)&EndTimeFrame);

        ElapsedMicroseconds = EndTimeFrame - StartTimeFrame;
        ElapsedMicroseconds *= 1000000;
        ElapsedMicroseconds /= MonitorData.Frequency.QuadPart;
        MonitorData.TotalFramesFutherTarget++;

        RawElapsedMicrosecondsAccumulator += ElapsedMicroseconds;

        while (ElapsedMicroseconds <= TARGET_MICROSECONDS) {
            ElapsedMicroseconds = EndTimeFrame - StartTimeFrame;
            ElapsedMicroseconds *= 1000000;
            ElapsedMicroseconds /= MonitorData.Frequency.QuadPart;
            QueryPerformanceCounter((LARGE_INTEGER*)&EndTimeFrame);
            if (ElapsedMicroseconds <= (TARGET_MICROSECONDS - MonitorData.CurrentTimerResolution)) {
                Sleep(1);
            }
        }

        CookedElapsedMicrosecondsAccumulator += ElapsedMicroseconds;

        if ((MonitorData.TotalFramesFutherTarget % TARGET_FPS) == 0) {
            MonitorData.FpsAveragePerSecond = 1.0f / ((RawElapsedMicrosecondsAccumulator / TARGET_FPS) * 0.000001f);
            MonitorData.FpsAverageCookedPerSecond = 1.0f / ((CookedElapsedMicrosecondsAccumulator / TARGET_FPS) * 0.000001f);
            char str[256] = { 0 };
            snprintf(str, sizeof(str),"AVG FPS Cooked: %.01f\tAVG FPS Raw: %.01f\n", MonitorData.FpsAverageCookedPerSecond, MonitorData.FpsAveragePerSecond);
            OutputDebugStringA(str);
            RawElapsedMicrosecondsAccumulator = 0;
            CookedElapsedMicrosecondsAccumulator = 0;
        }
    }

    if (Bitmap.Memory) {
        VirtualFree(Bitmap.Memory, 0, MEM_RELEASE);
    }
    return 0;

error:
    if (Bitmap.Memory) {
        VirtualFree(Bitmap.Memory, 0, MEM_RELEASE);
    }
    return -1;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        isRunning = FALSE;
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void ProcessInput(HWND hwnd) {
   static short keyWasPress;
    short keyIsPress = GetAsyncKeyState(VK_F12);
    if (GetAsyncKeyState(VK_ESCAPE)) {
        SendMessageA(hwnd, WM_CLOSE, 0, 0);
    }
    if (keyIsPress&&!keyWasPress ) {
        MonitorData.DebugInformationAboutFPS = !MonitorData.DebugInformationAboutFPS;
    }
    keyIsPress = keyWasPress;
}
void RenderFrame(HWND hwnd) {
    char str[64] = { 0 };
    Pixel pixel = { 0xFF, 0, 0, 0 };
    Pixel* bitmapMemory = (Pixel*)Bitmap.Memory;
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
        bitmapMemory[i] = pixel;
    }

    HDC graphicsContext = GetDC(hwnd);
    StretchDIBits(graphicsContext, 0, 0, MonitorData.MonitorWidth, MonitorData.MonitorHeight, 0, 0,
        SCREEN_WIDTH, SCREEN_HEIGHT, Bitmap.Memory, &Bitmap.bmpInfo, DIB_RGB_COLORS, SRCCOPY);
    if (MonitorData.DebugInformationAboutFPS) {
        sprintf_s(str, sizeof(str), "AVG FPS Raw: %.01f\n", MonitorData.FpsAveragePerSecond);
        TextOutA(graphicsContext, 0, 0, str, (int)strlen(str));

        sprintf_s(str, sizeof(str), "FPS Cooked: %.01f\n", MonitorData.FpsAverageCookedPerSecond);
        TextOutA(graphicsContext, 0, 16, str, (int)strlen(str));

        sprintf_s(str, sizeof(str), "Cur Timer Resolution: %.01f\n", MonitorData.CurrentTimerResolution/10000.0f);
        TextOutA(graphicsContext, 0, 32, str, (int)strlen(str));

        sprintf_s(str, sizeof(str), " Max Resolution: %.01f\n", MonitorData.MaximumTimerResolution / 10000.0f);
        TextOutA(graphicsContext, 0, 48, str, (int)strlen(str));

        sprintf_s(str, sizeof(str), "Min Resolution: %.01f\n", MonitorData.MinimumTimerResolution / 10000.0f);
        TextOutA(graphicsContext, 0, 64, str, (int)strlen(str));
    }
  

    ReleaseDC(hwnd, graphicsContext);
}
