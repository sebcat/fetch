CFLAGS		:= `curl-config --cflags` -Wall -Werror -g
LDFLAGS 	:= `curl-config --libs`
RM			?= rm -f
SRCS		:= main.c fetch.c

.PHONY: clean

all: curl-multi

curl-multi: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	$(RM) curl-multi
