#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "gta_sa.hpp"
#include "model_loader.hpp"

static DWORD WINAPI InitThread(LPVOID) {
    // Esperar a que GTA SA inicialice su tabla de model info.
    // El modelo 400 (Landstalker) es el primer vehiculo; cuando su puntero
    // es no-nulo, el sistema de modelos ya esta listo.
    void** table = gta::modelInfoTable();
    while (!table[400]) Sleep(100);

    // Margen extra para que el resto del streaming termine de inicializar
    Sleep(500);

    ModelLoader::init();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
