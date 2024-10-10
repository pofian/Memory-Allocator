// Copyright Filip Popa ~ ACS 313CAb 2024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

// Structure to store the necessary information for DUMP_MEMORY
typedef struct {
    int heap_size, allocated_bytes, free_bytes, free_blocks;
    int nr_allocated_blocks, nr_malloc_calls;
    int nr_fragmentations, nr_free_calls, bytes_per_list;
} info_about_sfl;

// sfl structure: stores the starting address of the heap, the lists and
//      their number, and also the number of allocated ones (for REALLOC)
// It also stores type_of_free (reconstruction type for FREE)
//      and the necessary information for DUMP_MEMORY in the info variable
typedef struct {
    int start_address;
    int nr_lists, nr_alloced_lists;
    int type_of_free;
    list **lists;
    info_about_sfl *info;
} sfl;

// Function called when INIT_HEAP is used
sfl *init_heap(int start_address, int nr_lists, int bytes_per_list, int type)
{
    sfl *x = malloc(sizeof(sfl));

    // Set the variables x->start_address and x->nr_lists,
    //      then allocate memory for x->lists
    x->start_address = start_address;
    x->nr_lists = nr_lists;
    x->nr_alloced_lists = nr_lists;
    x->lists = malloc(nr_lists * sizeof(list));
    x->type_of_free = type;

    // Initialize sfl information
    x->info = calloc(1, sizeof(info_about_sfl));
    x->info->heap_size = bytes_per_list * nr_lists;
    x->info->free_bytes = x->info->heap_size;
    x->info->bytes_per_list = bytes_per_list;

    // Create the lists whose block sizes are p (8, 16, 32, ...)
    // Each list will have bytes_per_list / p blocks.
    for (int i = 0, p = 8; i < nr_lists; i++, p *= 2) {
        x->info->free_blocks += bytes_per_list / p;
        x->lists[i] = new_list(start_address, bytes_per_list / p, p);
        start_address += bytes_per_list;
    }
    return x;
}

// Free the memory allocated to sfl
void free_sfl(sfl *x)
{
    if (!x)
        return;
    free(x->info);
    for (int i = 0; i < x->nr_lists; i++)
        free_list(x->lists[i], 0);
    free(x->lists);
    free(x);
}

// Function to delete a list from sfl (when it no longer contains blocks),
//      then shift the remaining lists to fill the gap
void remove_list_from_sfl(sfl *x, int position)
{
    free(x->lists[position]);
    for (int i = position; i < x->nr_lists - 1; i++)
        x->lists[i] = x->lists[i + 1];
    x->lists[x->nr_lists - 1] = NULL;
    x->nr_lists--;
}

// Function to calculate the index of the first list whose blocks
//      are larger than or equal to nr_bytes
// If nr_bytes > the size of any block,
//      x->nr_lists will be returned
int position_in_sfl(sfl *x, int nr_bytes)
{
    int i = 0;
    while (i < x->nr_lists && x->lists[i]->data_size < nr_bytes)
        i++;
    return i;
}

// Function to add a block of new size to sfl
void add_block_of_new_size_to_stl(sfl *x, node *p)
{
    x->info->free_blocks++;
    int i = position_in_sfl(x, p->size);

    // If there already exists a block with the same size as the block
    //      being added, it should be inserted into the corresponding list
    if (i != x->nr_lists && x->lists[i]->data_size == p->size) {
        add_to_list_in_order(x->lists[i], p);
        return;
    }
    // If this point is reached, the size of the block being added is
    //      different from all the block sizes in sfl, so a new list
    //      must be created for its size

    // Allocate memory for the new list, if needed
    if (x->nr_lists >= x->nr_alloced_lists) {
        x->nr_alloced_lists *= 2;
        x->lists = realloc(x->lists, x->nr_alloced_lists * sizeof(list *));
    }

    // Shift the remaining lists to the right to make room for the new one
    for (int j = x->nr_lists; j > i; j--)
        x->lists[j] = x->lists[j - 1];

    x->lists[i] = malloc(sizeof(list));
    x->lists[i]->data_size = p->size;
    x->lists[i]->first = p;

    x->nr_lists++;
}

