#include "plugincommon.h"
#include "amx/amx.h"
#include "model_manager.hpp"

void        *pAMXFunctions;
logprintf_t  logprintf;

// Lee un string Pawn accediendo directamente al segmento de datos del AMX,
// sin usar la tabla de dispatch (amx_GetAddr/amx_StrLen tienen indices erroneos en 0.3DL).
static std::string amxGetString(AMX *amx, cell relativeAddr) {
    unsigned char *data = amx->data;
    if (!data) {
        // AMX_HEADER::dat esta en offset 16 del header binario
        data = amx->base + *reinterpret_cast<int32_t *>(amx->base + 16);
    }
    if (relativeAddr < 0 || relativeAddr >= amx->stp) return "";

    const cell *phys = reinterpret_cast<const cell *>(data + relativeAddr);
    std::string s;
    s.reserve(64);
    for (int i = 0; i < 4096 && phys[i] != 0; ++i)
        s += static_cast<char>(phys[i] & 0xFF);
    return s;
}

// native RegisterVehicleModel(const name[], const dff[], const txd[]);
static cell AMX_NATIVE_CALL n_RegisterVehicleModel(AMX *amx, cell *params) {
    if (params[0] < 3 * (cell)sizeof(cell)) return -1;

    std::string name = amxGetString(amx, params[1]);
    std::string dff  = amxGetString(amx, params[2]);
    std::string txd  = amxGetString(amx, params[3]);

    int id = ModelManager::getInstance().registerModel(name, dff, txd);

    if (id == -1)
        logprintf("[custom_vehicles] Error al registrar modelo '%s' (archivos no encontrados o limite alcanzado)", name.c_str());
    else
        logprintf("[custom_vehicles] Modelo '%s' registrado con ID %d", name.c_str(), id);

    return id;
}

// native GetCustomVehicleModelCount();
static cell AMX_NATIVE_CALL n_GetCustomVehicleModelCount(AMX *amx, cell *params) {
    return ModelManager::getInstance().getModelCount();
}

// native bool:IsCustomVehicleModel(modelid);
static cell AMX_NATIVE_CALL n_IsCustomVehicleModel(AMX *amx, cell *params) {
    if (params[0] < 1 * (cell)sizeof(cell)) return 0;
    VehicleModelInfo info;
    return ModelManager::getInstance().getModel((int)params[1], info) ? 1 : 0;
}

// -----------------------------------------------------------------------

static AMX_NATIVE_INFO PluginNatives[] = {
    {"RegisterVehicleModel",       n_RegisterVehicleModel},
    {"GetCustomVehicleModelCount", n_GetCustomVehicleModelCount},
    {"IsCustomVehicleModel",       n_IsCustomVehicleModel},
    {nullptr, nullptr}
};

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
    return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
    pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
    logprintf      = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
    logprintf("[custom_vehicles] Plugin cargado v0.1");
    return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
    logprintf("[custom_vehicles] Plugin descargado");
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
    return amx_Register(amx, PluginNatives, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
    return AMX_ERR_NONE;
}
