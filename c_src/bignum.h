#include <stdint.h>

typedef struct bignum {
  uint8_t *b;
  uint32_t size;
} bignum;

bignum * bignum_alloc(int size);
void bignum_clear(bignum *b);
void bignum_print(bignum *b);
void bignum_print_bits(bignum *b);
void bignum_free(bignum *b);
void bignum_xor(bignum *b, int x);
void bignum_or(bignum *b, int x);
void bignum_set_bit(bignum *b, int pos);
void bignum_shift_left(bignum *b, int n);
uint64_t bignum_to_uint64(bignum *b);

