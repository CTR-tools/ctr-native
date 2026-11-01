#ifndef CTR_NATIVE_TRACK_VALIDATE_H
#define CTR_NATIVE_TRACK_VALIDATE_H

#include <stddef.h>

/* Bounds checks for the structures consumed by LOAD_DramFileCallback and
   LOAD_VramFileCallback. These do not certify gameplay compatibility. */
int NativeTrackValidate_Lev(const void *data, size_t size);
int NativeTrackValidate_Vrm(const void *data, size_t size);

#endif
