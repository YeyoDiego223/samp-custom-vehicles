#ifndef AMX_H_INCLUDED
#define AMX_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>

// Tipos basicos
typedef int32_t  cell;
typedef uint32_t ucell;

// AMXAPI debe definirse ANTES de usarlo en structs
#if !defined AMXAPI
    #if defined STDECL
        #define AMXAPI __stdcall
    #elif defined CDECL
        #define AMXAPI __cdecl
    #else
        #define AMXAPI
    #endif
#endif

#if !defined AMX_NATIVE_CALL
    #define AMX_NATIVE_CALL
#endif

// Codigos de error
#define AMX_ERR_NONE        0
#define AMX_ERR_EXIT        1
#define AMX_ERR_ASSERT      2
#define AMX_ERR_STACKERR    3
#define AMX_ERR_BOUNDS      4
#define AMX_ERR_MEMACCESS   5
#define AMX_ERR_INVINSTR    6
#define AMX_ERR_STACKLOW    7
#define AMX_ERR_HEAPLOW     8
#define AMX_ERR_CALLBACK    9
#define AMX_ERR_NATIVE      10
#define AMX_ERR_DIVIDE      11
#define AMX_ERR_SLEEP       12
#define AMX_ERR_INVSTATE    13
#define AMX_ERR_MEMORY      16
#define AMX_ERR_FORMAT      17
#define AMX_ERR_VERSION     18
#define AMX_ERR_NOTFOUND    19
#define AMX_ERR_INDEX       20
#define AMX_ERR_DEBUG       21
#define AMX_ERR_INIT        22
#define AMX_ERR_USERDATA    23
#define AMX_ERR_INIT_JIT    24
#define AMX_ERR_PARAMS      25
#define AMX_ERR_DOMAIN      26
#define AMX_ERR_GENERAL     27

// Flags
#define AMX_FLAG_DEBUG      0x02
#define AMX_FLAG_COMPACT    0x04
#define AMX_FLAG_BYTEOPC    0x08
#define AMX_FLAG_NOCHECKS   0x10
#define AMX_FLAG_NTVREG     0x1000
#define AMX_FLAG_JITC       0x2000
#define AMX_FLAG_BROWSE     0x4000
#define AMX_FLAG_RELOC      0x8000

// Forward declaration necesaria para AMX_NATIVE_INFO
struct tagAMX;

typedef cell (AMXAPI *AMX_NATIVE)   (struct tagAMX *amx, cell *params);
typedef int  (AMXAPI *AMX_CALLBACK) (struct tagAMX *amx, cell index, cell *result, cell *params);
typedef int  (AMXAPI *AMX_DEBUG)    (struct tagAMX *amx);

typedef struct tagAMX_NATIVE_INFO {
    const char *name;
    AMX_NATIVE  func;
} AMX_NATIVE_INFO;

typedef struct tagAMX {
    unsigned char *base;
    unsigned char *data;
    AMX_CALLBACK   callback;
    AMX_DEBUG      debug;
    cell           cip;
    cell           frm;
    cell           hea;
    cell           hlw;
    cell           stk;
    cell           stp;
    int            flags;
    long           usertags[4];
    void          *userdata[4];
    int            error;
    int            paramcount;
    cell           pri;
    cell           alt;
    cell           reset_stk;
    cell           reset_hea;
    cell           sysreq_d;
} AMX;

#ifdef __cplusplus
extern "C" {
#endif

int AMXAPI amx_Allot    (AMX *amx, int cells, cell *amx_addr, cell **phys_addr);
int AMXAPI amx_Callback (AMX *amx, cell index, cell *result, cell *params);
int AMXAPI amx_Exec     (AMX *amx, cell *retval, int index);
int AMXAPI amx_FindPublic(AMX *amx, const char *funcname, int *index);
int AMXAPI amx_FindPubVar(AMX *amx, const char *varname, cell *amx_addr);
int AMXAPI amx_GetAddr  (AMX *amx, cell amx_addr, cell **phys_addr);
int AMXAPI amx_GetPublic(AMX *amx, int index, char *funcname);
int AMXAPI amx_GetString(char *dest, const cell *source, int use_wchar, size_t size);
int AMXAPI amx_NameLength(AMX *amx, int *length);
int AMXAPI amx_NumNatives(AMX *amx, int *number);
int AMXAPI amx_NumPublics(AMX *amx, int *number);
int AMXAPI amx_Push     (AMX *amx, cell value);
int AMXAPI amx_PushArray(AMX *amx, cell *amx_addr, cell **phys_addr, const cell array[], int numcells);
int AMXAPI amx_PushString(AMX *amx, cell *amx_addr, cell **phys_addr, const char *string, int pack, int use_wchar);
int AMXAPI amx_RaiseError(AMX *amx, int error);
int AMXAPI amx_Register (AMX *amx, const AMX_NATIVE_INFO *nativelist, int number);
int AMXAPI amx_Release  (AMX *amx, cell amx_addr);
int AMXAPI amx_SetString(cell *dest, const char *source, int pack, int use_wchar, size_t size);
int AMXAPI amx_StrLen   (const cell *cstring, int *length);
int AMXAPI amx_Align16  (uint16_t *v);
int AMXAPI amx_Align32  (uint32_t *v);

#ifdef __cplusplus
}
#endif

#endif // AMX_H_INCLUDED
