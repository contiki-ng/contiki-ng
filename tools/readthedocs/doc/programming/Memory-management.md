# Memory management

Contiki-NG supports both static and dynamic memory allocations. In embedded systems, memory allocations have traditionally restricted to static sizes because static memory is free from leaks and fragmentation. Static memory is nevertheless cumbersome to handle when the memory requirements change during run-time. Such changes may occur in a web server keeping track of connections or a virtual machine supporting dynamic programming languages.
When being restricted to static memory, programmers have to guess the maximum usage of a resource and over-allocate a memory block to be safe from memory exhaustion. To mitigate such issues, we provide two different types of memory allocators in addition static memory: the semi-dynamic _MEMB_ module and the dynamic _HeapMem_ module.

## MEMB: Memory Blocks

The MEMB library, declared in `os/lib/memb.h`, provides a set of memory block management functions.  Memory blocks are allocated as an array of objects of constant size and are placed in static memory. The API is shown below:

| Function                                    | Purpose                                   |
|---------------------------------------------|-------------------------------------------|
|`MEMB(name, structure, num)`                 | Declare a memory block.                   |
|`void memb_init(struct memb *m)`             | Initialize a memory block.                |
|`void *memb_alloc(struct memb *m)`           | Allocate a memory block.                  |
|`char memb_free(struct memb *m, void *ptr)`  | Free a memory block.                      |
|`int memb_inmemb(struct memb *m, void *ptr)` | Check if an address is in a memory block. |

The `MEMB()` macro declares a memory block, which has the type `struct memb`. Since the block is put into static memory, it is typically placed at the top of a C source file that uses memory blocks. `name` identifies the memory block, and is later used as an argument to the other memory block functions. The `structure`parameter specifies the C type of the memory block, `num` represent the amount objects that the block accommodates. The definition of `struct memb` is a follows:

```c
struct memb {
  unsigned short size;
  unsigned short num;
  char *count;
  void *mem;
};
```

The expansion of the `MEMB` macro yields three statements that define static memory. One statement stores the amount of object that the memory block can hold. Since the amount is stored in a variable of type `unsigned short`, the memory block can hold at most `USHRT_MAX` objects. The second statement allocates an array of `num` structures of the type referenced to by the `structure` parameter.

Once the memory block has been declared by using `MEMB()`, it has to be initialized by calling `memb_init()`. This function takes a parameter of `struct memb`, identifying the memory block.

After initializing a `struct memb`, we are ready to start allocating objects from it by using `memb_alloc()`. All objects allocated through the same `struct memb` have the same size,  which is determined by the size of the `structure` argument to `MEMB()`. `memb_alloc()` returns a pointer to the allocated object if the operation was  successful, or `NULL` if the memory block has no free object.

`memb_free()` deallocates an object that has previously been allocated by using `memb_alloc()`. Two arguments are needed to free the object: `m` points to the memory block, whereas `ptr` points to the object within the memory block.

Any pointer can be checked to determine whether it is within the data area of a memory block. `memb_inmemb()` returns 1 if `ptr` is inside the memory block `m`, and 0 if it points to unknown memory.

We show an example of how the MEMB module can be used. The `open_connection()` function allocates a new `struct 
connection` variable for each new connection identified by `socket`. When a connection is closed, we free the memory block for the `struct connection` variable.

```c
#include "contiki.h"
#include "lib/memb.h"

struct connection {
  int socket;
};
MEMB(connections, struct connection, 16);

struct connection *
open_connection(int socket)
{
  struct connection *conn;

  conn = memb_alloc(&connections);
  if(conn == NULL) {
    return NULL;
  }
  conn->socket = socket;
  return conn;
}

void
close_connection(struct connection *conn)
{
  memb_free(&connections, conn);
}
```

## Heap Memory (HeapMem)

The standard C library provides a set of functions for allocating and freeing memory in the heap memory space. For different compiler toolchains, it is unclear how well the default heap memory module will perform in a resource-constrained execution environment. Allocation and deallocation patterns on objects of varying sizes may more be problematic in some malloc implementations. For this reason, Contiki-NG includes a heap memory module that has been used on a variety of hardware platforms and with different applications. The HeapMem module has an API that is similar to that of standard C. To avoid name collisions, the function names in HeapMem are `heapmem_alloc()`, `heapmem_realloc()`, and `heapmem_free()` instead of `malloc()`, `realloc()`, and `free()`. The API is shown in the table below.

| Function                                       | Purpose                                 |
|------------------------------------------------|-----------------------------------------|
|`void *heapmem_alloc(size_t size)`              | Allocate uninitialized memory.          |
|`void *heapmem_realloc(void *ptr, size_t size)` | Change the size of an allocated object. |
|`void heapmem_free(void *ptr)`                  | Free memory.                            |

All functions listed  in this section are declared in the C header `os/lib/heapmem.h`. The `heapmem_alloc()` function allocates `size` bytes of memory on the heap. If the memory was successfully allocated, `heapmem_alloc()` returns a 
pointer to it. If there was not enough contiguous free memory, `heapmem_alloc()` returns `NULL`.

The `heamem_realloc()` function reallocates a previously allocated block, `ptr`, with a new `size`. If the new block is smaller, `size` bytes of the data in the old block is copied into the new block. If the new block is larger, the complete old block is copied, and the rest of the new block contains unspecified data. Once the new block has been allocated, and its contents has been filled in, the old block is deallocated. `heapmem_realloc()` returns NULL if the block could not be allocated. If the reallocation succeeded, `heapmem_realloc()` returns a pointer to the new block.

`heapmem_free()` deallocates a block that was previously allocated through `heapmem_alloc()` or `heapmem_realloc()`. The argument `ptr` must point to the start of an allocated block.
