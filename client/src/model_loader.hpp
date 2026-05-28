#pragma once
#include <string>

namespace ModelLoader {

// Registra todos los modelos definidos en custom_vehicles.ini.
// Llamar solo despues de que GTA SA haya inicializado su sistema de modelos.
void init();

// Aplica patches de null-check en 0x4C4BC0 y 0x4C48D0 inmediatamente.
// Llamar desde DllMain antes de que SA-MP intente cargar modelos custom.
void applyPatches();

// Restaura vtable[1] y vtable[2] de CVehicleModelInfo a sus valores originales
// del binario de GTA SA, deshaciendo los patches de SA-MP que retornan null.
// Llamar despues de que la tabla de modelos este inicializada.
void restoreVtable();

// Registra un modelo custom en la tabla de GTA SA.
// modelId  : ID virtual (5000-5999)
// dffPath  : ruta al .dff relativa al directorio de GTA SA
// txdPath  : ruta al .txd relativa al directorio de GTA SA
// Devuelve true si el registro fue exitoso.
bool registerModel(int modelId, const std::string& dffPath, const std::string& txdPath);

} // namespace ModelLoader
