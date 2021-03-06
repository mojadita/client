# $Id: Makefile,v 1.1 2012/01/21 13:38:38 luis Exp $
# Author: Luis Colorado Urcola <lc@luiscoloradosistemas.com>
# Date: sáb ene 21 13:40:05 CET 2012
# PROJECT: CLIENTE
# CUSTOMER: N.A.
# DISCLAIMER: (C) 2012 LUIS COLORADO SISTEMAS S.L.U.
#             ALL RIGHTS RESERVED.

TARGETS=cliente nmeasrv srv
TOCLEAN=$(TARGETS)

RM		?= rm -f

.PHONY: all clean
all: $(TARGETS)
clean:
	$(RM) $(TOCLEAN) 
.depend:  $(TARGETS:=.c) $(cliente_objs:.o=.c)
	mkdep $(TARGETS:=.c) $(cliente_objs:.o=.c)

cliente_objs = cliente.o fprintbuf.o process.o usage.o timestamp.o
TOCLEAN += $(cliente_objs)

cliente: $(cliente_objs)
	$(CC) $(LDFLAGS) -o $@ $(cliente_objs)

# $Id: Makefile,v 1.1 2012/01/21 13:38:38 luis Exp $
#include .depend
