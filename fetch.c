#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "fetch.h"

int fetch_init(struct fetch_ctx *ctx, int nconcurrent, fetch_url_iter iter, void *iter_data) {
	int i;

	assert(ctx != NULL);
	assert(nconcurrent > 0);
	assert(iter != NULL);

	memset(ctx, 0, sizeof(*ctx));
	ctx->nconcurrent = nconcurrent;
	ctx->easies = malloc(sizeof(CURL*)*nconcurrent);
	if (ctx->easies == NULL) {
		return -1;
	}

	for (i=0; i<nconcurrent; i++) {
		ctx->easies[i] = curl_easy_init();
		curl_easy_setopt(ctx->easies[i], CURLOPT_HEADER, 1L);
		curl_easy_setopt(ctx->easies[i], CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(ctx->easies[i], CURLOPT_PROTOCOLS, CURLPROTO_HTTP|CURLPROTO_HTTPS);
		curl_easy_setopt(ctx->easies[i], CURLOPT_TCP_NODELAY, 1L);
		/* CURLOPT_ACCEPT_ENCODING: "" == all supported encodings */
		curl_easy_setopt(ctx->easies[i], CURLOPT_ACCEPT_ENCODING, "");
	}

	ctx->multi = curl_multi_init();
	ctx->url_iter = iter;
	ctx->url_iter_data = iter_data;
	return 0;
}

static int fetch_select(struct fetch_ctx *ctx) {
	fd_set fdread, fdwrite, fdexcept;
	int ret, maxfd;
	struct timeval to;
	long timeo;

	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdexcept);

	if (curl_multi_timeout(ctx->multi, &timeo) != CURLM_OK) {
		return -1;
	}

	to.tv_sec = timeo / 1000;
	if (to.tv_sec > 1) {
		to.tv_sec = 1;
	} else {
		to.tv_usec = (timeo % 1000) * 1000;
	}
	
	ret = curl_multi_fdset(ctx->multi, &fdread, &fdwrite, &fdexcept, &maxfd);
	if (ret != CURLM_OK) {
		return -1;
	}
	
	return select(maxfd+1, &fdread, &fdwrite, &fdexcept, &to);
}

int fetch_event_loop(struct fetch_ctx *ctx) {
	int i;
	const char *url;
	CURLMsg *msg;

	assert(ctx != NULL);

	for(i=0; i<ctx->nconcurrent; i++) {
		url = ctx->url_iter(ctx->url_iter_data);
		if (url == NULL) {
			break;
		}

		curl_easy_setopt(ctx->easies[i], CURLOPT_URL, url);
		curl_multi_add_handle(ctx->multi, ctx->easies[i]);
	}

	do {
		if (fetch_select(ctx) == -1) {
			return -1;
		}

		curl_multi_perform(ctx->multi, &ctx->nrunning);
		for(msg = curl_multi_info_read(ctx->multi, &i); msg != NULL; 
				msg = curl_multi_info_read(ctx->multi, &i)) {
			if (msg->msg==CURLMSG_DONE) {
				url = ctx->url_iter(ctx->url_iter_data);
				if (url != NULL) {
					curl_easy_setopt(msg->easy_handle, CURLOPT_URL, url);
				}
			}
		}
	} while (ctx->nrunning > 0);
	return 0;
}

void fetch_cleanup(struct fetch_ctx *ctx) {
	int i;

	if (ctx != NULL) {
		if (ctx->easies != NULL) {
			for(i=0; i<ctx->nconcurrent; i++) {
				curl_easy_cleanup(ctx->easies[i]);
			}

			free(ctx->easies);
		}

		if (ctx->multi != NULL) {
			curl_multi_cleanup(ctx->multi);
		}
	}
}

