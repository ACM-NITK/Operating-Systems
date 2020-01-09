#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

int Mem_Init(int size_of_region);

int Mem_IsValid(void *ptr);

int Mem_GetSize(void *ptr);

void *Mem_Alloc(int size);

int Mem_Free(void *ptr);

#endif