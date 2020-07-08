#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <netdb.h>

static int Connect(const char *host, const char *port)
{
	struct addrinfo *ais, hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM
	};

	int err = getaddrinfo(host, port, &hints, &ais);
	if (err != 0) {
		fprintf(stderr, "%s\n", gai_strerror(err));
		return -1;
	}

	for (struct addrinfo * ai = ais; ai; ai = ai->ai_next) {
		int fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd < 0) {
			perror("socket");
			continue;
		}

		if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
			freeaddrinfo(ais);
			return fd;
		}

		perror("connect");
		close(fd);
	}

	assert(0);
}
