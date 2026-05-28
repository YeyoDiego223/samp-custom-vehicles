// Redirige las funciones AMX al servidor SA-MP via la tabla de punteros.
#include "amx/amx.h"

extern void *pAMXFunctions;

#define AMX_FN(i) (((void**)pAMXFunctions)[i])

// Indices de la tabla (orden fijo del ABI de SA-MP)
// SA-MP 0.3DL agrego 2 funciones extra despues de NumNatives (idx 23),
// corriendo todos los indices siguientes +2 respecto al SDK estandar.
// amx_Register: estandar=31, 0.3DL=33 (confirmado via offset 0x84 en sscanf.dll)
enum {
    FUNC_Align16 = 0, FUNC_Align32, FUNC_Allot, FUNC_Callback, FUNC_Clone,
    FUNC_Exec, FUNC_FindNative, FUNC_FindPublic, FUNC_FindPubVar,
    FUNC_FindTagId, FUNC_Flags, FUNC_GetAddr, FUNC_GetNative,
    FUNC_GetPublic, FUNC_GetPubVar, FUNC_GetString, FUNC_GetTag,
    FUNC_GetUserData, FUNC_Init, FUNC_InitJIT, FUNC_MemInfo,
    FUNC_NameLength, FUNC_NativeInfo, FUNC_NumNatives,  // = 23 (igual en DL)
    FUNC_DL_Extra1, FUNC_DL_Extra2,                    // = 24, 25 (extras de 0.3DL)
    FUNC_NumPublics, FUNC_NumPubVars, FUNC_NumTags,
    FUNC_Push, FUNC_PushArray, FUNC_PushString, FUNC_RaiseError,
    FUNC_Register,      // = 33 en 0.3DL
    FUNC_Release, FUNC_SetCallback, FUNC_SetDebugHook,
    FUNC_SetString, FUNC_SetUserData,
    FUNC_StrLen,        // = 39 en 0.3DL
    FUNC_UTF8Check, FUNC_UTF8Get, FUNC_UTF8Len, FUNC_UTF8Put
};

typedef int  (AMXAPI *fn_Allot)     (AMX*, int, cell*, cell**);
typedef int  (AMXAPI *fn_Callback)  (AMX*, cell, cell*, cell*);
typedef int  (AMXAPI *fn_Exec)      (AMX*, cell*, int);
typedef int  (AMXAPI *fn_FindPublic)(AMX*, const char*, int*);
typedef int  (AMXAPI *fn_FindPubVar)(AMX*, const char*, cell*);
typedef int  (AMXAPI *fn_GetAddr)   (AMX*, cell, cell**);
typedef int  (AMXAPI *fn_GetPublic) (AMX*, int, char*);
typedef int  (AMXAPI *fn_GetString) (char*, const cell*, int, size_t);
typedef int  (AMXAPI *fn_NameLength)(AMX*, int*);
typedef int  (AMXAPI *fn_NumNatives)(AMX*, int*);
typedef int  (AMXAPI *fn_NumPublics)(AMX*, int*);
typedef int  (AMXAPI *fn_Push)      (AMX*, cell);
typedef int  (AMXAPI *fn_PushArray) (AMX*, cell*, cell**, const cell[], int);
typedef int  (AMXAPI *fn_PushString)(AMX*, cell*, cell**, const char*, int, int);
typedef int  (AMXAPI *fn_RaiseError)(AMX*, int);
typedef int  (AMXAPI *fn_Register)  (AMX*, const AMX_NATIVE_INFO*, int);
typedef int  (AMXAPI *fn_Release)   (AMX*, cell);
typedef int  (AMXAPI *fn_SetString) (cell*, const char*, int, int, size_t);
typedef int  (AMXAPI *fn_StrLen)    (const cell*, int*);
typedef int  (AMXAPI *fn_Align16)   (uint16_t*);
typedef int  (AMXAPI *fn_Align32)   (uint32_t*);

int AMXAPI amx_Allot    (AMX *a, int c, cell *aa, cell **p) { return ((fn_Allot)    AMX_FN(FUNC_Allot))(a,c,aa,p); }
int AMXAPI amx_Callback (AMX *a, cell i, cell *r, cell *p)  { return ((fn_Callback) AMX_FN(FUNC_Callback))(a,i,r,p); }
int AMXAPI amx_Exec     (AMX *a, cell *r, int i)             { return ((fn_Exec)     AMX_FN(FUNC_Exec))(a,r,i); }
int AMXAPI amx_FindPublic(AMX *a, const char *n, int *i)    { return ((fn_FindPublic)AMX_FN(FUNC_FindPublic))(a,n,i); }
int AMXAPI amx_FindPubVar(AMX *a, const char *n, cell *c)   { return ((fn_FindPubVar)AMX_FN(FUNC_FindPubVar))(a,n,c); }
int AMXAPI amx_GetAddr  (AMX *a, cell c, cell **p)          { return ((fn_GetAddr)  AMX_FN(FUNC_GetAddr))(a,c,p); }
int AMXAPI amx_GetPublic(AMX *a, int i, char *n)            { return ((fn_GetPublic)AMX_FN(FUNC_GetPublic))(a,i,n); }
int AMXAPI amx_GetString(char *d, const cell *s, int w, size_t z) { return ((fn_GetString)AMX_FN(FUNC_GetString))(d,s,w,z); }
int AMXAPI amx_NameLength(AMX *a, int *l)                   { return ((fn_NameLength)AMX_FN(FUNC_NameLength))(a,l); }
int AMXAPI amx_NumNatives(AMX *a, int *n)                   { return ((fn_NumNatives)AMX_FN(FUNC_NumNatives))(a,n); }
int AMXAPI amx_NumPublics(AMX *a, int *n)                   { return ((fn_NumPublics)AMX_FN(FUNC_NumPublics))(a,n); }
int AMXAPI amx_Push     (AMX *a, cell v)                    { return ((fn_Push)     AMX_FN(FUNC_Push))(a,v); }
int AMXAPI amx_PushArray(AMX *a, cell *aa, cell **p, const cell ar[], int n) { return ((fn_PushArray)AMX_FN(FUNC_PushArray))(a,aa,p,ar,n); }
int AMXAPI amx_PushString(AMX *a, cell *aa, cell **p, const char *s, int pk, int w) { return ((fn_PushString)AMX_FN(FUNC_PushString))(a,aa,p,s,pk,w); }
int AMXAPI amx_RaiseError(AMX *a, int e)                    { return ((fn_RaiseError)AMX_FN(FUNC_RaiseError))(a,e); }
int AMXAPI amx_Register (AMX *a, const AMX_NATIVE_INFO *n, int num) { return ((fn_Register)AMX_FN(FUNC_Register))(a,n,num); }
int AMXAPI amx_Release  (AMX *a, cell c)                    { return ((fn_Release)  AMX_FN(FUNC_Release))(a,c); }
int AMXAPI amx_SetString(cell *d, const char *s, int pk, int w, size_t z) { return ((fn_SetString)AMX_FN(FUNC_SetString))(d,s,pk,w,z); }
int AMXAPI amx_StrLen   (const cell *s, int *l)             { return ((fn_StrLen)   AMX_FN(FUNC_StrLen))(s,l); }
int AMXAPI amx_Align16  (uint16_t *v)                       { return ((fn_Align16)  AMX_FN(FUNC_Align16))(v); }
int AMXAPI amx_Align32  (uint32_t *v)                       { return ((fn_Align32)  AMX_FN(FUNC_Align32))(v); }
