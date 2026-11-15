#ifndef CTR_NATIVE_TEXT_LIMITS_H
#define CTR_NATIVE_TEXT_LIMITS_H

// Limits count Unicode code points in the decoded TOML value, including
// placeholders and line breaks. Every limit must be positive.
enum TextLengthLimit
{
	TEXT_LIMIT_DEFAULT = 30,
	TEXT_LIMIT_MENU_ENTRY = 14,
};

#endif
