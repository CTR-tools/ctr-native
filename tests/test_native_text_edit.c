#include <shared/ctr_test.h>
#include <SDL3/SDL.h>
#include <platform/native_text_edit.c>

int main(void)
{
	CTR_TEST_SECTION("host text editing");
	SDL_setenv_unsafe("SDL_VIDEODRIVER", "dummy", true);
	if (!SDL_Init(SDL_INIT_VIDEO)) { CtrTest_Bail("cannot initialize dummy video"); }
	SDL_Window *window = SDL_CreateWindow("text test", 64, 64, SDL_WINDOW_HIDDEN);
	if (window == NULL) { CtrTest_Bail("cannot create dummy window"); }
	CTR_TEST_CHECK(NativeTextEdit_Begin(window, "Name", 16), "an edit starts with the existing value");
	SDL_Event event = {0};
	event.type = SDL_EVENT_TEXT_INPUT;
	event.text.text = " \xc3\xa8";
	CTR_TEST_CHECK(NativeTextEdit_HandleEvent(&event) && strcmp(NativeTextEdit_Value(), "Name \xc3\xa8") == 0,
	               "text input appends UTF-8 characters");
	event.type = SDL_EVENT_KEY_DOWN;
	event.key.scancode = SDL_SCANCODE_BACKSPACE;
	NativeTextEdit_HandleEvent(&event);
	CTR_TEST_CHECK(strcmp(NativeTextEdit_Value(), "Name ") == 0, "backspace removes one UTF-8 code point");
	event.key.scancode = SDL_SCANCODE_ESCAPE;
	NativeTextEdit_HandleEvent(&event);
	CTR_TEST_CHECK(NativeTextEdit_Result() == NATIVE_TEXT_EDIT_CANCELLED, "escape cancels editing");
	NativeTextEdit_Close();
	CTR_TEST_CHECK(NativeTextEdit_Begin(window, "Title", 6), "an editor can be reopened");
	event.type = SDL_EVENT_TEXT_INPUT;
	event.text.text = "x";
	NativeTextEdit_HandleEvent(&event);
	CTR_TEST_CHECK(strcmp(NativeTextEdit_Value(), "Title") == 0, "input never exceeds its byte limit");
	event.type = SDL_EVENT_KEY_DOWN;
	event.key.scancode = SDL_SCANCODE_RETURN;
	NativeTextEdit_HandleEvent(&event);
	CTR_TEST_CHECK(NativeTextEdit_Result() == NATIVE_TEXT_EDIT_ACCEPTED, "enter accepts editing");
	NativeTextEdit_Close();
	SDL_DestroyWindow(window);
	SDL_Quit();
	return CtrTest_Summary();
}
