#include <stdlib.h>
#include <stdio.h>
#include "fetch.h"

struct fetch_data {
	const char * const *urls;
	int urlix;
};

static const char *next_url(void *data) {
	const char *url;
	struct fetch_data *urls = data;

	url = urls->urls[urls->urlix];
	if (url != NULL) {
		urls->urlix++;
	}

	return url;
}

static void on_complete(struct fetch_transfer *transfer, void *data) {
	printf("%s (%d) %luB\n", transfer->url, transfer->status, transfer->nrecv);
}

int main() {
	fetch_ctx *ctx;
	const char * const urls[] = {
		"http://www.example.com/",
		"http://www.google.com/",
		"http://www.google.se/",
		NULL
	};

	struct fetch_data data;

	data.urls = urls;
	data.urlix = 0;
	ctx = fetch_new(2, next_url, on_complete, &data);
	fetch_event_loop(ctx);
	fetch_free(ctx);
	return EXIT_SUCCESS;
}
