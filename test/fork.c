#include <sys/wait.h>
#include <stdlib.h>
#include "connect.h"

static int test()
{
	int fd = Connect("127.0.0.1", "1116");
	char buf[1024];
	size_t len = 0;

	for (size_t i = 0; i < 11; ++i) {
		for (size_t j = 0; j < i; ++j)
			buf[len++] = '0' + j;

		buf[len++] = '\n';
	}

	ssize_t n = write(fd, buf, len);
	assert(n == len);

	while (len > 0) {
		n = read(fd, buf, sizeof(buf));
		assert(n > 0);
		len -= n;
		write(STDOUT_FILENO, buf, n);
	}

	return 0;
}

int main(int c, char *v[])
{
	int i, n;

	if (c == 1)
		n = 1;
	else if (c == 2)
		n = atoi(v[1]);
	else
		return -1;

	for (i = 0; i < n; ++i) {
		pid_t pid = fork();
		if (pid == 0)
			return test();

		if (pid == -1) {
			perror("fork");
			break;
		}
	}

	while (wait(NULL) != -1)
		--i;

	perror("wait");
	assert(i == 0);
}
