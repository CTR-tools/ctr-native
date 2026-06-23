#ifndef NATIVE_STATIC_ASSERT_H
#define NATIVE_STATIC_ASSERT_H

// Retail struct layout asserts (offsetof/sizeof checks) verify byte-exact ABI
// parity with the original PS1 binary. They assume retail pointer width
// (4 bytes). On a genuinely 64-bit host (macOS/arm64, where pointers are
// 8 bytes) any struct that embeds a real C pointer field shifts every later
// field's offset, so these asserts correctly fail to compile -- not because
// the native layout is wrong, but because a 64-bit pointer cannot occupy a
// retail 4-byte slot. See "Known Gap: Retail-Offset Asserts vs. Embedded
// Pointers" in docs/MEMORY_MODEL.md.
//
// CTR_STATIC_ASSERT_LAYOUT wraps every offsetof/sizeof layout assert:
//   - 32-bit hosts (Windows/Linux, forced -m32): identical to _Static_assert,
//     so those builds remain the byte-parity guardians.
//   - 64-bit hosts (__LP64__): compiled out. Pure PSX-shaped structs (no
//     embedded host pointers) have identical layout at 64-bit anyway, so
//     nothing is lost there; structs that embed host pointers cannot satisfy
//     retail offsets and are validated by the 32-bit builds instead.
//
// Value asserts (enum constants, flag bit values, etc. -- anything not
// involving offsetof/sizeof) stay as plain _Static_assert on all platforms.
//
// This header is force-included for every translation unit (see CMakeLists.txt)
// so the macro is visible before the first layout assert in any header.

#if defined(__LP64__) || defined(_WIN64)
#define CTR_STATIC_ASSERT_LAYOUT(...)
#else
#define CTR_STATIC_ASSERT_LAYOUT(...) _Static_assert(__VA_ARGS__)
#endif

#endif // NATIVE_STATIC_ASSERT_H
