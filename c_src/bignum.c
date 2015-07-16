// Hacked together "bignum" implementation
#include <stdint.h>
#include "bignum.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "minunit.h"


bignum *bignum_alloc(int size) {
  if (size < 4)
    size = 4;
  uint8_t *b = malloc(size);
  bignum *big = (bignum *) malloc(size);
  big->b = b;
  big->size = size;
  bignum_clear(big);
  return big;
}

void bignum_free(bignum *big) {
  free(big->b);
  free(big);
}

void bignum_clear(bignum *big) {
  memset(big->b, 0, big->size);
}

void bignum_shift_left(bignum *b, int n) {
  unsigned char prev_overflow = 0;
  unsigned char next_overflow = 0;
  for(int i = 0; i <= b->size-1; i++) {
    next_overflow = b->b[i] >> (8-n);
    b->b[i] = b->b[i] << n;
    b->b[i] = b->b[i] | prev_overflow;
    prev_overflow = next_overflow;
  }
}


// XOR with the least significant 32 bits
void bignum_xor(bignum *b, int x) {
  int i = b->size - 4;
  uint32_t *bytes = (uint32_t *)(b->b+i);
  *bytes = *bytes ^ x;
}

// OR with the most significant 4 bytes
void bignum_or(bignum *b, int x) {
  /* int i = b->size - 4; */
  /* uint32_t *bytes = (uint32_t *)(b->b+i); */

  uint32_t *bytes = (uint32_t *)b->b;
  *bytes = *bytes | x;
}

void bignum_set_bit(bignum *b, int pos) {
  int byte_offset = pos / 8;
  int bit_offset = pos;
  if(pos >= 8)
    bit_offset = pos % 8;

  uint8_t *byte = b->b+byte_offset;
  uint8_t mask = 1 << bit_offset;
  /* printf("byte offset %d, bit offset %d, mask %u\n", */
  /*        byte_offset, bit_offset, mask); */
  *byte = *byte | mask;
}



void bignum_print(bignum *b) {
  for(int offset = b->size-1; offset >= 0; offset--) {
    uint8_t *byte = b->b + offset;
    printf("%03d ", *byte);
  }
  printf("\n");
}

void bignum_print_bits(bignum *b) {
  // Byte order: high -> low
  for(int byte = b->size-1; byte >= 0; byte--) {
    printf("%2d ", byte);
    for(int bit = 7; bit >= 0; bit--) {
      unsigned char power = 1 << bit;
      unsigned char *byte_ptr = b->b+byte;
      printf("%d", (*byte_ptr & power) >> bit);
    }
    printf("\n");
  }
}

uint64_t bignum_to_uint64(bignum *b) {
  return *(uint64_t *)b->b;
}
