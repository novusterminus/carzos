/*
 */

#include <stddef.h>

#include "ktype.h"
#include "kmem.h"

typedef struct kmem_block
{
	u32 length;
	struct kmem_block *next;
} kmem_Block;

typedef struct kmem_heap
{
	// List of allocated blocks
	kmem_Block *head;
	// Other data; stats etc...
} kmem_Heap;


kmem_Heap cos_heap;


u32 kmem_init(void *heap, u32 heap_size)
{
	if ((heap == NULL) || (heap_size < sizeof(kmem_Block)))
	{
		return ERR_GENERIC;
	}

	// An empty block at the very end of heap
	kmem_Block * tailBlock = (kmem_Block *) ((u32) heap + heap_size - sizeof(kmem_Block *));
	tailBlock->length = 0U;
	tailBlock->next = NULL;

	// An empty block at the start of the heap
	kmem_Block *headBlock = (kmem_Block *) heap;
	headBlock->length = 0U;
	headBlock->next = tailBlock;

	cos_heap.head = headBlock;

	return ERR_NONE;
}

void *kmem_alloc(u32 size)
{
	if (size == 0U)
	{
		return NULL;
	}

	// Add header size
	size += sizeof(kmem_Block);
	// Align to architecture
	size = ALIGN(size, 4);

	// Search for a first-fit
	kmem_Block *currentBlock = cos_heap.head;
	while (1)
	{
		// Find the size of the hole between current allocated block and next allocated block
		u32 holeSize = (u32) currentBlock->next - (u32) currentBlock - currentBlock->length;

		if (holeSize >= size)
		{
			// Found a fit (firs-fit)
			break;
		}

		// Could not found a fit resume with next block
		currentBlock = currentBlock->next;
		if (currentBlock->next == NULL)
		{
			// Last block reached, no more memory
			return NULL;
		}
	}

	void *p;
	if (currentBlock->length == 0U)
	{
		// First block
		currentBlock->length = size;
		p = (void *) (((u32) currentBlock) + sizeof(kmem_Block));
	}
	else
	{
		// Insert the newly allocated block
		kmem_Block *newBlock = (kmem_Block *) ((u32) currentBlock + currentBlock->length);
		newBlock->next = currentBlock->next;
		newBlock->length = size;
		currentBlock->next = newBlock;

		// Convert block to memory
		p = (void *) (((u32) newBlock) + sizeof(kmem_Block));
	}

	return (p);
}

u32 kmem_free(void *p)
{
	if ((p == NULL))
	{
		return ERR_GENERIC;
	}

	// Convert memory to block
	kmem_Block *blockReturned = (kmem_Block *) ((u32) p - sizeof(kmem_Block));

	// Try to find block returned and its previous block in the allocated blocks list
	kmem_Block *previousBlock = NULL;
	kmem_Block *currentBlock = cos_heap.head;
	while (currentBlock != blockReturned)
	{
		previousBlock = currentBlock;
		currentBlock = currentBlock->next;
		if (currentBlock == NULL)
		{
			// Returned block not found
			return ERR_GENERIC;
		}
	}

	if (previousBlock == NULL)
	{
		// Returned block is the first block in the list
		currentBlock->length = 0U;
	}
	else
	{
		// Remove the returned block from the list
		previousBlock->next = currentBlock->next;
	}

	return ERR_NONE;
}

