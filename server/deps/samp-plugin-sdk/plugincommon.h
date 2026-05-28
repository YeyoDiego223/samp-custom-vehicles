#ifndef PLUGINCOMMON_H
#define PLUGINCOMMON_H

#define SAMP_PLUGIN_VERSION 0x0200

#if defined LINUX || defined __linux__ || defined __linux
    #define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
    #define PLUGIN_CALL
#elif defined WIN32 || defined _WIN32 || defined __WIN32__
    #define PLUGIN_EXPORT extern "C" __declspec(dllexport)
    #define PLUGIN_CALL __stdcall
#endif

// Valores correctos del ABI de SA-MP 0.3.7
#define SUPPORTS_VERSION        (SAMP_PLUGIN_VERSION)   // 0x0200
#define SUPPORTS_VERSION_MASK   (0xffff)
#define SUPPORTS_AMX_NATIVES    (0x00010000)
#define SUPPORTS_PROCESS_TICK   (0x00020000)

enum PLUGIN_DATA_TYPE {
    PLUGIN_DATA_LOGPRINTF    = 0x00,
    PLUGIN_DATA_AMX_EXPORTS  = 0x10,
    PLUGIN_DATA_CALLPUBLIC_FS = 0x11,
    PLUGIN_DATA_CALLPUBLIC_GS = 0x12,
};

typedef void (*logprintf_t)(const char* format, ...);

#endif // PLUGINCOMMON_H
