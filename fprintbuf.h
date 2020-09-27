/* $Id: fprintbuf.h,v 2.0 2005-10-04 14:54:49 luis Exp $
 * Author: Luis Colorado <Luis.Colorado@HispaLinux.ES>
 * Date: Thu Aug 18 15:47:09 CEST 2005
 * Copyright: (C) 1992-2020 Luis Colorado.  All rights reserved.
 * License: BSD.
 */

/* Do not include anything BEFORE the line below, as it would not be
 * protected against double inclusion from other files
 */
#ifndef FPRINTBUF_H
#define FPRINTBUF_H

static char FPRINTBUF_H_RCSId[] = "\n$Id: fprintbuf.h,v 2.0 2005-10-04 14:54:49 luis Exp $\n";

/* prototypes */
int fprintbuf (FILE *f,	/* output file */
	long off,			/* offset of record. */
	int t,				/* buffer size */
	char *b,			/* pointer to buffer */
	char *fmt, ...);	/* first line print */

#endif /* FPRINTBUF_H */
/* Do not include anything AFTER the line above, as it would not be
 * protected against double inclusion from other files.
 */
/* $Id: fprintbuf.h,v 2.0 2005-10-04 14:54:49 luis Exp $ */
