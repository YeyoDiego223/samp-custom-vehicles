#pragma once
#include <string>
#include <unordered_map>

// Rango de IDs virtuales para modelos custom (empieza en 5000, fuera del rango SA 400-611)
constexpr int CUSTOM_MODEL_BASE = 5000;
constexpr int CUSTOM_MODEL_MAX  = 5999;

struct VehicleModelInfo {
    int         virtualId;
    std::string name;
    std::string dffPath;
    std::string txdPath;
};

class ModelManager {
public:
    static ModelManager& getInstance() {
        static ModelManager instance;
        return instance;
    }

    // Registra un modelo y devuelve su ID virtual (>= 5000), o -1 si falla
    int registerModel(const std::string& name,
                      const std::string& dffPath,
                      const std::string& txdPath);

    bool getModel(int virtualId, VehicleModelInfo& out) const;
    int  getModelCount() const { return (int)models_.size(); }

private:
    ModelManager() = default;

    std::unordered_map<int, VehicleModelInfo> models_;
    int nextId_ = CUSTOM_MODEL_BASE;
};
