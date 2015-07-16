// https://github.com/cne1x/sfseize/blob/master/src/main/scala/org/eichelberger/sfc/CompactHilbertCurve.scala

#include "erl_nif.h"
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include "bignum.h"
#include "minunit.h"


void print_bytes(uint64_t *i) {
  uint8_t *u = (uint8_t *) i;
  for(int offset = 7; offset >= 0; offset--) {
    uint8_t *byte = u + offset;
    printf("%03d ", *byte);
  }
  printf("\n");
}



//
// Compact Hilbert Index
//
// "Compact Hilbert Indices for Multi-Dimensional Data"
// https://web.cs.dal.ca/~arc/publications/2-43/paper.pdf

int sum(int a[], int s) {
  int r = 0;
  for(int i = 0; i < s; i++) {
    r += a[i];
  }
  return r;
}

int max(int a[], int s) {
  int m = a[0];
  for(int i = 1; i < s; i++) {
    if(a[i] > m)
      m = a[i];
  }
  return m;
}


uint64_t extract_mask(int n, int ms[], int i, int d) {
  uint64_t mu = 0;
  for(int j = n - 1; j >= 0; j--) {
    mu = mu << 1;
    if(ms[(j + d) % n] > i)
      mu = mu | 1;
  }
  return mu;
}


int bit_at_char(unsigned char *b, int i) {
  unsigned char bit_offset = i;
  if(i >= 8)
    bit_offset = i % 8;

  unsigned char power = 1 << bit_offset;
  int offset = i / 8;
  unsigned char *byte = b+offset;
  return (*byte & power) >> bit_offset;
}

void print_bits(unsigned char *b, int byte_size) {
  for(int byte = 0; byte <= byte_size-1; byte++) {
    printf("%2d ", byte);
    for(int bit = 7; bit >= 0; bit--) {
      unsigned char power = 1 << bit;
      unsigned char *byte_ptr = b+byte;
      printf("%d", (*byte_ptr & power) >> bit);
    }
    printf("\n");
  }
}


int bit_at_int(unsigned int b, int i) {
  int power = 1 << i;
  return (b & power) >> i;
}

unsigned int rotl(unsigned int value, int shift) {
  return (value << shift) | (value >> (sizeof(value) * CHAR_BIT - shift));
}

unsigned int rotr(unsigned int value, int shift) {
  return (value >> shift) | (value << (sizeof(value) * CHAR_BIT - shift));
}

unsigned int gc(unsigned int i) {
  return i ^ (i >> 1);
}
unsigned int inverse_gc(unsigned int g) {
  /* int p = n; */
  /* while (n >>= 1) p ^= n; */
  /* return p; */

  int m = sizeof(g)*8;
  unsigned int i = g;
  int j = 1;
  while(j < m) {
    i = i ^ (g >> j);
    j++;
  }

  return i;
}

unsigned int gcr(int n, int mu, unsigned int w) {
  unsigned int r = 0;
  int j = n-1;
  while(j >= 0) {
    if(bit_at_int(mu, j))
      r = (r << 1) | bit_at_int(w, j);
    j--;
  }
  return r;
}

unsigned int parity(unsigned int mu) {
  // TODO: Improve, maybe use popcount cpu instruction
  // http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
  int s = 0;
  for(int i = 0; i <= 32-1; i++)
    if(bit_at_int(mu, i))
      s += 1;

  return s;
}

unsigned int entry(unsigned int i) {
  if(i == 0) {
    return 0;
  } else {
    return gc(2 * (unsigned int) floor((i-1) / 2));
  }
}

int tsb(unsigned int i) {
  int c = 0;
  for (int j = 0; j < 32; j++) {
    if(bit_at_int(i, j))
      c++;
  }
  return c;
}

int g(unsigned int i) {
  return tsb(i);
}


unsigned int direction(unsigned int i, int n) {
  if (i == 0)
    return 0;
  else if (i % 2 == 0)
    return g(i-1) % n;
  else
    return g(i) % n;
}

void bitshift_left(unsigned char *b, int size, int n) {
  unsigned char prev_overflow = 0;
  unsigned char next_overflow = 0;
  for(int i = 0; i <= size-1; i++) {
    next_overflow = b[i] >> (8-n);
    b[i] = b[i] << n;
    b[i] = b[i] | prev_overflow;

    prev_overflow = next_overflow;
  }
}


bignum * chi_point2index(unsigned char *p[], int ms[], int n) {
  uint64_t h = 0;
  int e = 0;
  int d = 0;
  int m = max(ms, n);
  int M = sum(ms, n);
  bignum *big_h = bignum_alloc(M/8);

  uint64_t mu;
  uint64_t l;
  uint64_t t;
  uint64_t w;
  uint64_t r;
  for(int i = m-1; i >= 0; i--) {
    mu = extract_mask(n, ms, i, d);
    l = 0;
    for(int j = n-1; j >= 0; j--) {
      l |= (bit_at_char(p[j], i) << j);
    }
    t = rotr((l ^ e), d);
    w = inverse_gc(t);
    r = gcr(n, mu, w);
    //h = (h << parity(mu)) | r;
    bignum_shift_left(big_h, parity(mu));
    bignum_or(big_h, r);
    e = e ^ rotl(entry(w), d);
    d = (d + direction(w, n) + 1) % n;
  }

  return big_h;
}


