#ifndef _heap_h_
#define _heap_h_

/* Forward declaration */
struct heap_node_s;

/* Comparator function */
typedef int (*heap_comparator)(unsigned a, unsigned b);

/* Heap object */
typedef struct {
        struct heap_node_s *nodes;
        heap_comparator    comp;
        unsigned           size;
        unsigned           count;
} heap;

/* Element of heap */
typedef struct heap_node_s {
        unsigned key;
        void     *value;
} heap_node;

/* Initializer */
void heap_create(heap *h, heap_node *buffer, unsigned size, heap_comparator comp);

/* Elementary operations */
#define HEAP_INDEX(n, h)     ((n - h->nodes)/sizeof(heap_node))
#define HEAP_PARENT_INDEX(i) (i/2)
#define HEAP_LEFT_INDEX(i)   (2*i)
#define HEAP_RIGHT_INDEX(i)  (2*i+1)

/* Heap operations */
heap_node *heap_insert(heap *h, unsigned key, void *value);
unsigned heap_pop(heap *h, heap_node *d);
unsigned heap_peek(heap *h, heap_node *d);

#endif
