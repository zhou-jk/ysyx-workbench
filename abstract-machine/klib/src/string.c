#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *str) {
  size_t len = 0;
  while (str[len] != '\0') {
    len++;
  }
  return len;
}

size_t strnlen(const char *str, size_t count) {
  size_t len = 0;
  while (len < count && str[len] != '\0') {
    len++;
  }
  return len;
}

char *strcpy(char *dest, const char *src) {
  return memcpy(dest, src, strlen(src) + 1);
}

char *strncpy(char *dest, const char *src, size_t count) {
  size_t size = strnlen(src, count);
  if (size != count) {
    memset(dest + size, '\0', count - size);
  }
  return memcpy(dest, src, size);
}

char *strcat(char *dest, const char *src) {
  strcpy(dest + strlen(dest), src);
  return dest;
}

int strcmp(const char *lhs, const char *rhs) {
  const unsigned char *pl = (const unsigned char *)lhs;
  const unsigned char *pr = (const unsigned char *)rhs;
  unsigned char cl, cr;

  do
  {
    cl = *pl++, cr = *pr++;
    if (cl == '\0' || cr == '\0') {
      return cl - cr;
    }
  }
  while (cl == cr);

  return cl - cr;
}

int strncmp(const char *lhs, const char *rhs, size_t count) {
  unsigned char cl = '\0', cr = '\0';
  const unsigned char *pl = (const unsigned char *)lhs;
  const unsigned char *pr = (const unsigned char *)rhs;

  if (count >= 4)
  {
    size_t count4 = count >> 2;
    do
    {
      cl = (unsigned char) *pl++;
      cr = (unsigned char) *pr++;
      if (cl == '\0' || cr == '\0' || cl != cr) {
        return cl - cr;
      }
      cl = (unsigned char) *pl++;
      cr = (unsigned char) *pr++;
      if (cl == '\0' || cr == '\0' || cl != cr) {
        return cl - cr;
      }
      cl = (unsigned char) *pl++;
      cr = (unsigned char) *pr++;
      if (cl == '\0' || cr == '\0' || cl != cr) {
        return cl - cr;
      }
      cl = (unsigned char) *pl++;
      cr = (unsigned char) *pr++;
      if (cl == '\0' || cr == '\0' || cl != cr) {
        return cl - cr;
      }
    } while (--count4 > 0);
    count &= 3;
  }

  while (count-- > 0)
  {
    cl = (unsigned char) *pl++;
    cr = (unsigned char) *pr++;
    if (cl == '\0' || cl != cr) {
      return cl - cr;
    }
  }

  return cl - cr;
}

void *memset(void *dest, int ch, size_t count) {
  uint8_t *pd = (uint8_t *)dest;
  uint8_t byte_value = (uint8_t)ch;
  uint32_t word_value;

  word_value = (byte_value << 24) | (byte_value << 16) | (byte_value << 8) | byte_value;

  while (count >= 4) {
    *(uint32_t *)pd = word_value;
    pd += 4;
    count -= 4;
  }

  while (count > 0) {
    *pd++ = byte_value;
    --count;
  }

  return dest;
}

void *memmove(void *dest, const void *src, size_t count) {
  unsigned char *pd = (unsigned char *)dest;
  const unsigned char *ps = (const unsigned char *)src;
  if (ps < pd) {
    pd += count, ps += count;
    while (count-- > 0) {
      *--pd = *--ps;
    }
  } else {
    while (count-- > 0) {
      *pd++ = *ps++;
    }
  }
  return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
  uint8_t *pd = (uint8_t *)dest;
  const uint8_t *ps = (const uint8_t *)src;

  while (count >= 4) {
    *(uint32_t *)pd = *(const uint32_t *)ps;
    pd += 4;
    ps += 4;
    count -= 4;
  }

  while (count > 0) {
    *pd++ = *ps++;
    --count;
  }
  return dest;
}
int memcmp(const void *lhs, const void *rhs, size_t count) {
  const uint8_t *p1 = (const uint8_t *)lhs;
  const uint8_t *p2 = (const uint8_t *)rhs;

  while (count >= 4) {
    uint32_t u1 = *(const uint32_t *)p1;
    uint32_t u2 = *(const uint32_t *)p2;

    if (u1 != u2) {
      while (*p1 == *p2) {
        p1++;
        p2++;
        count--;
      }
      return *p1 < *p2 ? -1 : 1;
    }

    p1 += 4;
    p2 += 4;
    count -= 4;
  }

  while (count > 0) {
    if (*p1 != *p2) {
      return *p1 < *p2 ? -1 : 1;
    }
    p1++;
    p2++;
    count--;
  }

  return 0;
}
#endif
