#ifndef __PKG_SAMPLE__
#define __PKG_SAMPLE__
#include <stdint.h>

void sample_install(void (*callback)(uint32_t));
void sample_trigger(void);

#endif
