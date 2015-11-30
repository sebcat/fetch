#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "fetch.h"

/* received data is buffered in memory until completion. This is obviously bad
   for larger responses, however it's sensible if we don't care about larger
   responses.  */
#define FETCH_BUFSZ			(1024 * 1000 - 1)
#define CONNECT_TIMEO_S		20

struct fetch_ctx {
	CURLM *multi;
	struct fetch_transfer *transfers;
	CURL **easies;
	int nconcurrent, nrunning;
	struct curl_slist *resolvestrs;

	fetch_url_cb url_iter;
	on_complete_cb on_complete;
	void *data;
};

static int calc_http_status(const char *buf, size_t len) {
	int a,b,c;

	assert(buf != NULL);

	if (len < 12) {
		return -1;
	}
	a = buf[9] - '0';
	b = buf[10] - '0';
	c = buf[11] - '0';

	if (a < 0 || a > 9 || b < 0 || b > 9 || c < 0 || c > 9) {
		return -1;
	}

	return a*100+b*10+c;
}

static void fetch_call_complete(struct fetch_ctx *ctx,
		struct fetch_transfer *transfer) {

	assert(ctx != NULL);
	assert(transfer != NULL);

	if (ctx->on_complete != NULL) {
		transfer->status =
				calc_http_status(transfer->recv, transfer->nrecv);
		transfer->recv[transfer->nrecv] = '\0';
		ctx->on_complete(transfer, ctx->data);
	}

	transfer->nrecv = 0;
}

static size_t fetch_write(char *ptr, size_t size, size_t nmemb, void *data) {
	struct fetch_transfer *transfer;
	size_t rsize = size*nmemb, left, ncopy;

	assert(data != NULL);
	transfer = data;

	left = FETCH_BUFSZ - transfer->nrecv;
	if (rsize > 0 && left > 0)  {
		if (rsize > left) {
			ncopy = left;
		} else {
			ncopy = rsize;
		}

		memcpy(transfer->recv+transfer->nrecv, ptr, ncopy);
		transfer->nrecv += ncopy;
	}

	if (transfer->nrecv >= FETCH_BUFSZ) {
		return 0;
	}

	return rsize;
}

fetch_ctx *fetch_new(int nconcurrent, fetch_url_cb urlfetch,
		on_complete_cb on_complete, void *data) {
	fetch_ctx *ctx;
	CURL *easy;
	int i;

	assert(nconcurrent > 0);
	assert(urlfetch != NULL);

	ctx = malloc(sizeof(fetch_ctx));
	if (ctx == NULL) {
		goto fail;
	}

	memset(ctx, 0, sizeof(*ctx));
	ctx->nconcurrent = nconcurrent;
	ctx->transfers = malloc(sizeof(struct fetch_transfer)*nconcurrent);
	if (ctx->transfers == NULL) {
		goto fail;
	}

	ctx->easies = malloc(sizeof(CURL*)*nconcurrent);
	if (ctx->easies == NULL) {
		goto fail;
	}

	memset(ctx->transfers, 0, sizeof(struct fetch_transfer)*nconcurrent);
	memset(ctx->easies, 0, sizeof(CURL*)*nconcurrent);
	for (i=0; i<nconcurrent; i++) {
		ctx->transfers[i].id = i;
		ctx->transfers[i].reserved = ctx;
		ctx->transfers[i].recv = malloc(FETCH_BUFSZ+1);
		if (ctx->transfers[i].recv == NULL) {
			goto fail;
		}

		easy = curl_easy_init();
		curl_easy_setopt(easy, CURLOPT_HEADER, 1L);
		curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(easy, CURLOPT_PROTOCOLS,
				CURLPROTO_HTTP|CURLPROTO_HTTPS);
		curl_easy_setopt(easy, CURLOPT_TCP_NODELAY, 1L);
		/* CURLOPT_ACCEPT_ENCODING: "" == all supported encodings */
		curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");

		/* Abort the transfer if the transfer speed falls below
		   100 B/s for 10 seconds */
		curl_easy_setopt(easy, CURLOPT_LOW_SPEED_LIMIT, 100L);
		curl_easy_setopt(easy, CURLOPT_LOW_SPEED_TIME, 10L);
		curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEO_S);

		curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, fetch_write);
		curl_easy_setopt(easy, CURLOPT_WRITEDATA, &ctx->transfers[i]);

		/* XXX: don't verify certificates */
		curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 0L);

		ctx->easies[i] = easy;
	}

	ctx->multi = curl_multi_init();
	if (ctx->multi == NULL) {
		goto fail;
	}

	ctx->on_complete = on_complete;
	ctx->url_iter = urlfetch;
	ctx->data = data;
	return ctx;

