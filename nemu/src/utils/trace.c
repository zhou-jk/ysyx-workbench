#include <common.h>
#include <device/map.h>
#include <isa.h>

#ifdef CONFIG_IRINGTRACE
#include <cpu/decode.h>

#define IRINGBUF_SIZE 32

static Decode iringbuf[IRINGBUF_SIZE];
static int iringbuf_index = 0;

void iringbuf_get(Decode s) {
  iringbuf[iringbuf_index] = s;
  iringbuf_index++;
  if (iringbuf_index >= IRINGBUF_SIZE) {
    iringbuf_index = 0;
  }
  return;
}

static void iringbuf_translate(Decode *s) {
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val;
  for (int i = ilen - 1; i >= 0; i--) {
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) {
    space_len = 0;
  }
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

#ifndef CONFIG_ISA_loongarch32r
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p, MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
}

void iringbuf_display() {
  int iringbuf_cur = (iringbuf_index - 1) < 0 ? IRINGBUF_SIZE - 1 : iringbuf_index - 1;

  for (int i = 0; i < IRINGBUF_SIZE; i++) {
    if (i == iringbuf_cur)
      printf("%-4s", "-->");
    else
      printf("%-4s", "   ");
    iringbuf_translate(&iringbuf[i]);
    printf("%s\n", iringbuf[i].logbuf);
  }
  return;
}
#endif

#ifdef CONFIG_MTRACE
void mtrace_read_display(paddr_t addr, int len) {
  printf("read address = " FMT_PADDR " at pc = " FMT_WORD " with byte = %d\n",  addr, cpu.pc, len);
  return;
}
void mtrace_write_display(paddr_t addr, int len) {
  printf("write address = " FMT_PADDR " at pc = " FMT_WORD " with byte = %d and data =" FMT_WORD "\n", addr, cpu.pc, len, data);
  return;
}
#endif

#ifdef CONFIG_FTRACE

#include <elf.h>
elf_func_symbol func_symbol[MAX_SYMBOLS];
uint32_t func_symbol_number;

static uint32_t read_uint32_big_endian(const uint8_t *data) {
  return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

static uint16_t read_uint16_big_endian(const uint8_t *data) {
  return (data[0] << 8) | data[1];
}

void parse_elf(const char *elf_file) {
  if (elf_file == NULL) {
    Log("No ELF file is given.");
    return;
  }

  FILE *fp = fopen(elf_file, "rb");
  Assert(fp != NULL, "Cannot open '%s'", elf_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  rewind(fp);

  Log("The ELF file is %s, size = %ld", elf_file, size);

  uint8_t *elf = malloc(size);
  assert(fread(elf, size, 1, fp) == 1);
  fclose(fp);

  uint32_t sector_header_index = read_uint32_big_endian(elf + 32);
  uint16_t sector_header_length = read_uint16_big_endian(elf + 46);
  uint16_t sector_header_number = read_uint16_big_endian(elf + 48);

  uint32_t symbol_table_index = 0, symbol_table_size = 0;
  uint32_t string_table_index = 0;
  // uint32_t string_table_size = 0;

  for (uint32_t i = 0; i < sector_header_number; ++i) {
    uint32_t type = read_uint32_big_endian(elf + sector_header_index + i * sector_header_length + 4);
    uint32_t current_index = sector_header_index + i * sector_header_length;

    if (type == 2) { // Symbol table
      symbol_table_index = read_uint32_big_endian(elf + current_index + 16);
      symbol_table_size = read_uint32_big_endian(elf + current_index + 20);
    } else if (type == 3) { // String table
      string_table_index = read_uint32_big_endian(elf + current_index + 16);
      // string_table_size = read_uint32_big_endian(elf + current_index + 20);
    }

    if (symbol_table_index && string_table_index) {
      break;
    }
  }

  for (uint32_t i = 0; i < symbol_table_size / 16; ++i) {
    uint32_t entry_index = symbol_table_index + i * 16;
    if ((elf[entry_index + 12] & 0x0f) == 2) { // Function symbol
      uint32_t string_index = read_uint32_big_endian(elf + entry_index);
      uint32_t value = read_uint32_big_endian(elf + entry_index + 4);
      uint32_t size = read_uint32_big_endian(elf + entry_index + 8);

      if (func_symbol_number >= MAX_SYMBOLS) {
        Log("Function symbol storage limit exceeded.");
        break;
      }
      func_symbol[func_symbol_number++] = (elf_func_symbol){value, size, string_index};
      uint32_t str_end = string_index;
      while (elf[string_table_index + str_end]) str_end++;
      strncpy(func_symbol[func_symbol_number - 1].name, (const char *)&elf[string_table_index + string_index], str_end - string_index);
    }
  }

  free(elf);
  return;
}
#endif