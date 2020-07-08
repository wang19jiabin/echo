#pragma once

#include <sys/syscall.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#define print(f, ...) do {\
	char S[128];\
	size_t N = snprintf(S, sizeof(S), "%ld:%s:%d:%s:"f"\n", syscall(SYS_gettid), __FILE__, __LINE__, __func__, ##__VA_ARGS__);\
	assert(N < sizeof(S));\
	write(STDOUT_FILENO, S, N);\
} while (0)

struct message {
	struct message *next;
	struct string *str;
	struct connection *conn;
};

struct connection *connection(int, int);
void _connection(struct connection *);
bool dead(const struct connection *);
void Read(struct connection *);
void Send(struct connection *);
void reply(struct connection *, struct message *);

struct queue *Queue(void);
void _queue(struct queue *);
struct queue *queue(void);
void push(struct queue *, void *);
void *pop(struct queue *);
void *wait(struct queue *);

struct string *string(void);
void _string(struct string *);
const char *str(const struct string *);
size_t len(const struct string *);
void append(struct string *, const char *, size_t);
