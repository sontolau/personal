LIB=libst.a
OBJC=STnotification.o SThash.o
CFLAGS=-Wall
CC=cc


${LIB}:${OBJC}
	ar -crs ${LIB} ${OBJC}
.c.o:
	${CC} ${CFLAGS} -c $< -o $@

clean:
	rm ${OBJC} ${LIB}
