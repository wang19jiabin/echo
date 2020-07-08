#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <netdb.h>
#include "common.h"

static int init(void)
{
	struct sigaction sa = {
		.sa_handler = SIG_IGN,
		.sa_flags = 0
	};
	sigemptyset(&sa.sa_mask);
	return sigaction(SIGPIPE, &sa, NULL);
}

static int nonblock(int fd)
{
	int fl = fcntl(fd, F_GETFL);
	return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

static void *work(void *p)
{
	while (1) {
		struct message *msg = wait(Queue());
		reply(msg->conn, msg);
	}
	assert(0);
}

static void Accept(int sfd, int efd)
{
	for (int cfd; (cfd = accept(sfd, NULL, NULL)) != -1;) {
		nonblock(cfd);
		struct epoll_event ev = {
			.events = EPOLLIN | EPOLLOUT | EPOLLET,
			.data.ptr = connection(cfd, efd)
		};
		epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev);
	}

	if (errno != EAGAIN)
		perror("accept");
}

static void *io(void *p)
{
	int *fd = p, efd = epoll_create1(0);
	struct epoll_event evs[128], ev = {
		.events = EPOLLIN,
		.data.ptr = fd
	};
	epoll_ctl(efd, EPOLL_CTL_ADD, *fd, &ev);

	while (1) {
		int n = epoll_wait(efd, evs, sizeof(evs) / sizeof(evs[0]), -1);
		if (n < 0) {
			perror("epoll_wait");
			continue;
		}

		for (int i = 0; i < n; ++i) {
			if (evs[i].data.ptr == fd) {
				Accept(*fd, efd);
				continue;
			}

			if (evs[i].events & EPOLLIN)
				Read(evs[i].data.ptr);

			if (evs[i].events & EPOLLOUT)
				Send(evs[i].data.ptr);

			if (dead(evs[i].data.ptr))
				_connection(evs[i].data.ptr);
		}
	}

	assert(0);
}

static int Listen(const char *port)
{
	struct addrinfo *ais, hints = {
		.ai_flags = AI_PASSIVE,
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM
	};

	int err = getaddrinfo(NULL, port, &hints, &ais);
	if (err) {
		fprintf(stderr, "%s\n", gai_strerror(err));
		exit(EXIT_FAILURE);
	}

	for (struct addrinfo * ai = ais; ai; ai = ai->ai_next) {
		int fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd < 0) {
			perror("socket");
			continue;
		}

		int opt = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (bind(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
			if (listen(fd, 128) == 0) {
				freeaddrinfo(ais);
				return fd;
			}
		}
		close(fd);
	}
	exit(EXIT_FAILURE);
}

int main(void)
{
	int i, fd;
	pthread_t tid;

	init();
	fd = Listen("1116");
	nonblock(fd);

	for (i = 0; i < 4; ++i)
		pthread_create(&tid, NULL, work, NULL);

	for (i = 0; i < 3; ++i)
		pthread_create(&tid, NULL, io, &fd);

	io(&fd);
}
