#!/bin/bash


# Receiver - Aufruf muss zuerst erfolgen, z.B. :
# 	rs485-test.sh rx /dev/ttyRS485 
# dann der Sender , z.B. :
#	rs485-test.sh tx /dev/ttyRS485 


RXTX="$1"
RSDEV="$2"
RSBAUD="19200"

if [ "x$RXTX" = "x" -o "x$RSDEV" = "x" -o "x$RSBAUD" = "x" ] ; then 
	echo "Fehler im Aufruf !  -> rs485-test.sh <tx|rx> <tty-device> " 
	exit 1
fi
if [ ! -c $RSDEV ] ; then 
	echo "ERROR : $RSDEV existiert nicht"
	exit 1
fi

# so weit so gut ...
function init() {
	stty -F $RSDEV -echo raw speed $RSBAUD > /dev/null 2>&1
}

function rs485tx() {
	cnt=1
	while [ $cnt -lt 12 ] ; 
	do
		sleep 0.3
		echo $cnt > $RSDEV
		read ack < $RSDEV
		ack=$(echo $ack|perl -p -e 's/\r//cg')
		echo "$cnt -> $ack"
		[ $(($ack - $cnt)) -ne 1 ] && echo "ERROR" && exit 1
		cnt=$(( ack + 1 ))
	done
}

function rs485rx() {
	cnt=0
	while [ $cnt -lt 11 ] ;
	do
		read ack < $RSDEV
		ack=$(echo $ack|perl -p -e 's/\r//cg')
		echo "$cnt -> $ack"
		[ $(($ack - $cnt)) -ne 1 ] && echo "ERROR" && exit 1
		cnt=$(( ack + 1 ))
		sleep 0.3
		echo $cnt > "$RSDEV"
	done
	echo "SUCCESS" && exit 0
}

function main() {
	init
	[ "$RXTX" = "tx" ] && rs485tx 
	[ "$RXTX" = "rx" ] && rs485rx 
}

# los geht es 
main