//
// TESTS
//



MU_TEST(test_rotation) {
  // We give rotr 8 bit int, but it operates on 32 bit unsigned ints
  mu_assert_int_eq(rotr(0b00000100, 1), 0b00000010);
  mu_assert_int_eq(rotr(0b00000100, 2), 0b00000001);
  mu_assert_int_eq(rotr(0b00000001, 1), 2147483648);
  mu_assert_int_eq(rotl(0b00010100, 2), 0b01010000);
}

MU_TEST(test_gc) {
  mu_assert_int_eq(gc(0b0110), 0b0101);
  mu_assert_int_eq(gc(0b0111), 0b0100);
  mu_assert_int_eq(gc(0b1111), 0b1000);

  mu_assert_int_eq(inverse_gc(0b0101), 0b0110);
  mu_assert_int_eq(inverse_gc(0b0100), 0b0111);
  mu_assert_int_eq(inverse_gc(0b1000), 0b1111);
  mu_assert_int_eq(inverse_gc(1), 1);
}

MU_TEST(test_entry) {
  // Lifted from sfseize execution
  mu_assert_int_eq(entry(-2), 2147483650);
  mu_assert_int_eq(entry(6), 6);
  mu_assert_int_eq(entry(-6), 2147483652);
  mu_assert_int_eq(entry(2), 0);
  mu_assert_int_eq(entry(4), 3);
  mu_assert_int_eq(entry(0), 0);
}

MU_TEST(test_parity) {
  mu_assert_int_eq(parity(0b00000000), 0);
  mu_assert_int_eq(parity(0b00001000), 1);
  unsigned int i = -1;
  mu_assert_int_eq(32, parity(i));
}

MU_TEST(test_gcr) {
  int mu = 0b1111; // gcr can use the last 4 bits
  mu_assert_int_eq(gcr(2, mu, 7), 3);
}

MU_TEST(test_tsb) {
  mu_assert_int_eq(tsb(0b00001111), 4);
  mu_assert_int_eq(tsb(0b10001111), 5);
}

MU_TEST(test_max) {
  int l[] = {1,2,3};
  mu_assert_int_eq(max(l, 3), 3);
}

MU_TEST(test_bit_at_char) {
  unsigned char *b = malloc(16);
  memset(b, 0, 16);
  b[0 ] = 0b10101010;
  b[1 ] = 0b00000010;
  b[2 ] = 0b00000000;
  b[15] = 0b01000000;

  mu_assert_int_eq(bit_at_char(b, 0), 0);
  mu_assert_int_eq(bit_at_char(b, 1), 1);
  mu_assert_int_eq(bit_at_char(b, 7), 1);
  mu_assert_int_eq(bit_at_char(b, 8), 0);
  mu_assert_int_eq(bit_at_char(b, 9), 1);
  mu_assert_int_eq(bit_at_char(b, 10), 0);

  free(b);
}


MU_TEST(bignum_test_shift_left) {
  bignum *b = bignum_alloc(16);
  b->b[0 ] = 0b01001000;
  b->b[2 ] = 0b01000000;
  b->b[4 ] = 0b01000000;
  b->b[14] = 0b01000010;
  b->b[15] = 0b00100000;

  bignum_shift_left(b, 3);

  mu_assert_int_eq(b->b[0 ], 0b01000000);
  mu_assert_int_eq(b->b[14], 0b00010000);
  mu_assert_int_eq(b->b[15], 0b00000010);
}

MU_TEST_SUITE(test_suite) {
  MU_RUN_TEST(test_bit_at_char);
  MU_RUN_TEST(test_max);
  MU_RUN_TEST(test_rotation);
  MU_RUN_TEST(test_gc);
  MU_RUN_TEST(test_gcr);
  MU_RUN_TEST(test_tsb);
  MU_RUN_TEST(test_entry);
  MU_RUN_TEST(test_parity);

  // TODO: bignum tests really belong in bignum.c
  MU_RUN_TEST(bignum_test_shift_left);
}



//
// NIF API
//



static ERL_NIF_TERM
point2index(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  int tuple_size;
  const ERL_NIF_TERM *tuple;
  if(!enif_get_tuple(env, argv[0], &tuple_size, &tuple))
    return enif_make_badarg(env);

  unsigned char *points[tuple_size];
  int points_sizes[tuple_size];
  for(int i = 0; i < tuple_size; i++) {
    ErlNifBinary point;
    if(!enif_inspect_binary(env, tuple[i], &point))
      return enif_make_badarg(env);

    points[i] = point.data;
    points_sizes[i] = point.size*8;
  }

  bignum *big = chi_point2index(points, points_sizes, tuple_size);

  ERL_NIF_TERM ret;
  uint8_t *b = enif_make_new_binary(env, big->size, &ret);
  memcpy(b, big->b, big->size);
  bignum_free(big); // If we free here, we should also call alloc

  return ret;
}


static ERL_NIF_TERM
run_tests(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
  MU_RUN_SUITE(test_suite);
  MU_REPORT();

  return enif_make_atom(env, "true");
}


static ErlNifFunc nif_funcs[] = {
  {"point2index", 1, point2index},
  {"run_nif_tests", 0, run_tests}
};


ERL_NIF_INIT(hilbert, nif_funcs, NULL, NULL, NULL, NULL);
