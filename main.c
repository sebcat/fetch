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

struct options {
	int nconcurrent;
	fetch_ctx * ctx;
};

struct optlist {
	char *str;
	struct optlist *next;
};

static struct optlist *optlist_cons(struct optlist *list, const char *str) {
	struct optlist *head;

	head = malloc(sizeof(struct optlist));
	if (head != NULL) {
		head->str = strdup(str);
		if (head->str == NULL) {
			free(head);
			return NULL;
		}

		head->next = list;
	}

	return head;
}

static void optlist_free(struct optlist *list) {
	struct optlist *next;
	if (list != NULL) {
		for(; list != NULL; list = next) {
			next = list->next;
			free(list->str);
			free(list);
		}
	}
}

static void usage() {
	fprintf(stderr,
		"Read a list of urls from stdin, print results to stdout\n"
		"args:\n"
		"  n <1-100>           Concurrent transfer limit\n"
		"  r <host:port:addr>  Resolve host:port to addr\n");
	exit(EXIT_FAILURE);
}


static void args(int argc, char **argv, struct options *opts) {
	int ch;
	char *eptr;
	struct optlist *resolves = NULL, *curr;

	opts->nconcurrent = DEFAULT_NCONCURRENT;
	while ((ch = getopt(argc, argv, "hn:r:")) != -1) {
		switch(ch) {
			case 'n':
				opts->nconcurrent = (int)strtol(optarg, &eptr, 10);
				if (*eptr != '\0' || opts->nconcurrent < 1 ||
						opts->nconcurrent > 100) {
					fprintf(stderr, "invalid n value\n");
					usage();
				}
				break;
			case 'r':
				resolves = optlist_cons(resolves, optarg);
				if (resolves == NULL) {
					fprintf(stderr, "optlist_cons malloc error\n");
					exit(EXIT_FAILURE);
				}
				break;
			case '?':
			case 'h':
			default:
				usage();
		}
	}

	opts->ctx = fetch_new(opts->nconcurrent, next_url, on_complete, NULL);
	if (opts->ctx == NULL) {
		fprintf(stderr, "fetch_new malloc error\n");
		exit(EXIT_FAILURE);
	}

	for (curr = resolves; curr != NULL; curr = curr->next) {
		fetch_resolve(opts->ctx, curr->str);
	}

	optlist_free(resolves);
}

int main(int argc, char *argv[]) {
	struct options opts;

	args(argc, argv, &opts);
	fetch_event_loop(opts.ctx);
	fetch_free(opts.ctx);
	return EXIT_SUCCESS;
}
