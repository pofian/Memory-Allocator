// Copyright Filip Popa ~ ACS 313CAb

// The library with the functions used in sfl.c
//      which perform generic operations on lists
typedef struct node node;
typedef struct list list;

struct node {
    int size;       // The size of the memory block
    size_t address; // The starting address of the block
    void *data;
    node *prev, *next;
};

struct list {
    int data_size; // The size of each block in the list
    node *first;
};

// This function allocates memory for a new block and returns it
node *new_node(size_t start_address, int data_size)
{
    node *p = malloc(sizeof(node));
    p->address = start_address;
    p->size = data_size;
    p->prev = NULL;
    p->next = NULL;
    p->data = NULL;
    return p;
}

// This function creates a list of nr_nodes memory blocks, whose addresses
//      start from start_address. It is called in init_heap()
list *new_list(size_t start_address, int nr_nodes, int data_size)
{
    list *x = malloc(sizeof(list));
    x->data_size = data_size;
    x->first = new_node(start_address, data_size);
    node *p = x->first;
    for (int i = 1; i < nr_nodes; i++) {
        node *q = new_node(p->address + data_size, data_size);
        q->prev = p;
        p->next = q;
        p = q;
    }
    return x;
}

// This function adds a node to a list in such a way
//      that it is always sorted in ascending order by the address of each block.
void add_to_list_in_order(list *x, node *p)
{
    p->next = NULL;
    p->prev = NULL;
    if (!x->first) {
        x->first = p;
        return;
    }
    node *q = x->first;
    if (p->address < q->address) {
        p->next = q;
        q->prev = p;
        x->first = p;
        return;
    }
    while (p->address > q->address && q->next)
        q = q->next;
    if (!q->next && p->address > q->address) {
        q->next = p;
        p->prev = q;
        return;
    }
    p->prev = q->prev;
    p->next = q;
    q->prev->next = p;
    q->prev = p;
}

// The length of a list
int size_of_list(list *x)
{
    if (!x)
        return 0;
    int nr = 0;
    node *p = x->first;
    while (p) {
        p = p->next;
        nr++;
    }
    return nr;
}

// Free the memory allocated to a list
void free_list(list *x, int free_data)
{
    if (!x)
        return;
    node *p = x->first, *q;
    if (free_data)
        while (p) {
            q = p->next;
            free(p->data);
            free(p);
            p = q;
        }
    else
        while (p) {
            q = p->next;
            free(p);
            p = q;
        }
    free(x);
}

// Display a list. The type_of_print parameter differentiates
//      between the lists in stl and the allocated_memory list,
//      which must be printed differently, as required by the DUMP_MEMORY model
void print_list(list *x, int type_of_print)
{
    if (!x)
        return;
    node *p = x->first;
    if (type_of_print == 0)
        while (p) {
            printf(" 0x%lx", p->address);
            p = p->next;
        }
    else
        while (p) {
            printf(" (0x%lx - %d)", p->address, p->size);
            p = p->next;
        }
    printf("\n");
}

// This function removes a node from a list that it is already known to belong to
void remove_node_from_list(list *x, node *p)
{
    if (!x || !x->first)
        return;
    if (p == x->first)
        x->first = p->next;
    if (p->prev)
        p->prev->next = p->next;
    if (p->next)
        p->next->prev = p->prev;
    p->prev = NULL;
    p->next = NULL;
}

