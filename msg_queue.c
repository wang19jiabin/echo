#include "msg_queue.h"

void resp_queue_init(struct resp_queue *q)
{
	SIMPLEQ_INIT(&q->head);
	pthread_mutex_init(&q->mutex, NULL);
}

void resp_queue_destroy(struct resp_queue *q)
{
	pthread_mutex_destroy(&q->mutex);
}

void resp_queue_push(struct resp_queue *q, struct msg *m)
{
	pthread_mutex_lock(&q->mutex);
	SIMPLEQ_INSERT_TAIL(&q->head, m, entry);
	pthread_mutex_unlock(&q->mutex);
}

struct msg *resp_queue_pop(struct resp_queue *q)
{
	struct msg *m = NULL;
	pthread_mutex_lock(&q->mutex);
	if (!SIMPLEQ_EMPTY(&q->head)) {
		m = SIMPLEQ_FIRST(&q->head);
		SIMPLEQ_REMOVE_HEAD(&q->head, entry);
	}
	pthread_mutex_unlock(&q->mutex);
	return m;
}

void req_queue_push(struct req_queue *q, struct msg *m)
{
	pthread_mutex_lock(&q->queue.mutex);
	SIMPLEQ_INSERT_TAIL(&q->queue.head, m, entry);
	pthread_cond_signal(&q->cond);
	pthread_mutex_unlock(&q->queue.mutex);
}

struct msg *req_queue_pop(struct req_queue *q)
{
	struct msg *m;
	pthread_mutex_lock(&q->queue.mutex);
	while (SIMPLEQ_EMPTY(&q->queue.head))
		pthread_cond_wait(&q->cond, &q->queue.mutex);

	m = SIMPLEQ_FIRST(&q->queue.head);
	SIMPLEQ_REMOVE_HEAD(&q->queue.head, entry);
	pthread_mutex_unlock(&q->queue.mutex);
	return m;
}
