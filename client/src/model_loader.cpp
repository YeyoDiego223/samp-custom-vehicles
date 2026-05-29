#include "model_loader.hpp"
#include "gta_sa.hpp"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>

// ID del vehiculo base cuyo model info clonamos (Landstalker, siempre valido)
static constexpr int BASE_VEHICLE_ID = 400;

// Rango valido de IDs custom
static constexpr int CUSTOM_ID_MIN = 5000;
static constexpr int CUSTOM_ID_MAX = 5999;

static DWORD g_txdExcAddr = 0;
static int filterTxd(EXCEPTION_POINTERS* ep, DWORD* outCode) {
    *outCode = ep->ExceptionRecord->ExceptionCode;
    g_txdExcAddr = (DWORD)(uintptr_t)ep->ExceptionRecord->ExceptionAddress;
    return EXCEPTION_EXECUTE_HANDLER;
}
static bool tryLoadTxd(int slot, const char* path, DWORD* outCode) {
    __try {
        return gta::LoadTxd(slot, path);
    } __except(filterTxd(GetExceptionInformation(), outCode)) {
        return false;
    }
}

// Parche de null-check en el INICIO de las funciones: si ecx=null al entrar,
// retorna de inmediato (ret 8). Si ecx es valido, ejecuta todo normal.
// Patch de 5 bytes: 85 C9 74 XX 56 = test ecx,ecx; jz retOff; push esi
// (El "push esi" es el primer byte original en ambas funciones, preservado.)
static void patchEntryNullCheck(DWORD funcVA, BYTE retOffset) {
    BYTE* ptr = (BYTE*)funcVA;
    DWORD old; VirtualProtect(ptr, 5, PAGE_EXECUTE_READWRITE, &old);
    ptr[0] = 0x85; ptr[1] = 0xC9;   // test ecx, ecx
    ptr[2] = 0x74; ptr[3] = retOffset; // jz +retOffset (a ret 8)
    ptr[4] = 0x56;                   // push esi (primer byte original)
    VirtualProtect(ptr, 5, old, &old);
}

static bool g_nullCheckPatched = false;
static void applyNullCheckPatches() {
    if (g_nullCheckPatched) return;
    g_nullCheckPatched = true;
    // 0x4C4BC0: ret 8 (C2 08 00) esta en offset 0x3D = 61.
    //   jmpOff = 0x3D - 4 = 0x39 (desde next instr en pos 4)
    patchEntryNullCheck(0x4C4BC0, 0x39);
    // 0x4C48D0: ret 8 (C2 08 00) esta en offset 0x1E = 30.
    //   jmpOff = 0x1E - 4 = 0x1A
    patchEntryNullCheck(0x4C48D0, 0x1A);
    OutputDebugStringA("[cv_client] Entry null-check patches aplicados\n");
}

static DWORD g_excAddr    = 0;
static DWORD g_excRetAddr = 0;  // quien llamo a la funcion que crasheo
static DWORD g_excEcx     = 0;
static int filterRpClump(EXCEPTION_POINTERS* ep, DWORD* outCode) {
    *outCode   = ep->ExceptionRecord->ExceptionCode;
    g_excAddr  = (DWORD)(uintptr_t)ep->ExceptionRecord->ExceptionAddress;
    g_excEcx   = ep->ContextRecord->Ecx;
    // [esp+4] en el momento del crash = return addr al caller de la funcion que crasheo
    // (la funcion comienza con push esi, asi que el ret addr esta en [esp+4])
    DWORD esp_at_crash = ep->ContextRecord->Esp;
    __try { g_excRetAddr = *(DWORD*)(esp_at_crash + 4); } __except(EXCEPTION_EXECUTE_HANDLER) { g_excRetAddr = 0; }
    return EXCEPTION_EXECUTE_HANDLER;
}
static gta::RpClump* tryRpClumpRead(gta::RwStream* stream, DWORD* outCode) {
    __try {
        return gta::RpClumpRead(stream);
    } __except(filterRpClump(GetExceptionInformation(), outCode)) {
        return nullptr;
    }
}

static gta::RpClump* tryRpClumpRead2(const char* path, DWORD* outCode) {
    __try {
        return gta::LoadClumpFromFile(path);
    } __except(filterRpClump(GetExceptionInformation(), outCode)) {
        return nullptr;
    }
}

// CFileLoader::LoadAtomicFile necesita esi = modelId (lo usa 0x5305A0 internamente).
// Usamos un wrapper naked para setear esi antes de la llamada.
static bool g_loadAtomicResult;
static uint32_t g_loadAtomicModelId;
static gta::RwStream* g_loadAtomicStream;

