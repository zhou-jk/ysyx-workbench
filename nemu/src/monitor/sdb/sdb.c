/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();

#ifdef CONFIG_WATCHPOINT
void init_wp_pool();
void wp_display();
bool wp_check();
void wp_new(char *arg);
void wp_free(int index);
#endif

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
  int n = 1;
  if (args != NULL) {
    sscanf(args, "%d", &n);
  }
  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args) {
  if (args == NULL) {
    printf("Please specify 'r' for registers or 'w' for watchpoints.\n");
    return 0;
  }

  if (strcmp(args, "r") == 0) {
    isa_reg_display();
  } else if (strcmp(args, "w") == 0) {
#ifdef CONFIG_WATCHPOINT
    wp_display();
#else
    printf("Please enable watchpoint at nemu menuconfig\n");
#endif

  } else {
    printf("Unknown argument for info: %s\n", args);
  }

  return 0;
}

static int cmd_x(char *args) {
  if (args == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  char *ptr;
  int n = strtol(args, &ptr, 10);
  paddr_t addr = 0;
  sscanf(ptr, "%x", &addr);
  if (sscanf(ptr, "%x", &addr) == EOF) {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  else {
    for (int i = 0; i < n; i++) {
      word_t content = paddr_read(addr + i * 4, 4);
      printf("0x%x    %08x\n", addr + i * 4, content);
    }
  }
  return 0;
}

static int cmd_p(char *args) {
  if (args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }

  bool success = true;
  word_t result = expr(args, &success);

  if (success) {
    printf("%s = %u\n", args, result);
  } else {
    printf("Invalid expression: %s\n", args);
  }
  return 0;
}

static int cmd_w(char *args) {
#ifdef CONFIG_WATCHPOINT
  char *arg = strtok(NULL, " ");
  wp_new(arg);
#else
  printf("Please enable watchpoint at nemu menuconfig\n");
#endif
  return 0;
}

static int cmd_d(char *args) {
  int id = 0;
  if (args == NULL) {
    printf("Please input the index of watchpoint!\n");
    return 0;
  }
  sscanf(args, "%d", &id);
#ifdef CONFIG_WATCHPOINT
  wp_free(id);
#else
  printf("Please enable watchpoint at nemu menuconfig\n");
#endif
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "si [N] Execute the program for N steps and then pause. If N is not given, the default is 1", cmd_si },
  { "info", "info r Print register state, info w Print watchpoint information", cmd_info },
  { "x", "x N EXPR Evaluate the expression EXPR, use the result as the starting memory address, and output N consecutive 4-byte values in hexadecimal", cmd_x },
  { "p", "p EXPR Evaluate the expression EXPR", cmd_p },
  { "w", "w EXPR Pause program execution when the value of expression EXPR changes", cmd_w },
  { "d", "d N Delete the watchpoint with index N", cmd_d },
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

#ifdef CONFIG_WATCHPOINT
  /* Initialize the watchpoint pool. */
  init_wp_pool();
#endif
}
