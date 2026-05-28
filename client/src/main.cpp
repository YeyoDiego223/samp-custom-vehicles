#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "gta_sa.hpp"
#include "model_loader.hpp"

#define WM_CV_LOAD (WM_USER + 42)

static HWND    g_hwnd       = NULL;
static WNDPROC g_origWndProc = NULL;

// Ejecutado en el main thread gracias a PostMessage
static LRESULT CALLBACK CvWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_CV_LOAD) {
        // Restaurar WndProc antes de hacer el loading
        SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)g_origWndProc);
        ModelLoader::init();
        return 0;
    }
    return CallWindowProcA(g_origWndProc, hwnd, msg, wp, lp);
}

static HWND findGameWindow() {
    // Titulos conocidos de GTA SA con SA-MP
    static const char* titles[] = {
        "GTA:SA:MP", "Grand theft auto San Andreas",
        "GTA SA:MP", "GTASA", nullptr
    };
    for (int i = 0; titles[i]; ++i) {
        HWND h = FindWindowA(NULL, titles[i]);
        if (h) return h;
    }
    return NULL;
}

static DWORD WINAPI InitThread(LPVOID) {
    // Esperar a que la tabla de model info este lista
    // Re-aplicar patches inmediatamente (SA-MP puede sobreescribirlos
    // durante su propia inicializacion, antes de que el jugador conecte)
    ModelLoader::applyPatches();

    void** table = gta::modelInfoTable();
    while (!table[400]) Sleep(100);
    Sleep(3000);
    ModelLoader::applyPatches();   // Re-aplicar por si SA-MP los sobreescribio
    ModelLoader::restoreVtable();  // Restaurar vtable[1/2] a valores originales

    // Esperar a que la ventana del juego exista
    HWND hwnd = NULL;
    for (int i = 0; i < 60 && !hwnd; ++i) {
        hwnd = findGameWindow();
        if (!hwnd) Sleep(500);
    }

    if (!hwnd) {
        // Sin ventana: cargar directo (fallback, puede crashear)
        ModelLoader::init();
        return 0;
    }

    g_hwnd = hwnd;
    g_origWndProc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)CvWndProc);
    PostMessageA(hwnd, WM_CV_LOAD, 0, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        // Aplicar patches de null-check INMEDIATAMENTE antes de que SA-MP
        // intente validar modelos custom (lo cual llama funciones de GTA SA
        // que crashean con ecx=null sin estos patches).
        ModelLoader::applyPatches();
        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
