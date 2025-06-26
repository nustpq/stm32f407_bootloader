#ifndef _KFIFO_INC_
#define _KFIFO_INC_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */

typedef struct _kfifo
{
	uint8_t *buffer; // the buffer holding the data
	uint32_t size;   // the size of the allocated buffer
	uint32_t in;	 // data is added at offset (in % size)
	uint32_t out;	// data is extracted from off. (out % size)
} kfifo_t;

void kfifo_init(kfifo_t *fifo, int size);
void kfifo_init_static(kfifo_t *fifo, uint8_t *pBuf, int size);
void kfifo_free(kfifo_t *fifo);

// put data into buffer
uint32_t kfifo_put(kfifo_t *fifo, uint8_t *buffer, uint32_t len);

// take data from buffer
uint32_t kfifo_get(kfifo_t *fifo, uint8_t *buffer, uint32_t len);

//release data memory
uint32_t kfifo_release(kfifo_t *fifo, uint32_t len);

// calculate free space
uint32_t kfifo_get_free_space(kfifo_t *fifo);

uint32_t kfifo_get_data_size(kfifo_t *fifo);
void kfifo_reset(kfifo_t *fifo);

#ifdef __cplusplus
}
#endif

#endif