fail:
	fetch_free(ctx);
	return NULL;
}

static int fetch_select(struct fetch_ctx *ctx) {
	fd_set fdread, fdwrite, fdexcept;
	int ret, maxfd;
	struct timeval to, wait = {0, 100*1000};
	long timeo = -1;

	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdexcept);

	curl_multi_timeout(ctx->multi, &timeo);
	if (timeo < 0) {
		to.tv_sec =1;
		to.tv_usec = 0;
	} else {
		to.tv_sec = timeo / 1000;
		to.tv_usec = (timeo % 1000) * 1000;
	}

	ret = curl_multi_fdset(ctx->multi, &fdread, &fdwrite, &fdexcept, &maxfd);
	if (ret != CURLM_OK) {
		return -1;
	}

	if (maxfd == -1) {
		return select(maxfd+1, NULL, NULL, NULL, &wait);
	} else {
		return select(maxfd+1, &fdread, &fdwrite, &fdexcept, &to);
	}
}

static int fetch_easy_id(struct fetch_ctx *ctx, CURL *easy) {
	int i;

	for(i=0; i<ctx->nconcurrent; i++) {
		if (ctx->easies[i] == easy) {
			return i;
		}
	}

	return -1;
}

int fetch_event_loop(struct fetch_ctx *ctx) {
	int i;
	const char *url = NULL;
	CURLMsg *msg;
	struct fetch_transfer *transfer;

	assert(ctx != NULL);

	for(i=0; i<ctx->nconcurrent; i++) {
		url = ctx->url_iter(ctx->data);
		if (url == NULL) {
			break;
		}

		snprintf(ctx->transfers[i].url, URL_BUFSZ, "%s", url);
		curl_easy_setopt(ctx->easies[i], CURLOPT_URL, url);
		if (ctx->resolvestrs != NULL) {
			curl_easy_setopt(ctx->easies[i], CURLOPT_RESOLVE, ctx->resolvestrs);
		}

		curl_multi_add_handle(ctx->multi, ctx->easies[i]);
	}

	curl_multi_perform(ctx->multi, &ctx->nrunning);
	do {

		if (fetch_select(ctx) == -1) {
			return -1;
		}

		curl_multi_perform(ctx->multi, &ctx->nrunning);
		for(msg = curl_multi_info_read(ctx->multi, &i); msg != NULL;
				msg = curl_multi_info_read(ctx->multi, &i)) {
			if (msg->msg==CURLMSG_DONE) {
				i = fetch_easy_id(ctx, msg->easy_handle);
				transfer = &ctx->transfers[i];
				fetch_call_complete(ctx, transfer);
				if (url != NULL) {
					url = ctx->url_iter(ctx->data);
				}

				if (url == NULL) {
					break;
				}

				snprintf(transfer->url, URL_BUFSZ, "%s", url);
				curl_multi_remove_handle(ctx->multi, msg->easy_handle);
				curl_easy_setopt(msg->easy_handle, CURLOPT_URL, url);
				curl_multi_add_handle(ctx->multi, msg->easy_handle);
			}

			curl_multi_perform(ctx->multi, &ctx->nrunning);
		}
	} while (ctx->nrunning > 0);
	return 0;
}

int fetch_resolve(struct fetch_ctx *ctx, const char *resolvestr) {
	struct curl_slist *ret;

	assert(ctx != NULL);

	if (resolvestr != NULL) {
		ret = curl_slist_append(ctx->resolvestrs, resolvestr);
		if (ret == NULL) {
			return -1;
		}

		ctx->resolvestrs = ret;
	}

	return 0;
}

void fetch_free(struct fetch_ctx *ctx) {
	int i;

	if (ctx != NULL) {
		if (ctx->transfers != NULL) {
			for(i=0; i<ctx->nconcurrent; i++) {
				if (ctx->transfers[i].recv != NULL) {
					free(ctx->transfers[i].recv);
				}
			}

			free(ctx->transfers);
		}

		if (ctx->easies != NULL) {
			for(i=0; i<ctx->nconcurrent; i++) {
				if (ctx->easies[i] != NULL) {
					curl_easy_cleanup(ctx->easies[i]);
				}
			}

			free(ctx->easies);
		}

		if (ctx->multi != NULL) {
			curl_multi_cleanup(ctx->multi);
		}

		if (ctx->resolvestrs != NULL) {
			curl_slist_free_all(ctx->resolvestrs);
		}

		free(ctx);
	}
}

