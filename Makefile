# $Id: Makefile,v 1.1 2012/01/21 13:38:38 luis Exp $
# Author: Luis Colorado Urcola <lc@luiscoloradosistemas.com>
# Date: sáb ene 21 13:40:05 CET 2012
# PROJECT: CLIENTE
# CUSTOMER: N.A.
# DISCLAIMER: (C) 2012 LUIS COLORADO SISTEMAS S.L.U.
#             ALL RIGHTS RESERVED.

TARGETS=cliente nmeasrv

.PHONY: all clean
all: $(TARGETS)
clean:
	$(RM) $(TARGETS) $(cliente_objs) $(nmeasrv_objs)

cliente_objs=cliente.o
nmeasrv_objs=nmeasrv.o

cliente: $(cliente_objs)
	$(CC) $(LDFLAGS) -o cliente $(cliente_objs)

nmeasrv: $(nmeasrv_objs)
	$(CC) $(LDFLAGS) -o nmeasrv $(nmeasrv_objs)

# $Id: Makefile,v 1.1 2012/01/21 13:38:38 luis Exp $
