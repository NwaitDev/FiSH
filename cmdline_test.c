#include "cmdline.h"

#include <string.h>
#include <stdio.h>

#define OK 0
#define KO 1


/**
 * Test a command line "str"
 * 
 * This function is static : it means that it is a local function, accessible only in this source file.
 * This function prints "TEST OK!" if line_parse() returns a value consistent with the one transmitted 
 * via the parameter "expected", and another significant message otherwise
 * 
 * @param str command line to test
 * @param expected OK if the command line is expected to be valid, KO otherwise
 */
static void try(const char *str, int expected) {
  static int n = 0;
  static struct line li;
  
  if (n == 0){
    line_init(&li);
  }
  
  printf("TEST #%i\n", ++n);

  int err = line_parse(&li, str);

  if ((!!err) != (!!expected)) {
    printf("UNEXPECTED RETURN WITH: %s\n", str);
  } 
  else {
    if (err){
      printf("Command line : %s", str);
    }
    printf("TEST OK!\n");
  }
  line_reset(&li);
}


int main() {

  // things working
  try("bar\n", OK);
  try("bar baz\n", OK);
  try("bar 123\n", OK);
  try("bar > baz\n", OK);
  try("bar < baz\n", OK);
  try("bar > baz < qux\n", OK);
  try("bar &\n", OK);
  try("bar > baz &\n", OK);
  try("bar < baz &\n", OK);
  try("bar > baz < qux &\n", OK);
  try("bar | baz\n", OK);
  try("bar | baz &\n", OK);
  try("bar | baz > qux\n", OK);
  try("bar | > qux baz\n", OK);
  try("bar < qux | baz\n", OK);
  try("< qux bar | baz\n", OK);
  try("< fic1 bar > fic2 bar1\n", OK);
  try("bar bar1 < qux | baz\n", OK);
  try("bar < qux baz\n", OK);
  try("bar > qux baz\n", OK);
  try("bar | baz | qux\n", OK);
  try("bar \"baz\"\n", OK);
  try("bar \"baz qux\"\n", OK);
  try("     \n", OK);
  try("\n", OK);

  // things not working
  try("bar \"bar\n", KO);	
  
  try("bar & | baz\n", KO);
  try("bar > qux | baz\n", KO);
  try("bar > qux |\n", KO);
  try("bar | | barz\n", KO);
  try("|\n", KO);
  
  try("bar > qux > baz\n", KO);
  try("bar & > qux\n", KO);
  try("bar >\n", KO);
  try("bar > qu&x\n", KO);
  try("bar > >\n", KO);
  
  try("bar < qux < baz\n", KO);
  try("bar < qux <\n", KO);
  try("bar & < qux\n", KO);
  try("bar bar | baz < qux\n", KO);
  try("bar | < qux\n", KO);
  try("bar <   \n", KO);
  try("bar <\n", KO);
  try("bar < < baz\n", KO);
  try("bar < <baz\n", KO);
  
  try("bar & &\n", KO);		
  try("bar | &\n", KO);
  try("& \n", KO);
  
  try("bar & baz\n", KO);
  try("bar & ba&z\n", KO);
  try("bar << baz\n", KO);
  try("bar &ml baz\n", KO);
  
  try("bar |\n", KO);
  try("bar | > qux\n", KO);
  try("< qux \n", KO);
  try("< fic1 > fic2\n", KO);
  try("> qux \n", KO);
  

  return 0;
}
