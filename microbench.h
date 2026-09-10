#ifndef MICROBENCH_H
#define MICROBENCH_H

#define MICROBENCH_MIN_TIME 1.0 /* in seconds */

void microbench_initialize();
void microbench_run(const char *name, void (*fn)(void));

void microbench_no_optim(const void *ptr);
void microbench_clobber();

#endif
