#pragma once
#include <string>

namespace ModelLoader {

// Registra todos los modelos definidos en custom_vehicles.ini.
// Llamar solo despues de que GTA SA haya inicializado su sistema de modelos.
void init();

// Registra un modelo custom en la tabla de GTA SA.
// modelId  : ID virtual (5000-5999)
// dffPath  : ruta al .dff relativa al directorio de GTA SA
// txdPath  : ruta al .txd relativa al directorio de GTA SA
// Devuelve true si el registro fue exitoso.
bool registerModel(int modelId, const std::string& dffPath, const std::string& txdPath);

} // namespace ModelLoader
