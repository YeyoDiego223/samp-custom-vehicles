#pragma once
#include <string>

namespace ModelLoader {

// Registra todos los modelos definidos en custom_vehicles.ini.
// Llamar solo despues de que GTA SA haya inicializado su sistema de modelos.
// Fase 1 (DllMain, antes de que GTA SA abra gta3.img):
// lee cv_orig.ini y parchea gta3.img si hay info de streaming guardada.
void earlyPatch();

// Fase 2 (InitThread, despues del streaming):
// lee streaming table para modelos nuevos, actualiza tabla en memoria.
void init();

// Aplica patches de null-check en DllMain (antes de SA-MP).
void applyPatches();

// Restaura vtable[1/2] al original del binario de GTA SA.
void restoreVtable();

// Registra un modelo custom en la tabla de GTA SA.
// modelId  : ID virtual (5000-5999)
// dffPath  : ruta al .dff relativa al directorio de GTA SA
// txdPath  : ruta al .txd relativa al directorio de GTA SA
// Devuelve true si el registro fue exitoso.
bool registerModel(int modelId, const std::string& dffPath, const std::string& txdPath);

} // namespace ModelLoader
