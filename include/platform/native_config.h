#ifndef NATIVE_CONFIG_H
#define NATIVE_CONFIG_H

typedef struct {
    int skipIntro;  // 0 = off (default), 1 = skip all intros
} NativeConfig;

extern NativeConfig g_config;

void NativeConfig_Load(void);

#endif