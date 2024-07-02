#pragma once
#include<Windows.h>
#include <emmintrin.h>
#define SCREEN_WIDTH 380
#define SCREEN_HEIGHT 240
#define BITCOUNT 32
#define GAME_MEMORY_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT * (BITCOUNT / 8))
#define TARGET_FPS 60
#define TARGET_MICROSECONDS 16667

typedef struct {
    BITMAPINFO bmpInfo;
    void* Memory;
} BitmapGame;

typedef struct {
    unsigned char Blue;
    unsigned char Green;
    unsigned char Red;
    unsigned char Alpha;
} Pixel;

typedef struct {
    LARGE_INTEGER Frequency;
    float FpsAveragePerSecond;
    float FpsAverageCookedPerSecond;
    int TotalFramesFutherTarget;
    BOOL DebugInformationAboutFPS;
    int MonitorWidth;
    int MonitorHeight;
    LONG CurrentTimerResolution;
    LONG MinimumTimerResolution;
    LONG MaximumTimerResolution;
} MONITORANDDATA;

BOOL isRunning = TRUE;

// function types from NTDLL.DLL
typedef LONG(NTAPI* NTQUERYTIMERRESOLUTION) (OUT PULONG MinimumResolution,
    OUT PULONG MaximumResolution,
    OUT PULONG CurrentResolution);
typedef LONG(NTAPI* NTSETTIMERRESOLUTION) (IN ULONG DesiredResolution,
    IN BOOLEAN SetResolution,
    OUT PULONG CurrentResolution);
NTQUERYTIMERRESOLUTION  NtQueryTimerResolution;

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ProcessInput(HWND hwnd);
void RenderFrame(HWND hwnd);
void ClearBuffer(_In_ __m128i pixel);
const wchar_t CLASS_NAME[] = L"SampleWindowClass";
HMODULE NtDll;
BitmapGame Bitmap = { 0 };
MONITORINFO MonitorInfo = { 0 };
MONITORANDDATA MonitorData = { 0 };
MSG msg = { 0 };