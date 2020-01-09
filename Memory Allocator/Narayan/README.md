----
##Memory Allocator

To check it out, use the following functions in test.c

While compiling test.c, make sure to add mem_alloc.o as an argument

1.**int Mem_Init(int size_of_region)**

It is called during the beginning of the program to specify the maximum memory to be allocated by the allocator.
Returns 0 if the allocator could get the specified amount of memory;otherwise, returns -1.

2.**void \*Mem_Alloc(int size)**

Mem_Alloc() is similar to the library function malloc(). Mem_Alloc takes as input the size in bytes of the object to be allocated and returns a pointer to the start of that object. The function returns NULL if there is not enough free space within size_in_pages allocated by Mem_Init to satisfy this request.

3.**int Mem_Free(void \*ptr)**

Mem_Free frees the memory object that ptr falls within.Just like with the standard free(), if ptr is NULL, then no operation is performed. The function returns 0 on success and -1 if ptr to does not fall within a currently allocated object.

4.**int Mem_IsValid(void \*ptr)**

This function returns 1 if ptr falls within a currently allocated object and 0 if it does not.

5.**int Mem_GetSize(void \*ptr)**

If ptr falls within the range of a currently allocated object, then this function returns the size in bytes of that object; otherwise, the function returns -1.
