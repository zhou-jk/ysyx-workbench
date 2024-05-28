#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define BUF_SIZE 1024

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap);
int vprintf(const char* fmt, va_list vlist);


int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int done = vprintf(fmt, ap);
  va_end(ap);
  return done;
}

int vprintf(const char* fmt, va_list vlist) {
  char buffer[BUF_SIZE];
  int done = vsprintf(buffer, fmt, vlist);

  if (done >= BUF_SIZE) {
    panic("sprintf buffer overflow!!!");
  }

  putstr(buffer);

  return done;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, -1, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int done = vsprintf(out, fmt, ap);
  va_end(ap);
  return done;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int done = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return done;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  extern char* itoa(int value, char* str, int base);
  extern int atoi(const char* nptr);
  extern size_t strlen(const char *str);

  size_t written = 0;
  char number_buffer[25];

  const char* last_sign = NULL;
  for (const char *traverse = fmt; *traverse != '\0'; traverse++) {
    if (*traverse != '%' && last_sign == NULL) {
      if (written < n) {
        out[written] = *traverse;
      }
      written++;
      continue;
    }

    if (last_sign == NULL && *traverse == '%') {
      last_sign = traverse;
      continue;
    }

    switch (*traverse) {
      case 'd': {
        int i = va_arg(ap, int);
        itoa(i, number_buffer, 10);
        if (traverse - last_sign > 1) {
          char ch = ' ';
          const char* start = last_sign + 1;
          if (*start == '0') {
            ch = '0';
            ++start;
          }
          size_t length = strlen(number_buffer), limit = atoi(start);
          if (length < limit) {
            size_t count = limit - length;
            while (count-- > 0) {
              if (written < n) {
                out[written] = ch;
              }
              written++;
            }
          }
        }
        for (char *b = number_buffer; *b != '\0' && written < n; b++, written++) {
          out[written] = *b;
        }
        last_sign = NULL;
        break;
      }
      case 'x': {
        int i = va_arg(ap, int);
        itoa(i, number_buffer, 16);
        for (char *b = number_buffer; *b != '\0' && written < n; b++, written++) {
          out[written] = *b;
        }
        last_sign = NULL;
        break;
      }
      case 'u': {
        int i = va_arg(ap, unsigned int);
        itoa(i, number_buffer, 10);
        for (char *b = number_buffer; *b != '\0' && written < n; b++, written++) {
          out[written] = *b;
        }
        last_sign = NULL;
        break;
      }
      case 's': {
        const char *s = va_arg(ap, const char *);
        while (*s != '\0') {
          if (written < n) {
            out[written] = *s++;
          }
          written++;
        }
        last_sign = NULL;
        break;
      }
      case 'c': {
        char c = (char)va_arg(ap, int);
        if (written < n) {
          out[written] = c;
        }
        written++;
        last_sign = NULL;
        break;
      }
      case 'f': {
        break;
      }
      case '%': {
        if (written < n) {
          out[written] = '%';
        }
        written++;
        last_sign = NULL;
        break;
      }
      case '0' ... '9': {
        break;
      }
      default: {
        if (written < n) {
          out[written] = '%';
        }
        written++;
        if (written < n) {
          out[written] = *traverse;
        }
        written++;
        last_sign = NULL;
        break;
      }
    }
  }

  if (written < n) {
    out[written] = '\0';
  }
  else if (n > 0) {
    out[n - 1] = '\0';
  }

  return written;
}

#endif
