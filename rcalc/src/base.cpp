#include <cstring>
#if defined(__linux__) && !defined(_DEFAULT_SOURCE)
#define _DEFAULT_SOURCE
#endif

#include "base.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#else
#error "Arena allocator supports Windows and Linux only"
#endif

[[noreturn]] void __assert_fail(const char *expr, const char *file, int line, const char *func)
{
	fprintf(stderr, "Assertion failed: %s (%s: %s: %d)\n", expr, file, func, line);
	abort();
}

void arena_init(Arena *arena, Size capacity)
{
	assert(arena != 0);
	assert(arena->memory == 0);
	assert(capacity > 0);

	arena->memory = 0;
	arena->capacity = 0;
	arena->committed = 0;
	arena->used = 0;
	arena->page_size = 0;

#if defined(_WIN32)
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	Size page_size = static_cast<Size>(system_info.dwPageSize);
	assert(page_size > 0);
	void *memory = VirtualAlloc(0, capacity, MEM_RESERVE, PAGE_NOACCESS);
	assert(memory != 0);
#elif defined(__linux__)
	long system_page_size = sysconf(_SC_PAGESIZE);
	assert(system_page_size > 0);
	Size page_size = static_cast<Size>(system_page_size);
	void *memory = mmap(0, capacity, PROT_NONE,
	                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(memory != MAP_FAILED);
#endif

	arena->memory = static_cast<Byte *>(memory);
	arena->capacity = capacity;
	arena->page_size = page_size;
}

static void arena_commit(Arena *arena, Size required)
{
	assert(required > arena->committed);
	assert(required <= arena->capacity);
	assert(arena->page_size > 0);
	assert(required <= PTRDIFF_MAX - (arena->page_size - 1));

	Size new_committed =
	    ((required + arena->page_size - 1) / arena->page_size) * arena->page_size;
	Size amount = new_committed - arena->committed;
	assert(amount > 0);
	Byte *start = arena->memory + arena->committed;

#if defined(_WIN32)
	void *committed = VirtualAlloc(start, amount, MEM_COMMIT, PAGE_READWRITE);
	assert(committed != 0);
#elif defined(__linux__)
	int result = mprotect(start, amount, PROT_READ | PROT_WRITE);
	assert(result == 0);

	// Linux enables access with mprotect, but allocates anonymous physical
	// pages on first write. Touch each new page now so allocation is realized
	// when this commit operation succeeds.
	volatile Byte *pages = arena->memory;
	for (Size offset = arena->committed; offset < new_committed;
	     offset += arena->page_size)
		pages[offset] = 0;
#endif

	arena->committed = new_committed;
}

void *arena_alloc(Arena *arena, Size size, Size alignment)
{
	assert(arena != 0);
	assert(arena->memory != 0);
	assert(size > 0);
	assert(alignment > 0);
	assert((alignment & (alignment - 1)) == 0);
	assert(arena->used <= arena->capacity);
	assert(arena->page_size > 0);

	Uptr current = reinterpret_cast<Uptr>(arena->memory + arena->used);
	Uptr mask = static_cast<Uptr>(alignment - 1);
	assert(current <= UINTPTR_MAX - mask);
	Uptr aligned = (current + mask) & ~mask;
	Size offset = static_cast<Size>(aligned - reinterpret_cast<Uptr>(arena->memory));
	assert(offset <= arena->capacity);
	assert(size <= arena->capacity - offset);

	Size end = offset + size;
	if (end > arena->committed)
		arena_commit(arena, end);

	arena->used = end;
	return arena->memory + offset;
}

void arena_reset(Arena *arena)
{
	assert(arena != 0);
	assert(arena->memory != 0);
	arena->used = 0;
}

ArenaMark arena_save(const Arena *arena)
{
	assert(arena != 0);
	assert(arena->memory != 0);
	ArenaMark mark = {arena->memory, arena->used};
	return mark;
}

void arena_rewind(Arena *arena, ArenaMark mark)
{
	assert(arena != 0);
	assert(arena->memory != 0);
	assert(mark.memory == arena->memory);
	assert(mark.used <= arena->used);

	arena->used = mark.used;
}

void arena_release(Arena *arena)
{
	assert(arena != 0);
	assert(arena->memory != 0);

#if defined(_WIN32)
	BOOL released = VirtualFree(arena->memory, 0, MEM_RELEASE);
	assert(released != 0);
#elif defined(__linux__)
	int released = munmap(arena->memory, arena->capacity);
	assert(released == 0);
#endif
	arena->memory = 0;
	arena->capacity = 0;
	arena->committed = 0;
	arena->used = 0;
	arena->page_size = 0;
}

Str str_copy(Str s, Arena *a)
{
	char *p = make(char, s.size(), a);
	if (s.size())
		memcpy(p, s.data(), s.size());
	return Str{p, s.size()};
}

Str str_copy_cstr(char *s, Arena *a)
{
	return str_copy(Str{s, (size_t)strlen(s)}, a);
}

Str str_concat(Str a, Str b, Arena *perm)
{
	Str s1;
	if (perm->memory + perm->used == (Byte *)a.data() + a.size()) {
		s1 = a;
	} else {
		s1 = str_copy(a, perm);
	}
	str_copy(b, perm);
	return Str{s1.data(), a.size() + b.size()};
}

Str str_append(Str s, Byte c, Arena *perm)
{
	Str s1;
	if (perm->memory + perm->used == (Byte *)s.data() + s.size()) {
		s1 = s;
	} else {
		s1 = str_copy(s, perm);
	}
	*nu(Byte, perm) = c;
	return Str{s1.data(), s1.size() + 1};
}
