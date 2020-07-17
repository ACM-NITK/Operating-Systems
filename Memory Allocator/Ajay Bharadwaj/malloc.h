#ifndef MALLOC_H
#define MALLOC_H

int Mem_Init(int sizeOfRegion);

int Mem_IsValid(void* ptr);

int Mem_GetSize(void* ptr);

void* Mem_Alloc(int size);

int Mem_Free(void* ptr);

#endif