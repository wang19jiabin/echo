#include <stdlib.h>
#include <string.h>

struct string {
	char *str;
	size_t len, cap;
};

struct string *string(void)
{
	struct string *s = malloc(sizeof(struct string));
	s->len = 0;
	s->cap = 1;
	s->str = malloc(s->cap);
	return s;
}

void _string(struct string *s)
{
	free(s->str);
	free(s);
}

const char *str(const struct string *s)
{
	return s->str;
}

size_t len(const struct string *s)
{
	return s->len;
}

void append(struct string *s, const char *c, size_t n)
{
	size_t len = s->len + n;
	if (len > s->cap) {
		do {
			s->cap *= 2;
		} while (len > s->cap);
		s->str = realloc(s->str, s->cap);
	}
	memcpy(s->str + s->len, c, n);
	s->len = len;
}
