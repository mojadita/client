#!/bin/sh
# $Id: winsvc.sh,v 1.5 2012/01/31 21:22:47 luis Exp $
# Author: Luis Colorado <lc@luiscoloradosistemas.com>
# Date: Fri Jan 20 23:00:06     2012
# Disclaimer: (C) 2012 LUIS COLORADO SISTEMAS S.L.U.
#             All rights reserved.

TTY=/dev/ttyS15
PORT=23456

exec cygrunsrv \
	--install=NMEASRV \
	--path=/home/luis/cliente/nmeasrv \
	--args="-i ${TTY} -p ${PORT} -d" \
	--chdir=/home/luis \
	--stdin=/dev/null \
	--stdout=/dev/null \
	--stderr=NMEASRV.log \
	--desc="Servicio para leer un gps NMEA en el puerto ${TTY} y replicar sus datos a clientes que conecten por el puerto TCP ${PORT}" \
	--disp="NMEA Service (${TTY} -> ${PORT})" \
	--type="auto"

# $Id: winsvc.sh,v 1.5 2012/01/31 21:22:47 luis Exp $
