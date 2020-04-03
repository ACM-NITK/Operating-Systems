#include <stdio.h>
#include "Helpers.h"

void set(bitmap_t *bitmap, int index)
{
	int curr_index = -1;
	int integer_index = index / (sizeof(int) * 8);
	bitmap->integer[integer_index] &= (1 << (index % (sizeof(int) * 8)));
}

void unset(bitmap_t *bitmap, int index)
{
	int curr_index = -1;
	int integer_index = index / (sizeof(int) * 8);
	bitmap->integer[integer_index] ^= (1 << (index % (sizeof(int) * 8)));
}