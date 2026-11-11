#ifndef CTR_NATIVE_CUSTOM_CUP_STORAGE_H
#define CTR_NATIVE_CUSTOM_CUP_STORAGE_H

#include <custom_cup.h>

enum { NATIVE_CUSTOM_CUP_ID_SIZE = 33 };

enum NativeCustomCupStorageResult
{
	NATIVE_CUSTOM_CUP_OK,
	NATIVE_CUSTOM_CUP_NOT_FOUND,
	NATIVE_CUSTOM_CUP_INVALID,
	NATIVE_CUSTOM_CUP_IO_ERROR,
	NATIVE_CUSTOM_CUP_DUPLICATE,
};

/* The cup is borrowed for the callback and is NULL when a file cannot be read. */
typedef void (*NativeCustomCupVisitor)(void *context, const char *id,
                                      enum NativeCustomCupStorageResult result, const struct CustomCup *cup);
/* Visits each recognized file independently, including invalid or unreadable files. */
enum NativeCustomCupStorageResult NativeCustomCup_Scan(NativeCustomCupVisitor visitor, void *context);

/* IDs are opaque 32-character hexadecimal file identifiers, not cup names.
   Initialize destination cups to zero. Load leaves the destination unchanged on failure. */
enum NativeCustomCupStorageResult NativeCustomCup_Load(const char *id, struct CustomCup *cup);
/* Empty id creates a file and receives its ID on success; nonempty id updates
   an existing valid file. Names must be unique (exact UTF-8 match). The input
   cup and ID are unchanged on failure. */
enum NativeCustomCupStorageResult NativeCustomCup_Save(char id[NATIVE_CUSTOM_CUP_ID_SIZE], const struct CustomCup *cup);
/* Deletes only an existing valid cup file; callers are responsible for confirmation. */
enum NativeCustomCupStorageResult NativeCustomCup_Delete(const char *id);

#endif
