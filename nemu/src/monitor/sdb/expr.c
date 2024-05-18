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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/paddr.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NEQ,
  /* TODO: Add more token types */
  TK_HEX, TK_DEC, TK_REG, TK_PLUS, TK_MINUS, TK_MUL, TK_DIV, TK_LB, TK_RB,
  TK_POS, TK_NEG, TK_DEREF
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  
  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {"0[xX][0-9a-fA-F]+", TK_HEX},  // hexadecimal numbers
  {"[0-9]+", TK_DEC},             // decimal numbers
  {"\\$[a-z0-9]+", TK_REG},       // register
  {" +", TK_NOTYPE},              // spaces
  {"\\+", TK_PLUS},               // plus
  {"\\-", TK_MINUS},              // minus
  {"\\*", TK_MUL},                // multiply
  {"\\/", TK_DIV},                // divide
  {"\\(", TK_LB},                 // left bracket
  {"\\)", TK_RB},                 // right bracket
  {"==", TK_EQ},                  // equal
  {"!=", TK_NEQ},                 // not equal
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        Assert(substr_len < sizeof(tokens[nr_token].str) / sizeof(tokens[nr_token].str[0]), "The Token length is out of bound!");

        switch (rules[i].token_type) {
        case TK_NOTYPE:
          break;
        case TK_HEX:
          uint32_t hex_val = (uint32_t)strtol(substr_start, NULL, 16);
          sprintf(tokens[nr_token].str, "%u", hex_val);
          tokens[nr_token].type = TK_DEC;
          nr_token++;
          break;
        case TK_REG:
          bool *success = false;
          uint32_t reg_val = isa_reg_str2val(substr_start + 1, success);
          if (!success) {
            Log("Invalid register!\n");
            return false;
          }
          sprintf(tokens[nr_token].str, "%u", reg_val);
          tokens[nr_token].type = TK_DEC;
          nr_token++;
          break;
        case TK_PLUS:
          strcpy(tokens[nr_token].str, substr_start);
          if (nr_token == 0 || (tokens[nr_token - 1].type != TK_HEX && tokens[nr_token - 1].type != TK_DEC && tokens[nr_token - 1].type != TK_REG && tokens[nr_token - 1].type != TK_RB)) {
            tokens[nr_token].type = TK_POS;
          }
          else {
            tokens[nr_token].type = TK_PLUS;
          }
          nr_token++;
          break;
        case TK_MINUS:
          strcpy(tokens[nr_token].str, substr_start);
          if (nr_token == 0 || (tokens[nr_token - 1].type != TK_HEX && tokens[nr_token - 1].type != TK_DEC && tokens[nr_token - 1].type != TK_REG && tokens[nr_token - 1].type != TK_RB)) {
            tokens[nr_token].type = TK_NEG;
          }
          else {
            tokens[nr_token].type = TK_MINUS;
          }
          nr_token++;
          break;
        case TK_MUL:
          strcpy(tokens[nr_token].str, substr_start);
          if (nr_token == 0 || (tokens[nr_token - 1].type != TK_HEX && tokens[nr_token - 1].type != TK_DEC && tokens[nr_token - 1].type != TK_REG && tokens[nr_token - 1].type != TK_RB)) {
            tokens[nr_token].type = TK_DEREF;
          }
          else {
            tokens[nr_token].type = TK_MUL;
          }
          nr_token++;
          break;
        default:
          tokens[nr_token].type = rules[i].token_type;
          strcpy(tokens[nr_token].str, substr_start);
          nr_token++;
          break;
        }
        break;
      }
    }

    if (i == NR_REGEX) {
      Log("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool check_parentheses(int p, int q) {
  if (tokens[p].type != TK_LB || tokens[q].type != TK_RB) {
    return false;
  }
  int bracket_number = 0;
  for (int i = p + 1; i < q; i++) {
    if (tokens[i].type == TK_LB) {
      bracket_number++;
    }
    else if (tokens[i].type == TK_RB) {
      if (bracket_number <= 0) {
        return false;
      }
      bracket_number--;
    }
  }
  return bracket_number == 0;
}

static int token_priority(int token_type) {
  switch (token_type)
  {
  case TK_LB:
  case TK_RB:
    return 1;
  case TK_DEREF:
  case TK_POS:
  case TK_NEG:
    return 2;
  case TK_MUL:
  case TK_DIV:
    return 4;
  case TK_PLUS:
  case TK_MINUS:
    return 5;
  case TK_EQ:
  case TK_NEQ:
    return 8;
  default:
    panic("Unknown type");
    break;
  }
  return -1;
}

static int main_operator_position(int p, int q, bool *success) {
  int position = q + 1, max_priority = 0, bracket_number = 0;
  bool have_number = false;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == TK_HEX || tokens[i].type == TK_DEC || tokens[i].type == TK_REG) {
      have_number = true;
      continue;
    }
    if (tokens[i].type == TK_LB) {
      bracket_number++;
    }
    else if (tokens[i].type == TK_RB) {
      if (bracket_number <= 0) {
        Log("Brackets don't match\n");
        *success = false;
        return -1;
      }
      bracket_number--;
    }
    else if (bracket_number == 0) {
      if (token_priority(tokens[i].type) >= max_priority) {
        position = i;
        max_priority = token_priority(tokens[i].type);
      }
    }
  }
  if (!have_number) {
    Log("No number\n");
    *success = false;
    return -1;
  }
  return position;
}

