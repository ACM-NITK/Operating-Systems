#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

#define PAGE_SIZE 4096

//for struct packing without padding
#pragma pack(1)

struct node
{
	struct node *next;
	char free; //boolean variable indicating free or not
};

typedef struct node node_t;
typedef unsigned long int ul;
node_t *head;
#define true 1
#define false 0

/*
	STRUCTURE OF THE VIRTUAL SPACE

	node_t --------> node_t ---------> node_t -------->node_t----------> NULL
			MEMORY			MEMORY				MEMORY			MEMORY
			FOR USE			FOR USE				FOr USE			FOR USE

*/

int Mem_Init(int size_of_region)
{
	if ((size_of_region % PAGE_SIZE))
		size_of_region += (PAGE_SIZE - size_of_region % PAGE_SIZE);
	head = (node_t *)mmap(NULL, size_of_region, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (head == NULL)
		return -1;
	head->free = true;
	/*
		node_t -------------------> node_t
	  	head       free-memory     last_node
	*/

	//space is used for 2 node_t
	ul free_size = size_of_region - 2 * sizeof(node_t);
	head->next = (node_t *)((ul)head + sizeof(node_t) + free_size);
	head->next->next = NULL, head->next->free = false;
	return 0;
}

void *Mem_Alloc(int size)
{
	node_t *temp = head;
	while (temp->next != NULL)
	{
		if (temp->free == true)
		{
			ul free_space = (ul)temp->next - (ul)temp - sizeof(node_t);
			void *return_address = (void *)((ul)temp + sizeof(node_t));

			/*if free space is exactly equal to size, just change the state of the block to "not free"
			No extra blocks created */
			if (free_space == size)
			{
				temp->free = false;
				return return_address;
			}

			//if there is enough space for also inserting another node_t in between
			if (free_space >= size + sizeof(node_t))
			{
				//change the state to "not free" and add another free block after the current block
				node_t *curr_next = temp->next;
				temp->free = false;
				temp->next = (node_t *)(sizeof(node_t) + size + (ul)temp);
				temp->next->free = true;
				temp->next->next = curr_next;
				return return_address;
			}
		}
		temp = temp->next;
	}
	return NULL;
}

int Mem_IsValid(void *ptr)
{
	node_t *temp = head;
	while (temp->next != NULL)
	{
		if (temp->free == false)
		{
			if ((ul)ptr >= (ul)temp + sizeof(node_t) && (ul)ptr < (ul)temp->next)
			{
				return 1;
			}
		}
		temp = temp->next;
		if ((ul)ptr < (ul)temp)
			break;
	}
	return 0;
}

int Mem_GetSize(void *ptr)
{
	node_t *temp = head;
	while (temp->next != NULL)
	{
		if (temp->free == false)
		{
			if ((ul)ptr >= (ul)temp + sizeof(node_t) && (ul)ptr < (ul)temp->next)
			{
				return (ul)temp->next - (ul)temp - sizeof(node_t);
			}
		}
		temp = temp->next;
		if ((ul)ptr < (ul)temp)
			break;
	}
	return -1;
}

int Mem_Free(void *ptr)
{
	if (head->free == false)
	{
		if ((ul)ptr >= (ul)head + sizeof(node_t) && (ul)ptr < (ul)head->next)
		{
			//if the next block is also free, merge the two blocks
			if (head->next->next != NULL && head->next->free == true)
			{
				head->next = head->next->next;
				head->free = true;
				return 0;
			}
			else
			{
				head->free = true;
			}
		}
	}
	node_t *temp = head->next;
	node_t *prev = head;
	while (temp->next != NULL)
	{
		if (temp->free == false)
		{
			if ((ul)ptr >= (ul)temp + sizeof(node_t) && (ul)ptr < (ul)temp->next)
			{
				//if the previous is free, merge with previous
				if (prev->free == true)
				{
					prev->next = temp->next;
					temp = prev;
				}

				//if the next is free, merge with next
				if (temp->next->free == true)
				{
					temp->next = temp->next->next;
				}

				temp->free = true;
				return 0;
			}
		}
		temp = temp->next;
		prev = prev->next;
		if ((ul)ptr < (ul)temp)
			break;
	}
	return -1;
}
