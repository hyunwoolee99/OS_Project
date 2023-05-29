/**********************************************************************
 * Copyright (c) 2020
 *  Jinwoo Jeong <jjw8967@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdint.h>

#include "malloc.h"
#include "types.h"
#include "list_head.h"

#define ALIGNMENT 32
#define HDRSIZE sizeof(header_t)

static LIST_HEAD(free_list); // Don't modify this line
static algo_t g_algo;        // Don't modify this line
static void *bp;             // Don't modify thie line

/***********************************************************************
 * extend_heap()
 *
 * DESCRIPTION
 *   allocate size of bytes of memory and returns a pointer to the
 *   allocated memory.
 *
 * RETURN VALUE
 *   Return a pointer to the allocated memory.
 */
void *my_malloc(size_t size)
{
	header_t *header;
	header_t *adequate;
	unsigned long free_size;
	int flag = 0;

	if (size % ALIGNMENT != 0)
	{
		size += ALIGNMENT - size % ALIGNMENT;
	}
	if (!list_empty(&free_list))
	{
		if (g_algo == FIRST_FIT)
		{
			list_for_each_entry(header, &free_list, list)
			{
				if (header->free == true)
				{
					if (header->size == size)
					{
						header->free = false;
						goto success;
					}
					else if (header->size > size)
					{
						header_t *remainder = header + (size + HDRSIZE) / HDRSIZE;
						remainder->free = true;
						remainder->size = header->size - size - HDRSIZE;
						header->free = false;
						header->size = size;
						list_add(&remainder->list, &header->list);
						goto success;
					}
					else if (list_is_last(&header->list, &free_list))
					{
						sbrk(size + HDRSIZE);
						header_t *freeheader = header + (size + HDRSIZE) / HDRSIZE;
						freeheader->free = true;
						freeheader->size = header->size;
						header->free = false;
						header->size = size;
						list_add_tail(&freeheader->list, &free_list);
						goto success;
					}
				}
			}
		}
		if (g_algo == BEST_FIT)
		{
			list_for_each_entry(header, &free_list, list)
			{
				if (header->free == true)
				{
					if (header->size >= size)
					{
						if (free_size > header->size - size || flag == 0)
						{
							adequate = header;
							free_size = header->size - size;
							flag = 1;
						}
					}
				}
			}
			if (flag == 1)
			{
				if (free_size == 0)
				{
					adequate->free = false;
					header = adequate;
					goto success;
				}
				else if (free_size > 0)
				{
					header_t *remainder = adequate + (size + HDRSIZE) / HDRSIZE;
					remainder->free = true;
					remainder->size = adequate->size - size - HDRSIZE;
					adequate->free = false;
					adequate->size = size;
					list_add(&remainder->list, &adequate->list);
					header = adequate;
					goto success;
				}
			}
			header = list_prev_entry(header, list);
			if (header->free == true && list_is_last(&header->list, &free_list))
			{
				sbrk(size + HDRSIZE);
				header_t *freeheader = header + (size + HDRSIZE) / HDRSIZE;
				freeheader->free = true;
				freeheader->size = header->size;
				header->free = false;
				header->size = size;
				list_add_tail(&freeheader->list, &free_list);
				goto success;
			}
		}
	}

	header = sbrk(0);
	sbrk(size + HDRSIZE);
	header->free = false;
	header->size = size;
	INIT_LIST_HEAD(&header->list);
	list_add_tail(&header->list, &free_list);

  /* Implement this function */
	success:
  return header;
}

/***********************************************************************
 * my_realloc()
 *
 * DESCRIPTION
 *   tries to change the size of the allocation pointed to by ptr to
 *   size, and returns ptr. If there is not enough memory block,
 *   my_realloc() creates a new allocation, copies as much of the old
 *   data pointed to by ptr as will fit to the new allocation, frees
 *   the old allocation.
 *
 * RETURN VALUE
 *   Return a pointer to the reallocated memory
 */
