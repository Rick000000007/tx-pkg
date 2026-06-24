CC=clang
CFLAGS=-Wall -Wextra -O2

SRC = \
src/main.c \
src/db.c \
src/doctor.c \
src/info.c \
src/install.c \
src/json.c \
src/list.c \
src/parser.c \
src/remove.c \
src/search.c \
src/stats.c \
src/txbuild.c \
src/txrepo.c \
src/upgrade.c \
src/verify.c

all:
	$(CC) $(CFLAGS) $(SRC) -o pkg

clean:
	rm -f pkg
