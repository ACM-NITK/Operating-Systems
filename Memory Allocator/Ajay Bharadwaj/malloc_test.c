#include <stdio.h>
#include <unistd.h>
#include "/usr/include/x86_64-linux-gnu/sys/mman.h"

struct MemAllocator
{
	void* memStart; //Start of the entire allocated memory
	void* heapStart; //Start of the usable allocated memory
	void* heapEnd; //End of the entire/usable allocated memory
	
	int sizeOfRegion; //The size of the usable allocated memory in bytes
	int actSizeOfRegion; //The size of the entire allocated memory in bytes
	
	int numObj; //Number of allocated objects
	
	//Helper variables
	int numStart;
	int* startLoc;
	char* allocated;
};
struct MemAllocator malloc;

int Mem_Init(int sizeOfRegion)
{
	//Adding the size required by the allocator data structure
	malloc.actSizeOfRegion = sizeOfRegion * (1 + sizeof(int) + sizeof(char));
	
	//Padding the size of the region to be a multiple of the page size
	size_t pageSize = getpagesize();
	if (malloc.actSizeOfRegion % pageSize != 0)
	{
		malloc.actSizeOfRegion += pageSize - (malloc.actSizeOfRegion % pageSize);
	}

	malloc.sizeOfRegion = malloc.actSizeOfRegion - sizeOfRegion * (sizeof(int) + sizeof(char));

	//Allocating virtual memory using mmap
	malloc.memStart = mmap(NULL, malloc.actSizeOfRegion, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (malloc.memStart == MAP_FAILED)
	{
		return -1;
	}
	
	//Initializing the variables
	malloc.heapStart = (void*) ((char*) malloc.memStart + sizeOfRegion * (sizeof(int) + sizeof(char)));
	malloc.heapEnd = (void*) ((char*) malloc.heapStart + malloc.sizeOfRegion);
	
	malloc.numObj = 0;
	
	malloc.numStart = 1;
	malloc.startLoc = (int*) malloc.memStart;
	malloc.allocated = (char*) malloc.memStart + sizeOfRegion * sizeof(int);
	
	*malloc.startLoc = 0;
	*malloc.allocated = 0;
	
	return 0;
}

int Mem_IsValid(void* ptr)
{
	if (!ptr)
	{
		return 0;
	}

	//If the given is address does not lie within the usable allocated memory range
	if ((ptr < malloc.heapStart) || (ptr >= malloc.heapEnd))
	{
		return 0;
	}
	
	int loc = ptr - malloc.heapStart, index;
	for (index = 0; index < malloc.numStart; index++)
	{
		if (loc < malloc.startLoc[index])
		{
			break;
		}
	}
	index--;
	
	return malloc.allocated[index];
}

int Mem_GetSize(void* ptr)
{
	if (!ptr)
	{
		return -1;
	}

	//If the given is address does not lie within the usable allocated memory range
	if ((ptr < malloc.heapStart) || (ptr >= malloc.heapEnd))
	{
		return -1;
	}
	
	int loc = ptr - malloc.heapStart, index;
	for (index = 0; index < malloc.numStart; index++)
	{
		if (loc < malloc.startLoc[index])
		{
			break;
		}
	}
	index--;
	
	//If the given address does not refer to an allocated object
	if (!malloc.allocated[index])
	{
		return -1;
	}
	
	//If the given address refers to the last object
	if (index == (malloc.numStart - 1))
	{
		return (malloc.sizeOfRegion - malloc.startLoc[index]);
	}
	
	return (malloc.startLoc[index + 1] - malloc.startLoc[index]);
}

void* Mem_Alloc(int size)
{
	void* ptr = NULL;
	for (int a = 0; a < malloc.numStart; a++)
	{
		if (!malloc.allocated[a])
		{
			//Checking whether the free memory range is large enough
			if (a == (malloc.numStart - 1))
			{
				if ((malloc.sizeOfRegion - malloc.startLoc[a]) < size)
				{
					continue;
				}
			}
			else
			{
				if ((malloc.startLoc[a+1] - malloc.startLoc[a]) < size)
				{
					continue;
				}
			}
			
			ptr = (void*) ((char*) malloc.heapStart + malloc.startLoc[a]);
			
			//If the free memory range is exactly as large as required
			if (a == (malloc.numStart - 1))
			{
				if ((malloc.sizeOfRegion - malloc.startLoc[a]) == size)
				{
					malloc.allocated[a] = 1;
					malloc.numObj++;
					break;
				}
			}
			else
			{
				if ((malloc.startLoc[a+1] - malloc.startLoc[a]) == size)
				{
					malloc.allocated[a] = 1;
					malloc.numObj++;
					break;
				}
			}
			
			//Making space for another entry by moving all the entries up by one position
			malloc.numStart++;
			for (int b = (malloc.numStart - 1); b > a; b--)
			{
				malloc.startLoc[b] = malloc.startLoc[b-1];
				malloc.allocated[b] = malloc.allocated[b-1];
			}
			
			//Making a new entry and marking the required entry as allocated
			malloc.startLoc[a+1] += size;
			malloc.allocated[a] = 1;
			malloc.numObj++;
			
			break;
		}
	}
	
	return ptr;
}

int Mem_Free(void* ptr)
{
	if (!ptr)
	{
		return -1;
	}

	//If the given is address does not lie within the usable allocated memory range
	if ((ptr < malloc.heapStart) || (ptr >= malloc.heapEnd))
	{
		return -1;
	}
	
	int loc = ptr - malloc.heapStart, index;
	for (index = 0; index < malloc.numStart; index++)
	{
		if (loc < malloc.startLoc[index])
		{
			break;
		}
	}
	index--;
	
	if (!malloc.allocated[index])
	{
		return -1;
	}

	malloc.allocated[index] = 0;
	malloc.numObj--;
	
	//Coalescing consecutive entries of free memory ranges
	for (int a = 0; a < (malloc.numStart - 1); a++)
	{
		if (!malloc.allocated[a] && !malloc.allocated[a+1])
		{
			for (int b = a+1; b < (malloc.numStart - 1); b++)
			{
				malloc.startLoc[b] = malloc.startLoc[b+1];
				malloc.allocated[b] = malloc.allocated[b+1];
			}
			
			malloc.numStart--;
			break;
		}
	}
	for (int a = 0; a < (malloc.numStart - 1); a++)
	{
		if (!malloc.allocated[a] && !malloc.allocated[a+1])
		{
			for (int b = a+1; b < (malloc.numStart - 1); b++)
			{
				malloc.startLoc[b] = malloc.startLoc[b+1];
				malloc.allocated[b] = malloc.allocated[b+1];
			}
			
			malloc.numStart--;
			break;
		}
	}

	return 0;
}

int main()
{
	if (Mem_Init(1000) == -1)
	{
		printf("An error occured in Mem_Init()\n");
	}

	int* ptr1 = (int*) Mem_Alloc(10 * sizeof(int));
	printf("%u\n", ptr1);

	char* ptr2 = (char*) Mem_Alloc(sizeof(char));
	printf("%u\n", ptr2);

	if (Mem_Free(ptr1+1))
	{
		printf("An error occured in Mem_Free()\n");
	}

	ptr1 = (int*) Mem_Alloc(5 * sizeof(int));
	printf("%u\n", ptr1);

	float* ptr3 = (float*) Mem_Alloc(sizeof(float));
	printf("%u\n", ptr3);

	char* ptr4 = (char*) Mem_Alloc(sizeof(char));
	printf("%u\n", ptr4);

	if (Mem_Free(ptr2))
	{
		printf("An error occured in Mem_Free()\n");
	}
}