#define _GNU_SOURCE
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include "common.h"
#include "connection.h"

struct req_queue req_queue = REQ_QUEUE_INITIALIZER(req_queue);

static int ignore_sigpipe()
{
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;

	if (sigemptyset(&sa.sa_mask) < 0)
		assert(!"sigemptyset");

	return sigaction(SIGPIPE, &sa, NULL);
}

static int set_nonblock(int fd)
{
	int fl = fcntl(fd, F_GETFL);
	assert(fl != -1);
	return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

static void *work(void *unused)
{
	struct msg *msg;

	while (1) {
		msg = req_queue_pop(&req_queue);
		// use msg
		connection_reply(msg->conn, msg);
	}

	assert(0);
}

static void Accept(int sfd, int efd)
{
	int cfd;
	struct connection *conn;
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLET;

	while ((cfd = accept4(sfd, NULL, NULL, SOCK_NONBLOCK)) != -1) {
		conn = connection_create(cfd, efd);
		ev.data.ptr = conn;
		if (epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev) < 0)
			assert(!"EPOLL_CTL_ADD");
	}

	assert(errno == EAGAIN);
}

static void *io(void *p)
{
	int i, n, efd, sfd = *(int *)p;
	char buf[4096];
	struct connection *conn;
	struct epoll_event ev, evs[128];

	if ((efd = epoll_create1(0)) < 0)
		assert(!"epoll_create1");

	ev.events = EPOLLIN;
	ev.data.fd = sfd;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &ev) < 0)
		assert(!"EPOLL_CTL_ADD");

	while (1) {
		n = epoll_wait(efd, evs, sizeof(evs) / sizeof(evs[0]), -1);
		if (n < 0) {
			perror("epoll_wait");
			continue;
		}

		for (i = 0; i < n; ++i) {
			if (evs[i].data.fd == sfd) {
				Accept(sfd, efd);
				continue;
			}

			conn = evs[i].data.ptr;

			if (evs[i].events & EPOLLIN) {
				log("%u: %s", evs[i].events, "EPOLLIN");
				connection_read(conn, buf, sizeof(buf));
			}

			if (evs[i].events & EPOLLOUT) {
				log("%u: %s", evs[i].events, "EPOLLOUT");
				connection_write(conn);
			}

			if (connection_done(conn)) {
				connection_destroy(conn);
			}
		}
	}
}

static int Listen(const char *port)
{
	int sfd, val = 1;
	struct addrinfo *ai, *ais, hints;

	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(NULL, port, &hints, &ais) != 0)
		return -1;

	for (ai = ais; ai; ai = ai->ai_next) {
		sfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sfd < 0)
			continue;

		setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

		if (bind(sfd, ai->ai_addr, ai->ai_addrlen) == 0)
			break;

		perror("bind");
		close(sfd);
	}

	if (ai) {
		if (listen(sfd, 128) < 0) {
			perror("listen");
			close(sfd);
			sfd = -1;
		}
	} else {
		sfd = -1;
	}

	freeaddrinfo(ais);
	return sfd;
}

int main()
{
	int i, sfd;
	pthread_t tid;

	if (ignore_sigpipe() < 0)
		assert(!"ignore_sigpipe");

	if ((sfd = Listen("1116")) < 0)
		assert(!"Listen");

	if (set_nonblock(sfd) < 0)
		assert(!"set_nonblock");

	for (i = 0; i < 4; ++i)
		pthread_create(&tid, NULL, work, NULL);

	for (i = 0; i < 3; ++i)
		pthread_create(&tid, NULL, io, &sfd);

	io(&sfd);
}
