#pragma once

#include <stddef.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

void* malloc(size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);
void abort(void);
