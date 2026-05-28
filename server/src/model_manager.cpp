#include "model_manager.hpp"
#include <filesystem>

int ModelManager::registerModel(const std::string& name,
                                const std::string& dffPath,
                                const std::string& txdPath)
{
    if (nextId_ > CUSTOM_MODEL_MAX)
        return -1;

    // Los archivos DFF/TXD son del cliente, no del servidor — no verificar existencia

    int id = nextId_++;
    models_[id] = { id, name, dffPath, txdPath };
    return id;
}

bool ModelManager::getModel(int virtualId, VehicleModelInfo& out) const {
    auto it = models_.find(virtualId);
    if (it == models_.end()) return false;
    out = it->second;
    return true;
}
