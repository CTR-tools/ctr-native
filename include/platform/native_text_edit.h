#ifndef CTR_NATIVE_TEXT_EDIT_H
#define CTR_NATIVE_TEXT_EDIT_H

#include <SDL3/SDL.h>

enum NativeTextEditResult { NATIVE_TEXT_EDIT_IDLE, NATIVE_TEXT_EDIT_ACTIVE, NATIVE_TEXT_EDIT_ACCEPTED, NATIVE_TEXT_EDIT_CANCELLED };

int NativeTextEdit_Begin(SDL_Window *window, const char *initial, size_t maxBytes);
int NativeTextEdit_BeginForGame(const char *initial, size_t maxBytes);
int NativeTextEdit_HandleEvent(const SDL_Event *event);
enum NativeTextEditResult NativeTextEdit_Result(void);
const char *NativeTextEdit_Value(void);
void NativeTextEdit_Close(void);

#endif
