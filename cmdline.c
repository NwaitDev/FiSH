#include "cmdline.h"

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void line_init(struct line *li) {
  assert(li);
  memset(li, 0, sizeof(struct line));
}


/**
 * Test the validity of command arguments or file names used in redirections
 * 
 * This function is static : it means that it is a local function, accessible only in this source file
 * 
 * @param word pointer on the first char of string to test
 * @return true if the word is valid, false otherwise
 */
static bool valid_cmdarg_filename(const char *word){ 
   char forbidden[] = "<>&|";// forbidden characters in commands arguments and filenames

   size_t lenf = strlen(forbidden);
   for (size_t i = 0; i < lenf ; ++i){
      char *ptr = strchr(word, forbidden[i]);
      if (ptr) {
         return false;
      }
   }
   return true;
}

/**
 * Print the string "Error while parsing: ", followed by the string "format" to stderr
 * 
 * This function is static : it means that it is a local function, accessible only in this source file.
 * The string "format" may contain format specifications that specify how subsequent arguments are
 * converted for output. So, this function can be called with a variable number of parameters.
 * NB : vfprintf() uses the variable-length argument facilities of stdarg(3)
 * 
 * @param format string
 */
static void parse_error(const char *format, ...) {
  va_list ap;

  fprintf(stderr, "Error while parsing: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

/**
 * Search a new word in the string "str" from the "index" position
 * 
 * This function is static : it means that it is a local function, accessible only in this source file.
 * After the call, "index" contains the position of the last character used plus one.
 * If a word is found, it is copied to a dynamically allocated memory space. "pword" is a pointer
 * on a pointer which retrieves the address of this dynamically allocated memory space.
 * 
 * @param str pointer on the first char of the line entered by the user
 * @param index pointer on the index
 * @param pword pointer on a pointer which retrieves the address of this dynamically allocated memory space
 * @return   0 if a word is found or if the end of the line is reached
 *           -1 if a malformed line is detected
 *           -2 if a memory allocation failure occurs
 */
static int line_next_word(const char *str, size_t *index, char **pword) {
  assert(str);
  assert(index);
  assert(pword);
  
  size_t i = *index;
  *pword = NULL;

  /* eat space */
  while (str[i] != '\0' && isspace(str[i])) {
    ++i;
  }

  /* check if it is the end of the line */
  if (str[i] == '\0') {
    *index = i;
    return 0;
  }

  size_t start = i;
  size_t end = i;
  if (str[i] == '"') {
    ++start;
    do {
      ++i;
    } while (str[i] != '\0' && str[i] != '"');

    if (str[i] == '\0') {
      parse_error("Malformed line\n");
      return -1;
    }
    assert(str[i] == '"');
    end = i;
    ++i;
  } 
  else {
    while (str[i] != '\0' && !isspace(str[i])) {
      ++i;
    }
    end = i;
  }

  *index = i;

  /* copy this word */
  assert(end >= start); 
  *pword = calloc(end - start + 1, sizeof(char));
  if (*pword == NULL){
    fprintf(stderr, "Memory allocation failure\n");
    return -2;
  }
  memcpy(*pword, str + start, end - start);
  return 0;
}




int line_parse(struct line *li, const char *str) {
  assert(li);
  assert(str);

  size_t len = strlen(str);
  assert(len >= 1); 
  if (str[len -1] != '\n'){
    fprintf(stderr, "The command line is too long\n");
    char c = 0;
    while (c != '\n'){
      c = fgetc(stdin);
    }
    return -1;
  }
  
  size_t index = 0;
  size_t curr_cmd = 0;
  size_t curr_arg = 0;
  int valret = 0; 

  for (;;) {
    /* get the next word */
    char *word;
    int err = line_next_word(str, &index, &word);
    if (err) {
      valret = -1; 
      break;
    }

    if (!word) {
      break;
    }

#ifdef DEBUG
    fprintf(stderr, "\tnew word: '%s'\n", word);
#endif

    if (strcmp(word, "|") == 0) {
      free(word);

      if (li->background) {
        parse_error("No pipe allowed after a '&'\n");
        valret = -1;
        break;
      }
      
      if (li->redirect_output) {
        parse_error("No pipe allowed after an output redirection\n");
        valret = -1;
        break;
      }

      if (curr_arg == 0){ 
        parse_error("An empty command before a pipe detected\n");
        valret = -1;
        break;
      }

      li->cmds[curr_cmd].n_args = curr_arg;
      curr_arg = 0;
      ++curr_cmd;

    } 
    else if (strcmp(word, ">") == 0) {
      free(word);

      if (li->redirect_output) {
        parse_error("Output redirection already defined\n");
        valret = -1;
        break;
      }
      
      if (li->background) {
        parse_error("No output redirection allowed after a '&'\n");
        valret = -1;
        break;
      }

      err = line_next_word(str, &index, &word);
      if (err) {
        valret = -1; 
        break;
      }

      if (!word) {
        parse_error("Waiting for a filename after an output redirection\n");
        valret = -1;
        break;
      }

      if (!valid_cmdarg_filename(word)){
        parse_error("Filename \"%s\" is not valid\n", word);
        free(word);
        valret = -1;
        break;        
      }
      
      li->redirect_output = true;
      li->file_output = word;

    } 
    else if (strcmp(word, "<") == 0) {
      free(word);

      if (li->redirect_input) {
        parse_error("Input redirection already defined\n");
        valret = -1;
        break;
      }
      
      if (li->background) {
        parse_error("No input redirection allowed after a '&'\n");
        valret = -1;
        break;
      }
      
      if (curr_cmd > 0){
        parse_error("Input redirection is only allowed for the first command\n");
        valret = -1;
        break;
      }

      err = line_next_word(str, &index, &word);
      if (err) {
        valret = -1; 
        break;
      }

      if (!word) {
        parse_error("Waiting for a filename after an input redirection\n");
        valret = -1;
        break;
      }

      if (!valid_cmdarg_filename(word)){
        parse_error("Filename \"%s\" is not valid\n", word);
        free(word);
        valret = -1;
        break;      
      }
      
      li->redirect_input = true;
      li->file_input = word;

    } 
    else if (strcmp(word, "&") == 0) {
      free(word);

      if (li->background) {
        parse_error("More than one '&' detected\n");
        valret = -1;
        break;
      }

      if (curr_arg == 0){
        parse_error("An empty command before '&' detected\n");
        valret = -1;
        break;
      }

      li->background = true;
    } 
    else {
      if (li->background) {
        free(word);
        parse_error("No more commands allowed after a '&'\n");
        valret = -1;
        break;
      }
      if (curr_cmd == MAX_CMDS) {   
        free(word);
        parse_error("Too much commands. Max: %i\n", MAX_CMDS);
        valret = -1;
        break;
      }
      if (curr_arg == MAX_ARGS) {
        free(word);
        parse_error("Too much arguments. Max: %i\n", MAX_ARGS);
        valret = -1;
        break;
      }

      if (!valid_cmdarg_filename(word)){ 
        parse_error("Argument \"%s\" is not valid\n", word);
        free(word);
        valret = -1;
        break;        
      }

      li->cmds[curr_cmd].args[curr_arg] = word;
      ++curr_arg;
    }
  } //end of the loop for

  if (!valret && curr_arg == 0) {
    if (curr_cmd > 0){
      parse_error("An empty command detected\n");
      valret = -1;
    }
    // in a real shell, "< fic" is equivalent to "test -r fic"
    else if (li->redirect_input){
      parse_error("Missing first command\n");
      valret = -1;
    }
    // in a real shell, "> fic" :
    // - creates the regular file "fic" if it does not exist, 
    // - and truncates it if it already exists
    else if (li->redirect_output){
      parse_error("Missing last command\n");
      valret = -1;
    }
  }

  if (curr_arg != 0) {
    li->cmds[curr_cmd].n_args = curr_arg; 
    ++curr_cmd;
  }
  li->n_cmds = curr_cmd;
  return valret;
}

void line_reset(struct line *li) {
  assert(li);

  for (size_t i = 0; i < li->n_cmds; ++i) {
    for (size_t j = 0; j < li->cmds[i].n_args; ++j) {
      free(li->cmds[i].args[j]);
    }
  }

  if (li->redirect_input) {
    free(li->file_input);
  }

  if (li->redirect_output) {
    free(li->file_output);
  }

  memset(li, 0, sizeof(struct line));
}
