#if defined _custom_vehicles_included
    #endinput
#endif
#define _custom_vehicles_included

// Rango de IDs para modelos custom
#define CUSTOM_MODEL_BASE 5000
#define CUSTOM_MODEL_MAX  5999

/**
 * Registra un modelo de vehiculo personalizado.
 *
 * name  - Nombre identificador del modelo
 * dff   - Ruta al archivo .dff (geometria)
 * txd   - Ruta al archivo .txd (texturas)
 *
 * Devuelve el ID virtual (5000-5999) o -1 si falla.
 */
native RegisterVehicleModel(const name[], const dff[], const txd[]);

/**
 * Devuelve cuantos modelos custom estan registrados.
 */
native GetCustomVehicleModelCount();

/**
 * Devuelve true si el modelid dado es un modelo custom (5000-5999).
 */
native bool:IsCustomVehicleModel(modelid);
