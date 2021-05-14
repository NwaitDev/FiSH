#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>

struct pid_list {
  pid_t *data;
  size_t capacity;
  size_t size;
};

void pid_list_create(struct pid_list *newlist);

void pid_list_destroy(struct pid_list *todestroy);

/**
 * returns 0 if append succesfull, else -1
 */
void pid_list_add(struct pid_list *list, pid_t toadd);

/**
 * returns the index of the pid if the pid is present, else -1
 */
size_t pid_list_contain(struct pid_list *list, pid_t needle);

/**
 * returns 0 if the pid has been succesfully removed, else -1
 */
int pid_list_remove(struct pid_list *list, pid_t toremove);

