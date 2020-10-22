/* main.h -- global configuration options and defines.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Mon Sep 21 09:48:49 EEST 2020
 * Copyright: 1992-2020 Luis Colorado.  All rights reserved.
 * License: BSD
 */
#ifndef _MAIN_H
#define _MAIN_H

#include "timestamp.h"

#define F(_fmt) __FILE__":%d:%s:(%s)"_fmt,__LINE__,__func__,getTs(chr)

#define WARN(_fmt, ...) do {                                \
        fprintf(logger, F(" WARN:"_fmt),##__VA_ARGS__);     \
    } while(0)

#define INFO(_fmt, ...) do {                                \
        fprintf(logger, F(" INFO:"_fmt),##__VA_ARGS__);     \
    } while(0)

#define ERR(_code, _fmt, ...) do {                          \
        fprintf(logger, F("ERROR: "_fmt),                   \
            ##__VA_ARGS__);                                 \
        exit(_code);                                        \
    } while(0)

#define FLAG_DEBUG      (1 << 0)

extern int flags; /* the configuration flags are known everywhere */

extern FILE *logger;

#endif /* _MAIN_H */
