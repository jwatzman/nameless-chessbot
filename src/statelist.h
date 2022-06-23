#ifndef _STATELIST_H
#define _STATELIST_H

#include "types.h"

typedef struct Statelist Statelist;

Statelist* statelist_alloc(void);
State* statelist_new_state(Statelist *s);
void statelist_clear(Statelist *s);
void statelist_free(Statelist *s);

#endif
