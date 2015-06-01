#include "heap.h"
#include <string.h>

void heap_create(heap *h, heap_node *buffer, unsigned size, heap_comparator comp){
        h->count = 0;
        h->nodes = buffer;
        h->comp  = comp;
        h->size  = size;
        memset(buffer, 0, sizeof(heap_node) * size);
}

heap_node *heap_insert(heap *h, unsigned key, void *value){
        unsigned i;
        heap_node new_node;
        new_node.key       = key;
        new_node.value     = value;
        if(h->count < h->size){
                i = h->count++;
                while(i > 0 && h->comp(h->nodes[HEAP_PARENT_INDEX(i)].key, key) > 0){
                        memcpy(&h->nodes[i], &h->nodes[HEAP_PARENT_INDEX(i)], sizeof(heap_node));
                        i = HEAP_PARENT_INDEX(i);
                }
                memcpy(&h->nodes[i], &new_node, sizeof(heap_node));
                return &h->nodes[i];
        }
        return 0;
}

void heap_reheapify(heap *h, unsigned i){
        unsigned j = i;
        heap_node tmp;
        
        if(HEAP_LEFT_INDEX(i) < h->count && h->comp(h->nodes[HEAP_LEFT_INDEX(i)].key, h->nodes[i].key) < 0){
                j = HEAP_LEFT_INDEX(i);
        }
        
        if(HEAP_RIGHT_INDEX(i) < h->count && h->comp(h->nodes[HEAP_RIGHT_INDEX(i)].key, h->nodes[j].key) < 0){
                j = HEAP_RIGHT_INDEX(i);
        }
        
        if(i != j){
                memcpy(&tmp, &h->nodes[i], sizeof(heap_node));
                memcpy(&h->nodes[i], &h->nodes[j], sizeof(heap_node));
                memcpy(&h->nodes[j], &tmp, sizeof(heap_node));
                heap_reheapify(h, j);
        }
}

unsigned heap_pop(heap *h, heap_node *d){
        if(h->count > 0){
                memcpy(d, &h->nodes[0], sizeof(heap_node));
                if(h->count-- > 1){
                        memcpy(&h->nodes[0], &h->nodes[h->count], sizeof(heap_node));
                }
                memset(&h->nodes[h->count], 0, sizeof(heap_node));
                heap_reheapify(h, 0);
                return d->key;
        }
        return 0;
}

unsigned heap_peek(heap *h, heap_node *d){
        if(h->count > 0){
                memcpy(d, &h->nodes[0], sizeof(heap_node));
                return d->key;
        }
        return 0;
}
