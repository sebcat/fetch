CFLAGS		:= `curl-config --cflags` -Wall -Werror -s -O2 -DNDEBUG
LDFLAGS 	:= `curl-config --libs`
RM			:= rm -f
AR			:= ar rcs
SRCS		:= main.c fetch.c

.PHONY: clean

all: curl-multi

libfetch.a: fetch.c
	$(CC) $(CFLAGS) -c $<
	$(AR) $@ fetch.o

curl-multi: libfetch.a main.c
	$(CC) $(CFLAGS) -o $@ main.c libfetch.a $(LDFLAGS)

clean:
	$(RM) curl-multi libfetch.a fetch.o