// Function for the MALLOC command
void malloc_sfl(sfl *x, int nr_bytes, list *allocated_memory)
{
    int i = position_in_sfl(x, nr_bytes);
    if (i == x->nr_lists) {
        printf("Out of memory\n");
        return;
    }

    // Update sfl information
    x->info->allocated_bytes += nr_bytes;
    x->info->free_bytes -= nr_bytes;
    x->info->nr_allocated_blocks++;
    x->info->nr_malloc_calls++;
    x->info->free_blocks--;

    // First block in the heap whose size
    //      is larger than nr_bytes
    node *p = x->lists[i]->first;

    // Add a new block to allocated_memory
    node *q = new_node(p->address, nr_bytes);
    q->data = calloc(q->size + 1, sizeof(char));
    memcpy(q->data + q->size, "\0", sizeof(char));
    add_to_list_in_order(allocated_memory, q);

    // Store the size of the block being allocated
    int data_size = x->lists[i]->data_size;

    // Remove block p from the heap, and if p->next is NULL,
    //      it is the last block of its size, so the list
    //      it belongs to must be removed
    if (!p->next) {
        remove_list_from_sfl(x, i);
    } else {
        x->lists[i]->first = p->next;
        p->next->prev = NULL;
    }

    // Allocate without fragmentation
    if (nr_bytes == data_size) {
        free(p);
        return;
    }

    // Allocate with fragmentation, so update the size and address
    //      of the remaining block p, and add it back to the heap
    p->size -= nr_bytes;
    p->address += nr_bytes;
    p->next = NULL;
    p->prev = NULL;

    add_block_of_new_size_to_stl(x, p);
    x->info->nr_fragmentations++;
}

// Function to return the node from a given address in allocated memory
//      or NULL if no such node exists
node *remove_from_list(list *allocated_memory, size_t address)
{
    if (!allocated_memory || !allocated_memory->first)
        return NULL;
    node *p = allocated_memory->first;
    while (p && p->address != address)
        p = p->next;
    if (!p)
        return NULL;
    if (p->address == address) {
        remove_node_from_list(allocated_memory, p);
        return p;
    }
    return NULL;
}

typedef struct {
    int first, second;
} pair;

// Function to get the origin of a block fragment by calculating
//      the index of the list it belongs to and the index of the block
//      within the parent list.
// This helps in determining if two block fragments
//      can be merged by checking if their addresses share the same origin
void get_origin(pair *m, size_t address, sfl *x)
{
    m->first = (address - x->start_address) / x->info->bytes_per_list;
    m->second = (address - x->start_address) % x->info->bytes_per_list;
    m->second = m->second >> (3 + m->first);
}

// Function checks if there is any block fragment in the heap
//      that can be merged with another given fragment
// Also merges them if applicable
void try_to_tape(sfl *x, node *p)
{
    pair *m, *n;
    m = malloc(sizeof(node));
    n = malloc(sizeof(node));
    get_origin(m, p->address, x);

    // Traverse the heap twice, the first time to merge to the left
	//      and the second time to the right
    // I want to mention that this was not necessary; I could have implemented
    //       the function such that merging to the left and right
	//       could be done simultaneously
    for (int left_right = 0; left_right <= 1; left_right++)
        for (int i = 0; i < x->nr_lists; i++) {
            node *q = x->lists[i]->first;
            while (q) {
                // The variable ok checks if merging can occur,
				//       so it is initialized to 0
                int ok = 0;
                get_origin(n, q->address, x);

                if (m->first == n->first && m->second == n->second) {
                    // If m and n match, blocks p and q are
					//       fragments of the same block
                    if (left_right == 0 && q->address + q->size == p->address)
                        ok = 1;
                    if (left_right == 1 && p->address + p->size == q->address)
                        ok = 1;
                }
                if (!ok) {
                    q = q->next;
                    continue;
                }

                // If I reach here, ok=1 so I perform the merge
                x->info->free_blocks--;

                // Update block p, and if the merge is to the left
                //      I must also modify the addresses of the blocks
                p->size += q->size;
                if (left_right == 0) {
                    p->address = q->address;
                    q->address = p->address + q->size;
                }

                // Since I'm removing q, I need to save
                //      the next block that will be processed
                node *aux = q->next;

                // Remove block q from the heap
                if (q->prev)
                    q->prev->next = q->next;
                if (q->next)
                    q->next->prev = q->prev;

                if (x->lists[i]->first == q) {
                    x->lists[i]->first = q->next;

                    // If the list contains only block q,
                    //      then I remove it
                    if (!q->next) {
                        remove_list_from_sfl(x, i);
                        i--;
                    }
                }

                free(q);
                q = aux;
            }
        }
    free(m);
    free(n);
}

