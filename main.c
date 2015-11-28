#include <stdlib.h>
#include "fetch.h"

struct urls {
	const char * const *urls;
	int urlix;
};

static const char *next_url(void *data) {
	const char *url;
	struct urls *urls = data;

	url = urls->urls[urls->urlix];
	if (url != NULL) {
		urls->urlix++;
	}

	return url;
}

int main() {
	struct fetch_ctx ctx;
	const char * const urls[] = {
		"http://www.example.com/",
		"http://www.example.com/",
		"http://www.example.com/",
		NULL
	};

	struct urls url_iter;
	url_iter.urls = urls;
	url_iter.urlix = 0;

	fetch_init(&ctx, 2, next_url, &url_iter);
	fetch_event_loop(&ctx);
	fetch_cleanup(&ctx);
	return EXIT_SUCCESS;
}