static word_t eval(int p, int q, bool *success) {
  if (p > q) {
    Log("Invaild expression\n");
    *success = false;
    return 0;
  }
  if (p == q) {
    if (tokens[p].type != TK_DEC && tokens[p].type != TK_HEX && tokens[p].type != TK_REG) {
      Log("Invaild expression\n");
      *success = false;
      return 0;
    }
    word_t val = atol(tokens[p].str);
    return val;
  }
  else if (check_parentheses(p, q)) {
    word_t val = eval(p + 1, q - 1, success);
    if (!(*success)) {
      return 0;
    }
    return val;
  }
  else {
    int op = main_operator_position(p, q, success);
    if (!(*success)) {
      return 0;
    }
    if (op == q + 1) {
      word_t val = atol(tokens[p].str);
      return val;
    }
    if (tokens[op].type == TK_DEREF) {
      if (op != p) {
        Log("Invaild expression\n");
        *success = false;
        return 0;
      }
      word_t addr = eval(op + 1, q, success);
      if (!(*success)) {
        return 0;
      }
      word_t val = paddr_read(addr, 4);
      return val;
    }
    else if (tokens[op].type == TK_POS) {
      if (op != p) {
        Log("Invaild expression\n");
        *success = false;
        return 0;
      }
      word_t val = eval(op + 1, q, success);
      if (!(*success)) {
        return 0;
      }
      return val;
    }
    else if (tokens[op].type == TK_NEG) {
      if (op != p) {
        Log("Invaild expression\n");
        *success = false;
        return 0;
      }
      word_t val = -eval(op + 1, q, success);
      if (!(*success)) {
        return 0;
      }
      return val;
    }
    word_t val1 = eval(p, op - 1, success);
    if (!(*success)) {
      return 0;
    }
    word_t val2 = eval(op + 1, q, success);
    if (!(*success)) {
      return 0;
    }
    switch (tokens[op].type) {
    case TK_PLUS:
      return val1 + val2;
    case TK_MINUS:
      return val1 - val2;
    case TK_MUL:
      return val1 * val2;
    case TK_DIV:
      if (val2 == 0) {
        Log("Divided by 0");
        *success = false;
        return 0;
      }
      return val1 / val2;
    case TK_EQ:
      return val1 == val2;
    case TK_NEQ:
      return val1 != val2;
    default:
      Assert(false, "Unknown Type");
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */

  *success = true;
  word_t val = eval(0, nr_token - 1, success);
  if (!(*success)) {
    return 0;
  }

  return val;
}
