#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdbool.h>
#include "msg_queue.h"

struct connection *connection_create(int, int);

void connection_destroy(struct connection *);

bool connection_done(const struct connection *);

void connection_read(struct connection *, char *, size_t);

void connection_write(struct connection *);

void connection_reply(struct connection *, struct msg *);

#endif
