CC=clang
CFLAGS=-Wall -Wextra -O2

all:
$(CC) $(CFLAGS) src/main.c -o pkg

clean:
rm -f pkg
