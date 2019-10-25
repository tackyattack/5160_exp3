#ifndef _STAT013_H
#define _STA013_H

#include "Main.h"

#define STA013_INIT_OK (0)
#define STA013_INIT_ERROR (1)
#define STA013_CFG_SENT (2)
#define STA013_CFG_ERROR (3)

uint8_t STA013_init();

#endif
