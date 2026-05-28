#pragma once
#include <cstdint>
#include <cstddef>

// GTA San Andreas 1.0 US (hoodlum / retail sin Steam)
// Addresses verificadas contra plugin-sdk (DK22Pac) y gtamods wiki.
namespace gta {

// -----------------------------------------------------------------------
// Tabla de model info: CBaseModelInfo*[20000]
// CModelInfo::ms_modelInfoPtrs
// -----------------------------------------------------------------------
static constexpr uintptr_t ADDR_MODEL_INFO_PTRS = 0x00A9B0C8;
static constexpr int        MODEL_TABLE_SIZE     = 20000;

// Offsets dentro de CBaseModelInfo
static constexpr ptrdiff_t OFF_TXD_INDEX  = 0x0A; // int16_t  - indice en CTxdStore
static constexpr ptrdiff_t OFF_RW_OBJECT  = 0x1C; // RwObject* / RpClump*

// Tamano de CVehicleModelInfo (hereda de CClumpModelInfo -> CBaseModelInfo)
static constexpr size_t SIZEOF_VEHICLE_MODEL_INFO = 0x308;

// -----------------------------------------------------------------------
// CTxdStore (metodos estaticos, convencion __cdecl)
// -----------------------------------------------------------------------
static constexpr uintptr_t ADDR_AddTxdSlot    = 0x731C80; // int  AddTxdSlot(const char*)
static constexpr uintptr_t ADDR_LoadTxd       = 0x7320B0; // bool LoadTxd(int, const char*)
static constexpr uintptr_t ADDR_SetCurrentTxd = 0x7319C0; // void SetCurrentTxd(int)
static constexpr uintptr_t ADDR_RemoveTxdSlot = 0x731CD0; // void RemoveTxdSlot(int)

// -----------------------------------------------------------------------
// RenderWare
// -----------------------------------------------------------------------
static constexpr uintptr_t ADDR_RwStreamOpen      = 0x7ECEF0;
static constexpr uintptr_t ADDR_RwStreamClose     = 0x7ECE20;
static constexpr uintptr_t ADDR_RpClumpStreamRead = 0x74B420;

// RwStreamOpen(type, access, pData)
enum RwStreamType       : int { rwSTREAMFILENAME = 2 };
enum RwStreamAccessType : int { rwSTREAMREAD     = 1 };

// Tipos opacos de RenderWare
struct RwStream {};
struct RpClump  {};
struct RwObject {};

// -----------------------------------------------------------------------
// Punteros a funciones
// -----------------------------------------------------------------------
using fn_AddTxdSlot    = int       (__cdecl*)(const char*);
using fn_LoadTxd       = bool      (__cdecl*)(int, const char*);
using fn_SetCurrentTxd = void      (__cdecl*)(int);
using fn_RemoveTxdSlot = void      (__cdecl*)(int);
using fn_RwStreamOpen  = RwStream* (__cdecl*)(RwStreamType, RwStreamAccessType, const void*);
using fn_RwStreamClose = bool      (__cdecl*)(RwStream*, void*);
using fn_RpClumpRead   = RpClump*  (__cdecl*)(RwStream*);

inline fn_AddTxdSlot    AddTxdSlot    = reinterpret_cast<fn_AddTxdSlot>   (ADDR_AddTxdSlot);
inline fn_LoadTxd       LoadTxd       = reinterpret_cast<fn_LoadTxd>      (ADDR_LoadTxd);
inline fn_SetCurrentTxd SetCurrentTxd = reinterpret_cast<fn_SetCurrentTxd>(ADDR_SetCurrentTxd);
inline fn_RemoveTxdSlot RemoveTxdSlot = reinterpret_cast<fn_RemoveTxdSlot>(ADDR_RemoveTxdSlot);
inline fn_RwStreamOpen  RwStreamOpen  = reinterpret_cast<fn_RwStreamOpen> (ADDR_RwStreamOpen);
inline fn_RwStreamClose RwStreamClose = reinterpret_cast<fn_RwStreamClose>(ADDR_RwStreamClose);
inline fn_RpClumpRead   RpClumpRead   = reinterpret_cast<fn_RpClumpRead>  (ADDR_RpClumpStreamRead);

// Acceso a la tabla de model info
inline void** modelInfoTable() {
    return reinterpret_cast<void**>(ADDR_MODEL_INFO_PTRS);
}

// Lee/escribe un campo de un model info por offset de bytes
template<typename T>
inline T& modelField(void* mi, ptrdiff_t offset) {
    return *reinterpret_cast<T*>(static_cast<uint8_t*>(mi) + offset);
}

} // namespace gta