__declspec(naked) static void callLoadAtomicNaked() {
    __asm {
        mov  esi, [g_loadAtomicModelId]     // esi = modelId requerido por 0x5305A0
        push [g_loadAtomicModelId]           // arg2 = modelId
        push [g_loadAtomicStream]            // arg1 = stream
        mov  eax, 0x5371F0                  // direccion directa (no indirecta)
        call eax
        add  esp, 8
        and  eax, 0xFF                       // al = bool result
        mov  [g_loadAtomicResult], al
        ret
    }
}

static bool tryLoadAtomic(gta::RwStream* stream, uint32_t modelId, DWORD* outCode) {
    g_loadAtomicStream  = stream;
    g_loadAtomicModelId = modelId;
    g_loadAtomicResult  = false;
    __try {
        callLoadAtomicNaked();
        return g_loadAtomicResult;
    } __except(filterRpClump(GetExceptionInformation(), outCode)) {
        return false;
    }
}

struct RwMemory { void* start; uint32_t length; };

static gta::RwStream* tryRwStreamOpenMem(const RwMemory* mem, DWORD* outCode) {
    __try {
        // Probar valores 1, 2, 3 para rwSTREAMMEMORY hasta encontrar el correcto
        for (int t = 1; t <= 3; ++t) {
            gta::RwStream* s = gta::RwStreamOpen(
                static_cast<gta::RwStreamType>(t), gta::rwSTREAMREAD, mem);
            if (s) { *outCode = (DWORD)t; return s; }
        }
        *outCode = 0;
        return nullptr;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        *outCode = GetExceptionCode() | 0x80000000u;
        return nullptr;
    }
}

// Usa HANDLE directo para sobrevivir a corrupcion del C runtime
static void logMsg(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    OutputDebugStringA(buf);
    OutputDebugStringA("\n");

    static HANDLE hLog = INVALID_HANDLE_VALUE;
    if (hLog == INVALID_HANDLE_VALUE) {
        char tmpDir[MAX_PATH], logPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tmpDir);
        snprintf(logPath, sizeof(logPath), "%scustom_vehicles_client.log", tmpDir);
        hLog = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_READ,
                           nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hLog != INVALID_HANDLE_VALUE)
            SetFilePointer(hLog, 0, nullptr, FILE_END);
    }
    if (hLog != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(hLog, buf, (DWORD)strlen(buf), &written, nullptr);
        WriteFile(hLog, "\n", 1, &written, nullptr);
        FlushFileBuffers(hLog);
    }
}

// Obtiene el directorio del ejecutable (GTA SA root)
static std::string getGameDir() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string s(path);
    auto pos = s.rfind('\\');
    return (pos != std::string::npos) ? s.substr(0, pos) : ".";
}

// Lee custom_vehicles.ini y llama a registerModel por cada entrada.
// Formato:
//   [custom_vehicles]
//   5000=models/custom/mycar.dff,models/custom/mycar.txd
static void loadConfig() {
    std::string cfgPath = getGameDir() + "\\custom_vehicles.ini";
    std::ifstream f(cfgPath);
    if (!f.is_open()) {
        logMsg("[cv_client] No se encontro %s", cfgPath.c_str());
        return;
    }

    logMsg("[cv_client] Leyendo %s", cfgPath.c_str());
    std::string line;
    bool inSection = false;

    while (std::getline(f, line)) {
        // Ignorar comentarios y lineas vacias
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        if (line == "[custom_vehicles]") { inSection = true; continue; }
        if (line[0] == '[') { inSection = false; continue; }
        if (!inSection) continue;

        // Formato: id=dff,txd
        auto eq = line.find('=');
        auto cm = line.find(',');
        if (eq == std::string::npos || cm == std::string::npos || cm < eq) continue;

        int id = std::stoi(line.substr(0, eq));
        std::string dff = line.substr(eq + 1, cm - eq - 1);
        std::string txd = line.substr(cm + 1);

        // Trim espacios
        auto trim = [](std::string& s) {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
            while (!s.empty() && (s.back()  == ' ' || s.back()  == '\r' || s.back() == '\n')) s.pop_back();
        };
        trim(dff); trim(txd);

        ModelLoader::registerModel(id, dff, txd);
    }
}

