#ifndef __FETCH_H
#define __FETCH_H

#include <curl/curl.h>

typedef const char *(*fetch_url_iter)(void *);

struct fetch_ctx {
	CURL **easies;
	CURLM *multi;
	int nconcurrent, nrunning;

	fetch_url_iter url_iter;
	void *url_iter_data;		
};


int fetch_init(struct fetch_ctx *ctx, int nconcurrent, fetch_url_iter iter, void *iter_data);
int fetch_event_loop(struct fetch_ctx *ctx);
void fetch_cleanup(struct fetch_ctx *ctx);


#endif
