**Popa Filip-Andrei**  
**313CAb 2024**

## Segregated Free Lists

### Please read *project_description.pdf* for detailed explications, as well as an overview of the commands.

### Description:

* This project is a virtual memory allocator: in a structure of type SFL, the "unallocated" memory blocks are stored, while the "allocated" blocks are stored in a doubly linked list called `allocated_memory`.
* Each list in the SFL variable, as well as the `allocated_memory` list, has been implemented so that any two nodes (memory blocks) belonging to the same list have the same size. Furthermore, lists are always added/removed in such a way that they remain sorted in ascending order by the size of the blocks they contain. This makes it easy to find the block with a specific minimum size and the smallest address, as required by the MALLOC command.
* The `free_from_memory()` function checks if the allocated_memory contains the block with the given address; if it does, it returns it to the heap by calling `add_block_of_new_size_to_stl()`, which is also used by `malloc_sfl()`.
* The `try_to_tape()` function merges blocks in the case where `type_reconstruction=1`. It traverses all blocks in the heap and checks if any of them match (i.e., originate from the same initial block) with the current block. The check is done by `get_origin()`, based on the observation that it is sufficient to know the address of a block fragment to identify which block it comes from and its position in the heap.
* The `read_sfl()` and `write_sfl()` functions traverse the `allocated_memory` list, and due to its construction being sorted by the addresses of the blocks, it is easy to verify if all the bytes I want to access have been previously allocated.

### Comments on the project:

* I believe the following optimizations were possible:
    1. I could have implemented the `position_in_sfl()` function using binary search.
    2. There was no need for the `try_to_tape()` function to traverse the heap twice; I could have merged to the left and right simultaneously.
    3. I think I could have implemented `read_sfl()` in such a way that I do not simulate the addition, by memorizing everything that needs to be displayed and then only displaying it at the end if necessary.
* I believe I could have modularized `read_sfl()` and `write_sfl()` better, as the code for the two functions is almost identical.

* It took me some time to put comments/explanations in the program, so I hope you, the one reading this, enjoyed it. :)