bool ModelLoader::registerModel(int modelId, const std::string& dffPath, const std::string& txdPath) {
    // Aceptar IDs de vehiculos SA estandar (400-611) y custom (5000-5999)
    bool validSA    = (modelId >= 400 && modelId <= 611);
    bool validCustom = (modelId >= CUSTOM_ID_MIN && modelId <= CUSTOM_ID_MAX);
    if (!validSA && !validCustom) {
        logMsg("[cv_client] ID %d fuera de rango valido", modelId);
        return false;
    }
    if (modelId >= gta::MODEL_TABLE_SIZE) {
        logMsg("[cv_client] ID %d excede la tabla de GTA SA (%d)", modelId, gta::MODEL_TABLE_SIZE);
        return false;
    }

    void** table = gta::modelInfoTable();

    // Resolver rutas absolutas
    std::string gameDir = getGameDir();
    std::string absDff = gameDir + "\\" + dffPath;
    std::string absTxd = gameDir + "\\" + txdPath;

    if (GetFileAttributesA(absDff.c_str()) == INVALID_FILE_ATTRIBUTES) {
        logMsg("[cv_client] DFF no encontrado: %s", absDff.c_str());
        return false;
    }
    if (GetFileAttributesA(absTxd.c_str()) == INVALID_FILE_ATTRIBUTES) {
        logMsg("[cv_client] TXD no encontrado: %s", absTxd.c_str());
        return false;
    }

    // Para IDs de vehiculos SA existentes (400-611): usar la model info
    // ORIGINAL (no clonar) y solo reemplazar el RpClump — preserva fisicas,
    // damage frames, LODs y toda la estructura del vehiculo.
    // Para IDs custom (5000+): clonar desde el vehiculo base.
    bool isExistingSA = (modelId >= 400 && modelId <= 611);
    void* existingInfo = table[modelId];

    void* newInfo = nullptr;
    if (isExistingSA && existingInfo) {
        newInfo = existingInfo;  // reusar la model info original
        logMsg("[cv_client] Reemplazando RpClump en model info existente de ID %d", modelId);
    } else {
        void* baseInfo = table[BASE_VEHICLE_ID];
        if (!baseInfo) {
            logMsg("[cv_client] Model info base (%d) no disponible", BASE_VEHICLE_ID);
            return false;
        }
        newInfo = std::malloc(gta::SIZEOF_VEHICLE_MODEL_INFO);
        if (!newInfo) { logMsg("[cv_client] malloc fallo"); return false; }
        std::memcpy(newInfo, baseInfo, gta::SIZEOF_VEHICLE_MODEL_INFO);
    }


    // 2. Cargar TXD
    char txdName[32];
    snprintf(txdName, sizeof(txdName), "cv_%d", modelId);
    int txdSlot = gta::AddTxdSlot(txdName);
    if (txdSlot < 0) {
        logMsg("[cv_client] AddTxdSlot fallo para modelo %d", modelId);
        std::free(newInfo);
        return false;
    }
    logMsg("[cv_client] TXD slot %d asignado", txdSlot);

    DWORD txdExc = 0;
    bool txdOk = tryLoadTxd(txdSlot, absTxd.c_str(), &txdExc);
    if (!txdOk) {
        if (txdExc)
            logMsg("[cv_client] LoadTxd EXCEPCION 0x%08X en addr=0x%08X", txdExc, g_txdExcAddr);
        else
            logMsg("[cv_client] LoadTxd retorno false");
        gta::RemoveTxdSlot(txdSlot);
        std::free(newInfo);
        return false;
    }
    logMsg("[cv_client] TXD cargado OK, slot=%d", txdSlot);
    gta::SetCurrentTxd(txdSlot);
    gta::modelField<int16_t>(newInfo, gta::OFF_TXD_INDEX) = static_cast<int16_t>(txdSlot);

    // 4. Cargar DFF con CFileLoader::LoadAtomicFile(stream, modelId):
    //    busca ms_modelInfoPtrs[modelId], ejecuta virtual setup del pool de atomicos,
    //    luego llama al clump reader — pipeline identico al streaming de GTA SA.
    table[modelId] = newInfo;  // registrar ANTES para que la busqueda ms_modelInfoPtrs[modelId] funcione
    gta::SetCurrentTxd(txdSlot);

    // Parchear 0x4C4BC0/0x4C48D0 para manejar ecx=null (SA-MP los parchea y
    // retornan null para modelos no registrados con SA-MP, causando crash).
    applyNullCheckPatches();
    // GTA SA usa [0x9689E0] como "current model info" durante la carga del DFF
    *(void**)0x9689E0 = newInfo;
    logMsg("[cv_client] Abriendo stream DFF: %s", absDff.c_str());
    gta::RwStream* stream = gta::RwStreamOpen(gta::rwSTREAMFILENAME, gta::rwSTREAMREAD, absDff.c_str());
    if (!stream) {
        logMsg("[cv_client] RwStreamOpen fallo");
        table[modelId] = nullptr;
        gta::RemoveTxdSlot(txdSlot);
        std::free(newInfo);
        return false;
    }

    // Para IDs SA existentes usar LoadAtomicFile (setup completo de vehicle type,
    // LODs, damage frames). Para IDs custom usar LoadClumpFromFile (mas simple).
    bool loadOk = false;
    gta::RpClump* clump = nullptr;
    DWORD clumpExc = 0;

    // IDs SA existentes (400-611): usar LoadAtomicFile con vtable restaurado
    // para setup completo del vehiculo. IDs custom: LoadClumpFromFile.
    if (isExistingSA) {
        logMsg("[cv_client] LoadAtomicFile para ID SA %d (vtable restaurado)...", modelId);
        loadOk = tryLoadAtomic(stream, (uint32_t)modelId, &clumpExc);
        gta::RwStreamClose(stream, nullptr);
        clump = reinterpret_cast<gta::RpClump*>(
            gta::modelField<gta::RwObject*>(newInfo, gta::OFF_RW_OBJECT));
        if (clumpExc)
            logMsg("[cv_client] LoadAtomicFile EXCEPCION 0x%08X addr=0x%08X", clumpExc, g_excAddr);
        else
            logMsg("[cv_client] LoadAtomicFile ok=%d clump=%p", (int)loadOk, (void*)clump);
    } else {
        logMsg("[cv_client] Cargando DFF con LoadClumpFromFile...");
        gta::RwStreamClose(stream, nullptr);
        clump = tryRpClumpRead2(absDff.c_str(), &clumpExc);
        if (clumpExc)
            logMsg("[cv_client] LoadClumpFromFile EXCEPCION 0x%08X addr=0x%08X", clumpExc, g_excAddr);
        else
            logMsg("[cv_client] LoadClumpFromFile retorno: %p", (void*)clump);
        loadOk = (clump != nullptr);
    }

    if (!loadOk) {
        logMsg("[cv_client] LoadClumpFromFile fallo");
        if (!isExistingSA) { table[modelId] = nullptr; std::free(newInfo); }
        gta::RemoveTxdSlot(txdSlot);
        return false;
    }
    gta::modelField<gta::RwObject*>(newInfo, gta::OFF_RW_OBJECT) = reinterpret_cast<gta::RwObject*>(clump);

    logMsg("[cv_client] Modelo %d registrado OK (%s)", modelId, dffPath.c_str());
    return true;
}

