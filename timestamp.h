/* timestamp.h -- interface definition for timestamp.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date:
 * Copyright: (C) 2020 Luis Colorado.  All rights reserved.
 * License: BSD.
 */
#ifndef _TIMESTAMP_H
#define _TIMESTAMP_H

struct chrono {
    struct timeval start, last;
    char buffer[128]; /* to print the timestamp. */
};

int startTs(struct chrono *p);
char *getTs(struct chrono *p);

#endif /* _TIMESTAMP_H */
