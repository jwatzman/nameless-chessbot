#include <stdlib.h>

#include "statelist.h"
#include "types.h"

/**
 * Everything in here is just a really dumb/simple singly-linked list, but it
 * works to simply keep track of the states. Would use some stl structure if
 * this were C++. Or I might template it over the type State. Whatever, this is
 * easy and it works. :)
 */

struct StatelistNode {
  struct StatelistNode* next;
  State* s;
};

struct Statelist {
  struct StatelistNode* head;
};

Statelist* statelist_alloc(void) {
  Statelist* s = malloc(sizeof(Statelist));
  s->head = NULL;
  return s;
}

State* statelist_new_state(Statelist* s) {
  struct StatelistNode* n = malloc(sizeof(struct StatelistNode));
  n->s = malloc(sizeof(State));
  n->next = s->head;
  s->head = n;

  return n->s;
}

void statelist_clear(Statelist* s) {
  struct StatelistNode* n = s->head;
  while (n) {
    struct StatelistNode* next = n->next;
    free(n->s);
    free(n);
    n = next;
  }

  s->head = NULL;
}

void statelist_free(Statelist* s) {
  statelist_clear(s);
  free(s);
}