// Function corresponding to the FREE command
void free_from_memory(sfl *x, list *allocated_memory, size_t address)
{
    node *p = remove_from_list(allocated_memory, address);
    if (!p) {
        printf("Invalid free\n");
        return;
    }
    free(p->data);

    // Update sfl information
    x->info->nr_free_calls++;
    x->info->nr_allocated_blocks--;
    x->info->free_bytes += p->size;
    x->info->allocated_bytes -= p->size;

    // Merge block fragments if applicable
    if (x->type_of_free == 1)
        try_to_tape(x, p);

    // Add the deallocated block back to the heap
    add_block_of_new_size_to_stl(x, p);
}

// Function corresponding to the DUMP_MEMORY command
void dump_memory(sfl *x, list *allocated_memory)
{
    printf("+++++DUMP+++++\n");
    printf("Total memory: %d bytes\n", x->info->heap_size);
    printf("Total allocated memory: %d bytes\n", x->info->allocated_bytes);
    printf("Total free memory: %d bytes\n", x->info->free_bytes);
    printf("Free blocks: %d\n", x->info->free_blocks);
    printf("Number of allocated blocks: %d\n", x->info->nr_allocated_blocks);
    printf("Number of malloc calls: %d\n", x->info->nr_malloc_calls);
    printf("Number of fragmentations: %d\n", x->info->nr_fragmentations);
    printf("Number of free calls: %d\n", x->info->nr_free_calls);

    // Print the contents of the heap
    for (int i = 0; i < x->nr_lists; i++) {
        printf("Blocks with %d bytes - %d free block(s) :",
               x->lists[i]->data_size, size_of_list(x->lists[i]));
        print_list(x->lists[i], 0);
    }

    printf("Allocated blocks :");
    print_list(allocated_memory, 1);
    printf("-----DUMP-----\n");
}

// Function corresponding to the WRITE command. Returns -1 if I get
//      "segmentation fault" or 0 if the operation was successful
int write_sfl(list *allocated_memory, size_t address)
{
    if (!allocated_memory)
        return -1;

    // Read the input, which I will split with strtok
    const int max_input_length = 650;
    char input[max_input_length];
    fgets(input, max_input_length, stdin);

    // Split the input to build data
    //      and calculate nr_bytes
    strtok(input, "\n");
    char *data = strtok(input, "\"");
    data = strtok(NULL, "\"");
    size_t nr_bytes = atoi(strtok(NULL, "\""));

    // Write at most all data
    if (nr_bytes > strlen(data))
        nr_bytes = strlen(data);

    // Search for the block that contains the given address
    node *p = allocated_memory->first;
    while (p && p->address < address)
        p = p->next;

    size_t start_address = address;
    do {
        // Invalid write, so return -1
        if (!p || p->address > address)
            return -1;

        // The difference between the current address and the address
        //      where the block from which I'm displaying starts
        size_t add_address = address - p->address;

        // s is the number of characters that need to be
        //      written (at most nr_bytes)
        size_t s = p->size - add_address;
        if (s > nr_bytes)
            s = nr_bytes;

        // Write s characters, starting from the address
        //      (which is why I add add_address)
        int write_address = address - start_address;
        memcpy(p->data + add_address, data + write_address, s * sizeof(char));

        // Move to the next block
        nr_bytes -= s;
        address += s;
        p = p->next;

    } while (nr_bytes);
    // The loop ends when nr_bytes=0, meaning I've read everything

    // If the loop ends, the writing was valid and return 0
    return 0;
}

