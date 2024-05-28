#ifdef CONFIG_FTRACE
#define MAX_SYMBOLS 128
typedef struct {
  vaddr_t value;
  uint32_t size;
  uint32_t string_index;
  char name[256];
} elf_func_symbol;

extern elf_func_symbol func_symbol[MAX_SYMBOLS];
extern uint32_t func_symbol_number;
extern void parse_elf(const char *elf_file);
#endif