void *my_realloc(void *ptr, size_t size)
{
	header_t *header;
	header_t *adequate;
	header_t *temp;
	header_t *freeheader;
	unsigned long free_size;
	int flag = 0;

	if (size % ALIGNMENT != 0)
	{
		size += ALIGNMENT - size % ALIGNMENT;
	}
	temp = ptr;
	if (temp->size == size)
	{
		return temp;
	}
	if (!list_empty(&free_list))
	{
		if (g_algo == FIRST_FIT)
		{
			list_for_each_entry(header, &free_list, list)
			{
				if (header->free == true)
				{
					if (header->size == size)
					{
						header->free = false;
						goto success;
					}
					else if (header->size > size)
					{
						header_t *remainder;
						remainder = header + (size + HDRSIZE) / HDRSIZE;
						remainder->free = true;
						remainder->size = header->size - size - ALIGNMENT;
						header->free = false;
						header->size = size;
						list_add(&remainder->list, &header->list);
						goto success;
					}
					else if (list_is_last(&header->list, &free_list))
					{
						sbrk(size + HDRSIZE);
						header_t *freeheader = header + (size + HDRSIZE) / HDRSIZE;
						freeheader->free = true;
						freeheader->size = header->size;
						header->free = false;
						header->size = size;
						list_add_tail(&freeheader->list, &free_list);
						goto success;
					}
				}
			}
		}
		if (g_algo == BEST_FIT)
		{
			list_for_each_entry(header, &free_list, list)
			{
				if (header->free == true)
				{
					if (header->size >= size)
					{
						if (free_size > header->size - size || flag == 0)
						{
							adequate = header;
							free_size = header->size - size;
							flag = 1;
						}
					}
				}
			}
			if (flag == 1)
			{
				if (free_size == 0)
				{
					adequate->free = false;
					header = adequate;
					goto success;
				}
				else if (free_size > 0)
				{
					header_t *remainder = adequate + (size + HDRSIZE) / HDRSIZE;
					remainder->free = true;
					remainder->size = adequate->size - size - HDRSIZE;
					adequate->free = false;
					adequate->size = size;
					list_add(&remainder->list, &adequate->list);
					header = adequate;
					goto success;
				}
			}
			header = list_prev_entry(header, list);
			if (header->free == true && list_is_last(&header->list, &free_list))
			{
				sbrk(size + HDRSIZE);
				header_t *freeheader = header + (size + HDRSIZE) / HDRSIZE;
				freeheader->free = true;
				freeheader->size = header->size;
				header->free = false;
				header->size = size;
				list_add_tail(&freeheader->list, &free_list);
				goto success;
			}
		}
	}
	header = sbrk(0);
	sbrk(size + HDRSIZE);
	header->free = false;
	header->size = size;
	INIT_LIST_HEAD(&header->list);
	list_add_tail(&header->list, &free_list);

	success:
	
	freeheader = ptr;
	freeheader->free = true;

	temp = list_next_entry(freeheader, list);
	if (temp->free == true)
	{
		freeheader->size += temp->size + HDRSIZE;
		list_del(&temp->list);
	}
	temp = list_prev_entry(freeheader, list);
	if (temp->free == true)
	{
		freeheader = temp;
		temp = list_next_entry(freeheader, list);
		freeheader->size += temp->size + HDRSIZE;
		list_del(&temp->list);
	}
  /* Implement this function */
  return header;
}

/***********************************************************************
 * my_realloc()
 *
 * DESCRIPTION
 *   deallocates the memory allocation pointed to by ptr.
 */
void my_free(void *ptr)
{
	header_t *header = ptr;
	header_t *temp;
	header->free = true;
	
	temp = list_next_entry(header, list);
	if (temp->free == true)
	{
		header->size += temp->size + HDRSIZE;
		list_del(&temp->list);
	}
	temp = list_prev_entry(header, list);
	if (temp->free == true)
	{
		header = temp;
		temp = list_next_entry(header, list);
		header->size += temp->size + HDRSIZE;
		list_del(&temp->list);
	}
  /* Implement this function */
  return;
}

/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */
/*          ****** BUT YOU MAY CALL SOME IF YOU WANT TO.. ******      */
/*          ****** EXCEPT TO mem_init() AND mem_deinit(). ******      */
void mem_init(const algo_t algo)
{
  g_algo = algo;
  bp = sbrk(0);
}

void mem_deinit()
{
  header_t *header;
  size_t size = 0;
  list_for_each_entry(header, &free_list, list) {
    size += HDRSIZE + header->size;
  }
  sbrk(-size);

  if (bp != sbrk(0)) {
    fprintf(stderr, "[Error] There is memory leak\n");
  }
}

void print_memory_layout()
{
  header_t *header;
  int cnt = 0;

  printf("===========================\n");
  list_for_each_entry(header, &free_list, list) {
    cnt++;
    printf("%c %ld\n", (header->free) ? 'F' : 'M', header->size);
  }

  printf("Number of block: %d\n", cnt);
  printf("===========================\n");
  return;
}
