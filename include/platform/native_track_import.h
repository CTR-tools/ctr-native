#ifndef CTR_NATIVE_TRACK_IMPORT_H
#define CTR_NATIVE_TRACK_IMPORT_H

/* Installs a prepared local track directory. The returned error is static. */
const char *NativeTrackImport_Directory(const char *library, const char *source, const char *name);

#endif
