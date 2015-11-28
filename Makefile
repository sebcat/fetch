CFLAGS		:= `curl-config --cflags` -Wall -Werror -O2 -DNDEBUG
LDFLAGS 	:= `curl-config --libs`
RM			:= rm -f
AR			:= ar rcs
SRCS		:= main.c fetch.c

.PHONY: clean distclean

all: curl-multi

libfetch.a: fetch.c
	$(CC) $(CFLAGS) -c fetch.c
	$(AR) $@ fetch.o

curl-multi: libfetch.a main.c
	$(CC) $(CFLAGS) -s -o $@ main.c libfetch.a $(LDFLAGS)

clean:
	$(RM) curl-multi libfetch.a fetch.o

distclean: clean
