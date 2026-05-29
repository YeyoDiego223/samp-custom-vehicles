# SA-MP Custom Vehicles

Sistema para agregar vehículos con modelos personalizados en servidores SA-MP 0.3DL, sin requerir instalación manual por parte del jugador (los archivos se descargan automáticamente).

## ¿Cómo funciona?

En lugar de reemplazar los modelos en `gta3.img` manualmente, el ASI cliente:
1. **Descarga** los archivos DFF/TXD desde una URL configurada (si no existen)
2. **Parchea** `gta3.img` automáticamente en el primer arranque
3. **Redirige** el streaming de GTA SA al modelo personalizado en memoria

Los jugadores solo necesitan instalar el ASI una vez. Los modelos se descargan y configuran automáticamente.

## Requisitos

- **Servidor**: SA-MP 0.3DL-R1, GTA SA 1.0 US
- **Cliente**: GTA SA 1.0 US + SA-MP 0.3DL-R1 + el ASI instalado

## Instalación del servidor

### 1. Compilar el plugin

```bash
cd server
cmake -B build -A Win32
cmake --build build --config Release
```

Copiar `server/build/Release/custom_vehicles.dll` a `samp/plugins/`.

### 2. Configurar server.cfg

```
plugins streamer.dll custom_vehicles.dll
```

### 3. Filterscript o Gamemode

```pawn
#include <custom_vehicles>

public OnGameModeInit() {
    // Los modelos custom reemplazan slots existentes de GTA SA
    // El slot 479 (Stratum) ahora es el Porsche para los clientes con el ASI
    RegisterVehicleModel("porsche", "models/custom/porsche.dff", "models/custom/porsche.txd");
    return 1;
}

// Crear vehículo usando el slot modificado
public OnPlayerCommandText(playerid, cmdtext[]) {
    if (!strcmp(cmdtext, "/porsche")) {
        new Float:x, y, z, a;
        GetPlayerPos(playerid, x, y, z);
        GetPlayerFacingAngle(playerid, a);
        CreateVehicle(479, x+4.0, y, z, a, -1, -1, -1);
        return 1;
    }
    return 0;
}
```

## Instalación del cliente (jugadores)

Los jugadores solo necesitan instalar el ASI **una vez**:

1. Copiar `custom_vehicles.asi` a la carpeta de GTA SA/SAMP
2. Copiar `custom_vehicles.ini` a la misma carpeta
3. Listo — los modelos se descargan automáticamente al conectar

## Configuración del cliente (custom_vehicles.ini)

```ini
[custom_vehicles]
; Formato: model_id=dff_path,txd_path,dff_url,txd_url
; Las URLs son opcionales. Si no se especifican, los archivos deben estar en disco.

; Ejemplo con descarga automática:
479=models/custom/porsche.dff,models/custom/porsche.txd,https://miserver.com/models/porsche.dff,https://miserver.com/models/porsche.txd

; Ejemplo sin URL (archivos ya instalados):
560=models/custom/ferrari.dff,models/custom/ferrari.txd
```

## Flujo de primer uso (automático)

El sistema requiere **2 arranques** la primera vez que se agrega un modelo:

- **Arranque 1**: El ASI lee la streaming table de GTA SA y guarda el sector original del modelo en `%TEMP%/custom_vehicles_orig.ini`
- **Arranque 2**: El ASI parchea `gta3.img` con el modelo custom antes de que GTA SA lo abra. A partir de aquí el modelo aparece correctamente.
- **Arranques siguientes**: Carga instantánea usando el patch state guardado en `%TEMP%/custom_vehicles_patch.ini`

## Limitaciones

- Máximo 212 modelos custom (los slots de vehículos de GTA SA)
- Un slot no puede tener simultáneamente el modelo original Y el custom (elige uno u otro)
- Requiere GTA SA 1.0 US (no compatible con otras versiones)
- El patch state se guarda en `%TEMP%` — si el jugador cambia de PC necesita el proceso de 2 arranques de nuevo

## Estructura del repositorio

```
server/       Plugin SA-MP (C++) - nativo RegisterVehicleModel
client/       ASI cliente (C++) - descarga + parcheo de gta3.img
```

## Compilar el ASI cliente

```bash
cd client
cmake -B build -A Win32
cmake --build build --config Release
```

El ASI se genera en `client/build/Release/custom_vehicles.asi`.

## Créditos

Desarrollado para servidores de Roleplay en SA-MP 0.3DL-R1.
