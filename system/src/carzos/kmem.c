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

	size += sizeof(kmem_Block);

	size = (size + 3U) & ~(u32) 3U;

	kmem_Block *currentBlock = cos_heap.head;
	while (1)
	{
		u32 holeSize = (u32) currentBlock->next - (u32) currentBlock;
		holeSize -= currentBlock->length;

		if (holeSize >= size)
		{
			break;
		}
		currentBlock = currentBlock->next;
		if (currentBlock->next == NULL)
		{
			/* Failed, we are at the end of the list */
			return NULL;
		}
	}

	void *p;
	if (currentBlock->length == 0U)
	{
		/* No block is allocated, set the Length of the first element */
		currentBlock->length = size;
		p = (kmem_Block *) (((u32) currentBlock) + sizeof(kmem_Block));
	}
	else
	{
		/* Insert new list element into the memory list */
		kmem_Block *newBlock = (kmem_Block *) ((u32) currentBlock + currentBlock->length);
		newBlock->next = currentBlock->next;
		newBlock->length = size;
		currentBlock->next = newBlock;
		p = (kmem_Block *) (((u32) newBlock) + sizeof(kmem_Block));
	}

	return (p);
}

u32 kmem_free(void *p)
{
	if ((p == NULL))
	{
		return ERR_GENERIC;
	}

	kmem_Block *blockReturned = (kmem_Block *) ((u32) p - sizeof(kmem_Block));

	/* Set list header */
	kmem_Block *previousBlock = NULL;
	kmem_Block *currentBlock = cos_heap.head;
	while (currentBlock != blockReturned)
	{
		previousBlock = currentBlock;
		currentBlock = currentBlock->next;
		if (currentBlock == NULL)
		{
			/* Valid Memory block not found */
			return ERR_GENERIC;
		}
	}

	if (previousBlock == NULL)
	{
		/* First block to be released, only set length to 0 */
		currentBlock->length = 0U;
	}
	else
	{
		/* Discard block from chain list */
		previousBlock->next = currentBlock->next;
	}

	return ERR_NONE;
}

