#ifndef _ringbuffer_h_
#define _ringbuffer_h_

#include <stdint.h>
#include <string.h>

typedef struct ringbuffer_s {
        uint8_t *buf;
        size_t  size;
        size_t  start;
        size_t  avail;
} ringbuffer;

/* Initialize a ringbuffer on the given flat array. */
void rb_init(ringbuffer* rb, uint8_t *buffer, size_t len);

/* Pop n bytes from the ringbuffer into the flat array.
   Returns the number of bytes retrieved.
*/
size_t rb_get(ringbuffer *rb, uint8_t *buffer, size_t n);

/* Peek n bytes of the ringbuffer into the flat array.
   Returns the number of bytes retrieved.
*/
size_t rb_peek(const ringbuffer *rb, uint8_t *buffer, size_t n);

/* Push n bytes from the flat array into the ringbuffer.
   Returns 0 if there is insufficient space in the ringbuffer;
   otherwise returns the number of bytes written.
*/
size_t rb_put(ringbuffer *rb, const uint8_t *buffer, size_t n);

/* Remove n bytes from the ringbuffer.
   Returns 0 if there are less than n bytes available.
*/
size_t rb_del(ringbuffer *rb, size_t n);

/* Return the amount of free space in the ringbuffer */
size_t rb_free(const ringbuffer *rb);

#endif
