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
	CTR_TEST_CHECK(NativeTextEdit_Begin(window, "12345678901234567890123456789\xc3\xa8x", 64), "long existing text can be edited");
	NativeTextEdit_LimitCharacters(30);
	CTR_TEST_CHECK(SDL_utf8strlen(NativeTextEdit_Value()) == 30 && strcmp(NativeTextEdit_Value(), "12345678901234567890123456789\xc3\xa8") == 0,
	               "the character limit truncates existing text without splitting UTF-8");
	event.type = SDL_EVENT_TEXT_INPUT;
	event.text.text = "x";
	NativeTextEdit_HandleEvent(&event);
	CTR_TEST_CHECK(SDL_utf8strlen(NativeTextEdit_Value()) == 30, "input cannot exceed the character limit");
	event.type = SDL_EVENT_KEY_DOWN;
	event.key.scancode = SDL_SCANCODE_BACKSPACE;
	NativeTextEdit_HandleEvent(&event);
	event.type = SDL_EVENT_TEXT_INPUT;
	event.text.text = "\xc3\xa8";
	NativeTextEdit_HandleEvent(&event);
	CTR_TEST_CHECK(SDL_utf8strlen(NativeTextEdit_Value()) == 30 && strstr(NativeTextEdit_Value(), "\xc3\xa8") != NULL,
	               "a multibyte character fits into the final character slot");
	NativeTextEdit_Close();
	SDL_DestroyWindow(window);
	SDL_Quit();
	return CtrTest_Summary();
}
