#include <poll.h>
#include <ctype.h>
#include "connect.h"

static void Send(int fd)
{
	char buf[128];
	ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
	assert(n > 0);
	for (ssize_t i = 0; i < n; ++i) {
		if (isspace(buf[i]))
			buf[i] = '\n';
	}
	ssize_t s = write(fd, buf, n);
	assert(s == n);
}

static void Read(int fd)
{
	char buf[128];
	ssize_t n = read(fd, buf, sizeof(buf));
	assert(n > 0);
	write(STDOUT_FILENO, buf, n);
}

int main()
{
	int fd = Connect("127.0.0.1", "1116");
	struct pollfd fds[2] = {
		[0].fd = STDIN_FILENO,
		[0].events = POLLIN,
		[1].fd = fd,
		[1].events = POLLIN
	};

	while (1) {
		if (poll(fds, sizeof(fds) / sizeof(fds[0]), -1) < 0) {
			perror("poll");
			continue;
		}

		if (fds[0].revents & POLLIN)
			Send(fd);

		if (fds[1].revents & POLLIN)
			Read(fd);
	}
}
