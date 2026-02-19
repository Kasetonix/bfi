CSTD = gnu99
SRC = bfi.c
OBJ = ${SRC:.c=.o}
CFLAGS = -std=${CSTD}

all: bfi

debug: CFLAGS += -Wall -Wextra -Werror -g
debug: LFLAGS += -fsanitize=address
debug: bfi

release: CFLAGS += -O3
release: bfi

bfi: ${OBJ}
	${CC} ${OBJ} ${LIBS} ${LFLAGS} -o $@

clean: bfi 
	rm -f ${OBJ}
