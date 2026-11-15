#ifndef CTR_NATIVE_TEXT_EDIT_H
#define CTR_NATIVE_TEXT_EDIT_H

#include <stddef.h>
typedef struct SDL_Window SDL_Window;
typedef union SDL_Event SDL_Event;

enum NativeTextEditResult { NATIVE_TEXT_EDIT_IDLE, NATIVE_TEXT_EDIT_ACTIVE, NATIVE_TEXT_EDIT_ACCEPTED, NATIVE_TEXT_EDIT_CANCELLED };

int NativeTextEdit_Begin(SDL_Window *window, const char *initial, size_t maxBytes);
int NativeTextEdit_BeginForGame(const char *initial, size_t maxBytes);
void NativeTextEdit_LimitCharacters(size_t maxCharacters);
int NativeTextEdit_HandleEvent(const SDL_Event *event);
enum NativeTextEditResult NativeTextEdit_Result(void);
const char *NativeTextEdit_Value(void);
void NativeTextEdit_Close(void);

#endif
