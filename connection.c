#include "common.h"

struct connection {
	int cfd, efd;
	bool alive;
	struct queue *queue;
	struct string *reading;
	struct message *sending;
	size_t msgs, sent;
};

struct connection *connection(int cfd, int efd)
{
	print("accept %d", cfd);
	struct connection *conn = malloc(sizeof(struct connection));
	conn->cfd = cfd;
	conn->efd = efd;
	conn->alive = true;
	conn->queue = queue();
	conn->reading = string();
	conn->sending = NULL;
	conn->msgs = conn->sent = 0;
	return conn;
}

void _connection(struct connection *conn)
{
	print("close %d", conn->cfd);
	close(conn->cfd);
	_string(conn->reading);
	_queue(conn->queue);
	free(conn);
}

bool dead(const struct connection *conn)
{
	return !conn->alive && conn->msgs == 0;
}

struct message *message(struct string *str, struct connection *conn)
{
	struct message *msg = malloc(sizeof(struct message));
	msg->str = str;
	msg->conn = conn;
	return msg;
}

void Read(struct connection *conn)
{
	ssize_t n;

	for (char buf[11]; (n = read(conn->cfd, buf, sizeof(buf))) > 0;) {
		size_t b, e;
		for (b = e = 0; e < n; ++e) {
			if (buf[e] == '\n') {
				append(conn->reading, buf + b, e - b + 1);
				struct message *msg = message(conn->reading, conn);
				conn->reading = string();
				conn->msgs++;
				push(Queue(), msg);
				b = e + 1;
			}
		}
		append(conn->reading, buf + b, e - b);
	}

	if (n < 0 && errno == EAGAIN)
		return;

	conn->alive = false;
}

static void next(struct connection *conn)
{
	_string(conn->sending->str);
	free(conn->sending);
	conn->sending = NULL;
	conn->sent = 0;
	conn->msgs--;
}

void Send(struct connection *conn)
{
	while (conn->sending || (conn->sending = pop(conn->queue))) {
		if (!conn->alive) {
			next(conn);
			continue;
		}

		size_t l = len(conn->sending->str) - conn->sent;
		ssize_t n = write(conn->cfd, str(conn->sending->str) + conn->sent, l);
		if (n < 0 && errno == EAGAIN)
			return;

		if (n < 0) {
			perror("write");
			conn->alive = false;
		} else if (n == l) {
			next(conn);
		} else {
			conn->sent += n;
		}
	}
}

void reply(struct connection *conn, struct message *msg)
{
	push(conn->queue, msg);
	struct epoll_event ev = {
		.events = EPOLLIN | EPOLLOUT | EPOLLET,
		.data.ptr = conn
	};
	epoll_ctl(conn->efd, EPOLL_CTL_MOD, conn->cfd, &ev);
}
