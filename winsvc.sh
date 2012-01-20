#!/bin/sh
# $Id: winsvc.sh,v 1.1 2012/01/20 22:02:08 luis Exp $
# Author: Luis Colorado <lc@luiscoloradosistemas.com>
# Date: Fri Jan 20 23:00:06     2012
# Disclaimer: (C) 2012 LUIS COLORADO SISTEMAS S.L.U.
#             All rights reserved.

exec cygrunsrv \
	--install=NMEASRV \
	--path=/home/luis/cliente/nmeasrv \
	--args="-i /dev/ttyS15 -p 1234" \
	--chdir=/home/luis \
	--desc="Servicio para leer un gps NMEA y replicar sus datos a clientes que conecten por el puerto TCP 1234" \
	--disp="NMEA Service" \
	--type="auto"

# $Id: winsvc.sh,v 1.1 2012/01/20 22:02:08 luis Exp $
