#ifndef RCALC_BASE_H
#define RCALC_BASE_H

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <string_view>

// macros
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define ABS(x) (((x) < 0) ? -(x) : (x))
#define IS_BETWEEN(x, lower, upper) ((lower) <= (x) && (x) <= (upper))
#define C_ARRAY_LEN(xs) (sizeof(xs) / sizeof((xs)[0]))
#define C_ARRAY_ZERO(xs) memzero(xs, sizeof((xs)[0]) * C_ARRAY_LEN(xs))
[[noreturn]] void __assert_fail(const char *expr, const char *file, int line, const char *func);
#define assert(x) ((void)((x) || (__assert_fail(#x, __FILE__, __LINE__, __func__), 0)))
#define make(t, n, a) (t *)arena_alloc(a, n*sizeof(t), alignof(t))
#define nu(t, a) make(t, 1, a)
#define S(s) (Str){.data = s, .len = sizeof(s) / sizeof(*s) - 1}
#define STR(s) (Str){.data = s, .len = strlen(s)}
#define FMT(s) (int)(s).length(), (s).data()

// types
typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef size_t Usize;
typedef ptrdiff_t Size;
typedef uintptr_t Uptr;
typedef float F32;
typedef double F64;
typedef bool Bool;
typedef unsigned char Byte;

// A linear allocator backed by a reserved contiguous virtual address range.
// Initialize an Arena to {} before its first use. Pages are committed as
// allocations reach them. Allocated memory is uninitialized and remains valid
// until the arena is rewound past it or released.
struct Arena {
	Byte *memory;
	Size capacity;
	Size committed;
	Size used;
	Size page_size;
};

struct ArenaMark {
	const Byte *memory;
	Size used;
};
using Str = std::string_view;

// Reserves capacity bytes of contiguous virtual address space. No pages are
// committed until arena_alloc needs them. Invalid arguments or OS allocation
// failures trigger an assertion.
void arena_init(Arena *arena, Size capacity);

// Allocates size bytes with a power-of-two alignment. Invalid arguments,
// exhausted capacity, or OS commitment failures trigger an assertion.
void *arena_alloc(Arena *arena, Size size, Size alignment);

// Rewinds the arena without releasing its memory.
void arena_reset(Arena *arena);

// Saves the current allocation position. Pass the result to arena_rewind to
// discard allocations made since this point.
ArenaMark arena_save(const Arena *arena);
void arena_rewind(Arena *arena, ArenaMark mark);

// Releases the operating-system allocation and clears the arena fields.
void arena_release(Arena *arena);

Str str_copy(Str s, Arena *a);
Str str_copy_cstr(char *s, Arena *a);
Str str_concat(Str a, Str b, Arena *perm);
Str str_append(Str s, Byte c, Arena *perm);

#endif
