#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H

#include <sys/queue.h>
#include <pthread.h>
#include "string.h"

struct msg {
	SIMPLEQ_ENTRY(msg) entry;
	struct string str;
	struct connection *conn;
};

#define msg_create() calloc(1, sizeof(struct msg))

#define msg_destroy(m) do {\
	string_destroy(&(m)->str);\
	free((m));\
} while (0)

struct resp_queue {
	SIMPLEQ_HEAD(, msg) head;
	pthread_mutex_t mutex;
};

void resp_queue_init(struct resp_queue *);

void resp_queue_destroy(struct resp_queue *);

void resp_queue_push(struct resp_queue *, struct msg *);

struct msg *resp_queue_pop(struct resp_queue *);

struct req_queue {
	struct resp_queue queue;
	pthread_cond_t cond;
};

#define REQ_QUEUE_INITIALIZER(q) { { SIMPLEQ_HEAD_INITIALIZER((q).queue.head),\
	PTHREAD_MUTEX_INITIALIZER }, PTHREAD_COND_INITIALIZER }

void req_queue_push(struct req_queue *, struct msg *);

struct msg *req_queue_pop(struct req_queue *);

#endif
