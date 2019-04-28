curl_CFLAGS	!= curl-config --cflags
curl_LDFLAGS	!= curl-config --libs
CFLAGS		+= ${curl_CFLAGS} -Wall -Werror -O2
LDFLAGS 	+= ${curl_LDFLAGS}
RM		:= rm -f
AR		:= ar rcs
SRCS		+= main.c fetch.c

.PHONY: clean distclean

all: fetch

libfetch.a: fetch.o
	$(AR) $@ fetch.o

fetch: libfetch.a main.o
	$(CC) $(CFLAGS) -s -o $@ main.o libfetch.a $(LDFLAGS)

clean:
	$(RM) fetch libfetch.a fetch.o

distclean: clean
