#ifndef __FETCH_H
#define __FETCH_H

#include <curl/curl.h>

/* maximum URL length */
#define URL_BUFSZ 2048

struct fetch_transfer {
	int id;
	char *recv;
	size_t nrecv;
	int status;
	char url[URL_BUFSZ];
};

typedef struct fetch_ctx fetch_ctx;

typedef const char *(*fetch_url_cb)(void *);
typedef void (*on_complete_cb)(struct fetch_transfer *, void *);

fetch_ctx *fetch_new(int nconcurrent, fetch_url_cb urlfetch,
		on_complete_cb on_complete, void *data);
int fetch_event_loop(struct fetch_ctx *ctx);
int fetch_resolve(struct fetch_ctx *ctx, const char *resolvestr);
void fetch_free(struct fetch_ctx *ctx);


#endif
