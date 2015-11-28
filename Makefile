CFLAGS		:= `curl-config --cflags` -Wall -Werror -s -O2 -DNDEBUG
LDFLAGS 	:= `curl-config --libs`
RM			?= rm -f
SRCS		:= main.c fetch.c

.PHONY: clean

all: curl-multi

curl-multi: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	$(RM) curl-multi
