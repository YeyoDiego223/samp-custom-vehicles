#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
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

    // Actualizar ms_aInfoForModel en memoria para redirigir modelo 479 al Porsche.
    // CStreamingInfo: 20 bytes — [+8]=cdPosn(4), [+12]=cdSize(4)
    // El modelo 479 (Stratum) usa "regina.dff" internamente en gta3.img (imgId=2).
    // Nuestro Porsche DFF fue appendado al gta3.img en sector 459016 (319 sectores).
    static constexpr uintptr_t MS_AINFO = 0x8E4CC0;
    uint8_t* entry479 = reinterpret_cast<uint8_t*>(MS_AINFO + 479 * 20);
    __try {
        DWORD oldProt;
        VirtualProtect(entry479, 20, PAGE_EXECUTE_READWRITE, &oldProt);
        *reinterpret_cast<uint32_t*>(entry479 + 8)  = 459016u;
        *reinterpret_cast<uint32_t*>(entry479 + 12) = 319u;
        VirtualProtect(entry479, 20, oldProt, &oldProt);
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    // Descargar modelo cacheado
    typedef void (__cdecl* fn_RemoveModel)(int);
    fn_RemoveModel removeModel = reinterpret_cast<fn_RemoveModel>(0x4089A0);
    removeModel(479);

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
