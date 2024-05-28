#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;
static bool is_init_addr = false;
static void *addr;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr++;
  }
  return x;
}

char* itoa(int value, char* str, int base) {
  char* p = str;
  if (value < 0) {
    *p++ = '-';
  }
  int shifter = value;
  do {
    ++p;
    shifter /= base;
  } while (shifter);
  *p = '\0';
  do {
    *--p = "0123456789abcdefghijklmnopqrstuvwxyz"[abs(value % base)];
    value /= base;
  } while (value);
  return str;
}

void init_addr() {
  addr = (void *)ROUNDUP(heap.start, 8);
  is_init_addr = true;
  return;
}

void *malloc(size_t size) {
  if (!is_init_addr) {
    init_addr();
  }
  size = (size_t)ROUNDUP(size, 8);
  char *old = addr;
  addr += size;
  assert((uintptr_t)heap.start <= (uintptr_t)addr && (uintptr_t)addr < (uintptr_t)heap.end);
  // for (uint64_t *p = (uint64_t *)old; p != (uint64_t *)addr; p++) {
  //   *p = 0;
  // }
  return old;
}

void free(void *ptr) {
}

#endif