void ModelLoader::init() {
    logMsg("[cv_client] Inicializando ModelLoader");

    loadConfig();
    logMsg("[cv_client] ModelLoader listo");
}

void ModelLoader::applyPatches() {
    applyNullCheckPatches();
}

void ModelLoader::restoreVtable() {
    // Obtener el vtable de CVehicleModelInfo desde Landstalker (400)
    void** table = gta::modelInfoTable();
    void* info400 = table[400];
    if (!info400) return;

    void** vtable = *(void***)info400;  // vtable pointer del model info
    uintptr_t vtableVA = (uintptr_t)vtable;

    // Calcular file offset parseando las secciones PE para encontrar la correcta
    BYTE* base = (BYTE*)0x400000;
    auto* dos  = (IMAGE_DOS_HEADER*)base;
    auto* nt   = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    auto* sect = IMAGE_FIRST_SECTION(nt);
    uintptr_t fileOff = 0;
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        uintptr_t s = 0x400000 + sect[i].VirtualAddress;
        uintptr_t e = s + sect[i].Misc.VirtualSize;
        if (vtableVA >= s && vtableVA < e) {
            fileOff = sect[i].PointerToRawData + (vtableVA - s);
            break;
        }
    }
    if (!fileOff) { logMsg("[cv_client] No se encontro seccion para vtable 0x%p", (void*)vtableVA); return; }

    // Leer vtable[1] (offset 4) y vtable[2] (offset 8) del binario original
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    HANDLE hFile = CreateFileW(exePath, GENERIC_READ, FILE_SHARE_READ,
                               nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return;

    DWORD origVtable1 = 0, origVtable2 = 0, read = 0;
    SetFilePointer(hFile, (LONG)(fileOff + 4), nullptr, FILE_BEGIN);
    ReadFile(hFile, &origVtable1, 4, &read, nullptr);
    SetFilePointer(hFile, (LONG)(fileOff + 8), nullptr, FILE_BEGIN);
    ReadFile(hFile, &origVtable2, 4, &read, nullptr);
    CloseHandle(hFile);

    logMsg("[cv_client] Restaurando vtable: vtable[1]=0x%08X vtable[2]=0x%08X",
           origVtable1, origVtable2);

    // Restaurar los punteros originales en el vtable en memoria
    DWORD old;
    VirtualProtect(vtable, 16, PAGE_EXECUTE_READWRITE, &old);
    ((DWORD*)vtable)[1] = origVtable1;  // vtable offset 4
    ((DWORD*)vtable)[2] = origVtable2;  // vtable offset 8
    VirtualProtect(vtable, 16, old, &old);

    logMsg("[cv_client] Vtable restaurado OK");
}
