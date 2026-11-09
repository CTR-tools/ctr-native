#include <platform/native_text_edit.h>
#include <string.h>

enum { NATIVE_TEXT_EDIT_MAX = 256 };
static struct
{
	SDL_Window *window;
	enum NativeTextEditResult result;
	size_t limit;
	char value[NATIVE_TEXT_EDIT_MAX];
} s_nativeTextEdit;

int NativeTextEdit_Begin(SDL_Window *window, const char *initial, size_t maxBytes)
{
	if (window == NULL || initial == NULL || maxBytes < 2 || maxBytes > sizeof(s_nativeTextEdit.value) ||
	    strlen(initial) >= maxBytes || s_nativeTextEdit.result == NATIVE_TEXT_EDIT_ACTIVE)
	{
		return 0;
	}
	if (!SDL_StartTextInput(window)) { return 0; }
	memset(&s_nativeTextEdit, 0, sizeof(s_nativeTextEdit));
	s_nativeTextEdit.window = window;
	s_nativeTextEdit.result = NATIVE_TEXT_EDIT_ACTIVE;
	s_nativeTextEdit.limit = maxBytes;
	SDL_strlcpy(s_nativeTextEdit.value, initial, maxBytes);
	return 1;
}

int NativeTextEdit_HandleEvent(const SDL_Event *event)
{
	if (event == NULL || s_nativeTextEdit.result != NATIVE_TEXT_EDIT_ACTIVE) { return 0; }
	if (event->type == SDL_EVENT_TEXT_INPUT)
	{
		const char *text = event->text.text;
		if (text != NULL)
		{
			size_t used = strlen(s_nativeTextEdit.value), length = strlen(text);
			if (length < s_nativeTextEdit.limit - used && strchr(text, '\n') == NULL && strchr(text, '\r') == NULL)
			{
				memcpy(s_nativeTextEdit.value + used, text, length + 1);
			}
		}
		return 1;
	}
	if (event->type != SDL_EVENT_KEY_DOWN && event->type != SDL_EVENT_KEY_UP) { return 0; }
	if (event->type == SDL_EVENT_KEY_DOWN)
	{
		if (event->key.scancode == SDL_SCANCODE_ESCAPE)
		{
			s_nativeTextEdit.result = NATIVE_TEXT_EDIT_CANCELLED;
			SDL_StopTextInput(s_nativeTextEdit.window);
		}
		else if (event->key.scancode == SDL_SCANCODE_RETURN || event->key.scancode == SDL_SCANCODE_KP_ENTER)
		{
			s_nativeTextEdit.result = NATIVE_TEXT_EDIT_ACCEPTED;
			SDL_StopTextInput(s_nativeTextEdit.window);
		}
		else if (event->key.scancode == SDL_SCANCODE_BACKSPACE)
		{
			size_t n = strlen(s_nativeTextEdit.value);
			if (n != 0)
			{
				do { n--; } while (n != 0 && ((unsigned char)s_nativeTextEdit.value[n] & 0xc0) == 0x80);
				s_nativeTextEdit.value[n] = '\0';
			}
		}
	}
	return 1;
}

enum NativeTextEditResult NativeTextEdit_Result(void) { return s_nativeTextEdit.result; }
const char *NativeTextEdit_Value(void) { return s_nativeTextEdit.value; }

void NativeTextEdit_Close(void)
{
	if (s_nativeTextEdit.result == NATIVE_TEXT_EDIT_ACTIVE) { SDL_StopTextInput(s_nativeTextEdit.window); }
	memset(&s_nativeTextEdit, 0, sizeof(s_nativeTextEdit));
}
