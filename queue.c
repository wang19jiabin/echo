#include <pthread.h>
#include <stdlib.h>

struct type {
	struct type *next;
};

struct queue {
	struct type *head, **tail;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

struct queue *Queue(void)
{
	static struct queue q = {
		NULL, &q.head,
		PTHREAD_MUTEX_INITIALIZER,
		PTHREAD_COND_INITIALIZER
	};
	return &q;
}

struct queue *queue(void)
{
	struct queue *q = malloc(sizeof(struct queue));
	q->head = NULL;
	q->tail = &q->head;
	pthread_mutex_init(&q->mutex, NULL);
	pthread_cond_init(&q->cond, NULL);
	return q;
}

void _queue(struct queue *q)
{
	pthread_cond_destroy(&q->cond);
	pthread_mutex_destroy(&q->mutex);
	free(q);
}

void push(struct queue *q, void *p)
{
	struct type *t = p;
	t->next = NULL;
	pthread_mutex_lock(&q->mutex);
	*q->tail = t;
	q->tail = &t->next;
	pthread_mutex_unlock(&q->mutex);
	pthread_cond_signal(&q->cond);
}

void *pop(struct queue *q)
{
	pthread_mutex_lock(&q->mutex);
	struct type *t = q->head;
	if (t && !(q->head = t->next))
		q->tail = &q->head;

	pthread_mutex_unlock(&q->mutex);
	return t;
}

void *wait(struct queue *q)
{
	pthread_mutex_lock(&q->mutex);
	while (!q->head)
		pthread_cond_wait(&q->cond, &q->mutex);

	struct type *t = q->head;
	if (!(q->head = t->next))
		q->tail = &q->head;

	pthread_mutex_unlock(&q->mutex);
	return t;
}
