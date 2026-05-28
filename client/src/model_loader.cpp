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

static void logMsg(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    // Escribe al log de SA-MP si existe, o al debugger
    OutputDebugStringA(buf);
    OutputDebugStringA("\n");

    // Tambien escribe a archivo para inspeccion facil
    static FILE* f = nullptr;
    if (!f) f = fopen("custom_vehicles_client.log", "a");
    if (f) { fputs(buf, f); fputc('\n', f); fflush(f); }
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
    if (modelId < CUSTOM_ID_MIN || modelId > CUSTOM_ID_MAX) {
        logMsg("[cv_client] ID %d fuera de rango [%d, %d]", modelId, CUSTOM_ID_MIN, CUSTOM_ID_MAX);
        return false;
    }
    if (modelId >= gta::MODEL_TABLE_SIZE) {
        logMsg("[cv_client] ID %d excede la tabla de GTA SA (%d)", modelId, gta::MODEL_TABLE_SIZE);
        return false;
    }

    void** table = gta::modelInfoTable();

    // Verificar que el slot este libre
    if (table[modelId] != nullptr) {
        logMsg("[cv_client] Slot %d ya esta ocupado", modelId);
        return false;
    }

    // Verificar que el modelo base exista
    void* baseInfo = table[BASE_VEHICLE_ID];
    if (!baseInfo) {
        logMsg("[cv_client] Model info base (%d) no disponible", BASE_VEHICLE_ID);
        return false;
    }

    // Resolver rutas absolutas
    std::string gameDir = getGameDir();
    std::string absDff = gameDir + "\\" + dffPath;
    std::string absTxd = gameDir + "\\" + txdPath;

    // Verificar que los archivos existan
    if (GetFileAttributesA(absDff.c_str()) == INVALID_FILE_ATTRIBUTES) {
        logMsg("[cv_client] DFF no encontrado: %s", absDff.c_str());
        return false;
    }
    if (GetFileAttributesA(absTxd.c_str()) == INVALID_FILE_ATTRIBUTES) {
        logMsg("[cv_client] TXD no encontrado: %s", absTxd.c_str());
        return false;
    }

    // 1. Clonar el model info del vehiculo base
    void* newInfo = std::malloc(gta::SIZEOF_VEHICLE_MODEL_INFO);
    if (!newInfo) {
        logMsg("[cv_client] malloc fallo para model info %d", modelId);
        return false;
    }
    std::memcpy(newInfo, baseInfo, gta::SIZEOF_VEHICLE_MODEL_INFO);

    // 2. Cargar TXD
    // El nombre del slot debe ser unico; usamos el ID como string
    char txdName[32];
    snprintf(txdName, sizeof(txdName), "cv_%d", modelId);
    int txdSlot = gta::AddTxdSlot(txdName);
    if (txdSlot < 0) {
        logMsg("[cv_client] AddTxdSlot fallo para modelo %d", modelId);
        std::free(newInfo);
        return false;
    }
    logMsg("[cv_client] TXD slot %d asignado para modelo %d", txdSlot, modelId);

    if (!gta::LoadTxd(txdSlot, absTxd.c_str())) {
        logMsg("[cv_client] LoadTxd fallo: %s", absTxd.c_str());
        gta::RemoveTxdSlot(txdSlot);
        std::free(newInfo);
        return false;
    }

    // 3. Establecer TXD activo (requerido antes de leer el DFF)
    gta::SetCurrentTxd(txdSlot);

    // Actualizar el indice de TXD en el model info clonado
    gta::modelField<int16_t>(newInfo, gta::OFF_TXD_INDEX) = static_cast<int16_t>(txdSlot);

    // 4. Cargar DFF
    gta::RwStream* stream = gta::RwStreamOpen(gta::rwSTREAMFILENAME, gta::rwSTREAMREAD, absDff.c_str());
    if (!stream) {
        logMsg("[cv_client] RwStreamOpen fallo: %s", absDff.c_str());
        gta::RemoveTxdSlot(txdSlot);
        std::free(newInfo);
        return false;
    }

    gta::RpClump* clump = gta::RpClumpRead(stream);
    gta::RwStreamClose(stream, nullptr);

    if (!clump) {
        logMsg("[cv_client] RpClumpStreamRead fallo para: %s", absDff.c_str());
        gta::RemoveTxdSlot(txdSlot);
        std::free(newInfo);
        return false;
    }

    // 5. Asignar la geometria al model info clonado
    gta::modelField<gta::RwObject*>(newInfo, gta::OFF_RW_OBJECT) = reinterpret_cast<gta::RwObject*>(clump);

    // 6. Registrar en la tabla de GTA SA
    table[modelId] = newInfo;

    logMsg("[cv_client] Modelo %d registrado OK (%s)", modelId, dffPath.c_str());
    return true;
}

void ModelLoader::init() {
    logMsg("[cv_client] Inicializando ModelLoader");
    loadConfig();
    logMsg("[cv_client] ModelLoader listo");
}
