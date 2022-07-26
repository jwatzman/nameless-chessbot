#ifndef MT19937_H
#define MT19937_H

void init_genrand(unsigned long s);
unsigned long genrand_int32(void);

#define mt_srandom(s) init_genrand(s)
#define mt_random() genrand_int32()

#endif