// Function corresponding to the READ command. Returns -1 if I get
//      "segmentation fault" or 0 if the operation was successful
int read_sfl(list *allocated_memory)
{
    // I need the save_ variables which I use in the for loop because
    //      the address, nr_bytes, and p variables change after
    //      the first traversal and I must not lose their values
    size_t address, nr_bytes;
    size_t save_address, save_nr_bytes;
    scanf("%lx%lu", &save_address, &save_nr_bytes);

    if (!allocated_memory)
        return -1;

    // Search for the block that contains the given address
    node *save_p = allocated_memory->first;
    while (save_p && save_p->address + save_p->size < save_address)
        save_p = save_p->next;
    node *p = save_p;

    // The role of the for loop is to simulate the display to check if it is
    //       valid or not, and if it is not valid, nothing should be displayed
    for (int print = 0; print <= 1; print++) {
        address = save_address;
        nr_bytes = save_nr_bytes;
        p = save_p;
        do {
            // Invalid display, so return -1
            if (!p || p->address > address)
                return -1;

            // The difference between the current address and the address
            //      where the block I'm displaying starts
            size_t add_address = address - p->address;

            // s is the number of characters that need to be
            //      printed (at most nr_bytes)
            size_t s = p->size - add_address;
            if (s > nr_bytes)
                s = nr_bytes;

            // Print s characters, starting from the address
            //      (hence starting from add_address)
            char *c = (char *)(p->data);
            if (print)
                for (size_t i = add_address; i < s + add_address; i++)
                    printf("%c", *(c + i));

            // Move to the next block
            nr_bytes -= s;
            address += s;
            p = p->next;

        } while (nr_bytes);
        // The loop ends when nr_bytes=0, meaning I've printed everything
        // If the loop ends, printing was valid
    }

	// Printing was valid, so return 0
    printf("\n");
    return 0;
}

int main(void)
{
	const int max_command_length = 100;
	char *command = malloc(max_command_length * sizeof(char));
	sfl *x;

	// Store allocated memory in a list
	list *allocated_memory = malloc(sizeof(list));
	allocated_memory->first = NULL;
	allocated_memory->data_size = 0;

	while (1) {
		scanf("%s", command);
		if (!strcmp(command, "INIT_HEAP")) {
			size_t address;
			int nr_lists, bytes_per_list, type;
			scanf("%lx%d%d%d", &address, &nr_lists, &bytes_per_list, &type);
			x = init_heap(address, nr_lists, bytes_per_list, type);
		} else if (!strcmp(command, "MALLOC")) {
			int nr_bytes;
			scanf("%d", &nr_bytes);
			malloc_sfl(x, nr_bytes, allocated_memory);
		} else if (!strcmp(command, "FREE")) {
			size_t address;
			scanf("%lx", &address);
			free_from_memory(x, allocated_memory, address);
		} else if (!strcmp(command, "READ")) {
			if (read_sfl(allocated_memory)) {
				printf("Segmentation fault (core dumped)\n");
				dump_memory(x, allocated_memory);
				break;
			}
		} else if (!strcmp(command, "WRITE")) {
			size_t address;
			scanf("%lx", &address);
			if (write_sfl(allocated_memory, address)) {
				printf("Segmentation fault (core dumped)\n");
				dump_memory(x, allocated_memory);
				break;
			}
		} else if (!strcmp(command, "DUMP_MEMORY")) {
			dump_memory(x, allocated_memory);
		} else if (!strcmp(command, "DESTROY_HEAP")) {
			break;
		}
	}

	// Free auxiliary memory
	free(command);
	free_list(allocated_memory, 1);
	free_sfl(x);
}
