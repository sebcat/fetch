#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "fetch.h"

#define DEFAULT_NCONCURRENT		2

#define IS_TRIM(c) \
	((c) == ' ' || \
	(c) == '\r' || \
	(c) == '\n' || \
	(c) == '\t')


static const char *next_url(void *data) {
	static char urlbuf[URL_BUFSZ], *cur, *end;
	for (cur = fgets(urlbuf, URL_BUFSZ, stdin); cur != NULL;
			cur = fgets(urlbuf, URL_BUFSZ, stdin)) {
		end = cur+strlen(cur)-1;
		while(IS_TRIM(*end) && end > cur) {
			end--;
		}

		*++end='\0';

		while(*cur != '\0' && IS_TRIM(*cur)) {
			cur++;
		}

		if (*cur) {
			break;
		}
	}

	return cur;
}

static void on_complete(struct fetch_transfer *transfer, void *data) {
	printf("%s (%d) %luB\n", transfer->url, transfer->status, transfer->nrecv);
}

static void usage() {
	fprintf(stderr,
		"Read a list of urls from stdin, print results to stdout\n"
		"args:\n"
		"  n        maximum number of concurrent transfers (1-100)\n");
	exit(EXIT_FAILURE);
}

struct options {
	int nconcurrent;
};

static void args(int argc, char **argv, struct options *opts) {
	int ch;
	char *eptr;

	opts->nconcurrent = DEFAULT_NCONCURRENT;
	while ((ch = getopt(argc, argv, "hn:")) != -1) {
		switch(ch) {
			case 'n':
				opts->nconcurrent = (int)strtol(optarg, &eptr, 10);
				if (*eptr != '\0' || opts->nconcurrent < 1 ||
						opts->nconcurrent > 100) {
					fprintf(stderr, "invalid n value\n");
					usage();
				}
				break;
			case '?':
			case 'h':
			default:
				usage();
		}
	}
}

int main(int argc, char *argv[]) {
	fetch_ctx *ctx;
	struct options opts;

	args(argc, argv, &opts);

	ctx = fetch_new(opts.nconcurrent, next_url, on_complete, NULL);
	fetch_event_loop(ctx);
	fetch_free(ctx);
	return EXIT_SUCCESS;
}
