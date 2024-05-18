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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

  uint32_t val;
  char str[32];
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

void new_wp(char *arg) {
  Assert(free_ != NULL, "No free node");
  bool success;
  uint32_t val = expr(arg, &success);
  if (!success) {
    return;
  }
  WP *now = free_;
  free_ = free_->next;
  now->next = NULL;
  strcpy(now->str, arg);
  now->val = val;
  printf("Watchpoint %d: %s\nInitial value %u\n", now->NO, now->str, now->val);

  if (head == NULL) {
    head = now;
  }
  else {
    head->next = now;
  }
  return;
}

void free_wp(int index) {
  Assert(0 <= index && index < NR_WP, "Invalid index");
  Assert(head != NULL, "Watchpoint does not exist");

  if (head->NO == index) {
    WP *newhead = head->next;
    head->next = free_;
    free_ = head;
    free_->val = 0;
    free_->str[0] = '\0';
    head = newhead;
    return;
  }

  for (WP *p = head; p->next != NULL; p = p->next) {
    if (p->next->NO == index) {
      WP *target = p->next;
      target->next = free_;
      free_ = target;
      free_->val = 0;
      free_->str[0] = '\0';
      p->next = p->next->next;
      return;
    }
  }
  printf("Watchpoint %d not found!\n", index);
  return;
}

void wp_display() {
  if (head == NULL) {
    printf("No watchpoints.\n");
    return;
  }
  for (WP *p = head; p != NULL; p = p->next) {
    printf("Watchpoint %d: %s = %u\n", p->NO, p->str, p->val);
  }
  return;
}

bool wp_check() {
  bool have_changed = false;
  for (WP *p = head; p != NULL; p = p->next) {
    uint32_t origin = p->val;
    bool success;
    uint32_t current = expr(p->str, &success);
    if (success) {
      if (origin != current) {
        printf("Watchpoint %d: %s\nOld value = %u\nNew value = %u\n", p->NO, p->str, origin, current);
        p->val = current;
        have_changed = true;
      }
    }
    else {
      assert(false);
    }
  }
  return have_changed;
}

