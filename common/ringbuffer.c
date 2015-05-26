#include "ringbuffer.h"

ringbuffer rb_init(uint8_t *buffer, size_t len){
        ringbuffer rb = {
                .buf   = buffer,
                .size  = len,
                .start = 0,
                .avail = 0
        };
        return rb;
}

size_t rb_get(ringbuffer *rb, uint8_t *buffer, size_t n){
        if (n > rb->avail) {
                return 0;
        }
        else{
                size_t i;
                for(i = 0; i < n; i++){
                        buffer[i] = rb->buf[rb->start];
                        rb->start = (rb->start + 1) % rb->size;
                        rb->avail--;
                }
                return n;
        }
}

size_t rb_peek(const ringbuffer *rb, uint8_t *buffer, size_t n){
        if (n > rb->avail) {
                return 0;
        }
        else{
                size_t i;
                for(i = 0; i < n; i++){
                        buffer[i] = rb->buf[(rb->start + i) % rb->size];
                }
                return n;
        }
}

size_t rb_put(ringbuffer *rb, const uint8_t *buffer, size_t n){
        if(rb->size - rb->avail < n){
                return 0;
        }
        else{
                size_t i;
                for(i = 0; i < n; i++){
                        rb->buf[(rb->start + rb->avail) % rb->size] = buffer[i];
                        rb->avail++;
                }     
                return i;
        }
}
