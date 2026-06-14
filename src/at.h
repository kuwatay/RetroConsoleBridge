#pragma once
#include <stdbool.h>
#include "console.h"

void at_init(void);
void at_feed(console_owner_t src, char ch);
bool at_escape_feed(console_owner_t src, char ch);
void at_enter_command_mode(console_owner_t src);
void at_reply(console_owner_t dst, const char *s);
