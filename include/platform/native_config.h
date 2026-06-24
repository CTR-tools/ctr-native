#include <stdbool.h>

#ifndef NATIVE_CONFIG_H
#define NATIVE_CONFIG_H

typedef struct {
    bool skipIntro;  // false = off (default), true = skip all intros
    bool skipHints;  // false = off (default), true = skip all mask hints in adventure mode
} NativeConfig;

extern NativeConfig g_config;

void NativeConfig_Load(void);

#endif