#ifndef CMDLINE_H
#define CMDLINE_H

#include <stddef.h>
#include <stdbool.h>

#define MAX_ARGS 16
#define MAX_CMDS 16

struct cmd {
  char *args[MAX_ARGS + 1]; //+1 to have a NULL at the end if nargs = MAX_ARGS
  size_t n_args;
};

struct line {
  struct cmd cmds[MAX_CMDS];
  size_t n_cmds;
  bool redirect_input;
  char *file_input;
  bool redirect_output;
  char *file_output;
  bool background;
};

/**
 * Init a struct line
 * 
 * All bytes occupied by the structure are set to 0
 * 
 * @param li pointer on the struct line to initialize
 */
void line_init(struct line *li);


/**
 * Parse the string "str" and construct the struct line pointed by "li" 
 * 
 * You must call "line_init" or "line_reset" before calling this function
 * 
 * @param li pointer on the struct line to fill
 * @param str pointer on the first char of string line entered by the user
 * @return 0 on success, -1 on failure
 */
int line_parse(struct line *li, const char *str);

/**
 * Reset a struct line
 * 
 * Free dynamically allocated memory
 * All bytes occupied by the structure are set to 0
 * 
 * @param li pointer on the struct line to reset
 */
void line_reset(struct line *li);

#endif
