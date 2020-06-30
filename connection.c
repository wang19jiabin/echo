#include "common.h"
#include "connection.h"

extern struct req_queue req_queue;

struct connection {
	int cfd, efd;
	bool alive;
	size_t pending_reqs;
	struct string buf;
	struct resp_queue resp_queue;
	struct {
		struct msg *resp;
		size_t sent;
	} sending;
};

struct connection *connection_create(int cfd, int efd)
{
	struct connection *conn = malloc(sizeof(struct connection));
	assert(conn);
	conn->cfd = cfd;
	conn->efd = efd;
	conn->alive = true;
	conn->pending_reqs = 0;
	string_init(&conn->buf);
	resp_queue_init(&conn->resp_queue);
	conn->sending.resp = NULL;
	conn->sending.sent = 0;
	return conn;
}

void connection_destroy(struct connection *conn)
{
	close(conn->cfd);
	resp_queue_destroy(&conn->resp_queue);
	string_destroy(&conn->buf);
	free(conn);
}

bool connection_done(const struct connection *conn)
{
	return !conn->alive && conn->pending_reqs == 0;
}

static void dispatch(struct connection *conn, const char *buf, size_t len)
{
	struct msg *req;
	size_t beg, end;

	for (beg = end = 0; end < len; ++end) {
		if (buf[end] == '\0') {
			string_append(&conn->buf, buf + beg, end - beg);
			req = msg_create();
			req->conn = conn;
			string_move(&req->str, &conn->buf);
			req_queue_push(&req_queue, req);
			++conn->pending_reqs;
			beg = end + 1;
		}
	}

	if (beg < end)
		string_append(&conn->buf, buf + beg, end - beg);
}

void connection_read(struct connection *conn, char *buf, size_t len)
{
	ssize_t n;

	while ((n = read(conn->cfd, buf, len)) > 0)
		dispatch(conn, buf, n);

	if (n == -1 && errno == EAGAIN) {
		log("EAGAIN");
		return;
	}

	if (n == 0)
		log("EOF");
	else
		perror("read");

	conn->alive = false;
}

static void reset_sending(struct connection *conn)
{
	msg_destroy(conn->sending.resp);
	conn->sending.resp = NULL;
	conn->sending.sent = 0;
}

void connection_write(struct connection *conn)
{
	const char *str;
	size_t len;
	ssize_t n;

	while (conn->sending.resp
	       || (conn->sending.resp = resp_queue_pop(&conn->resp_queue))) {
		if (!conn->alive) {
			reset_sending(conn);
			continue;
		}

		str = conn->sending.resp->str.str + conn->sending.sent;
		len = conn->sending.resp->str.len - conn->sending.sent;
		if ((n = write(conn->cfd, str, len)) < 0) {
			if (errno == EAGAIN) {
				log("EAGAIN");
				return;
			} else {
				perror("write");
				conn->alive = false;
				reset_sending(conn);
				continue;
			}
		}

		if (n == len)
			reset_sending(conn);
		else
			conn->sending.sent += n;

	}
}

void connection_reply(struct connection *conn, struct msg *resp)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
	ev.data.ptr = conn;

	resp_queue_push(&conn->resp_queue, resp);
	epoll_ctl(conn->efd, EPOLL_CTL_MOD, conn->cfd, &ev);
	assert(!"EPOLL_CTL_MOD");
}
