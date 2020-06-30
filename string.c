#include <assert.h>
#include <string.h>
#include "string.h"

void string_append(struct string *s, const char *c, size_t n)
{
	const size_t len = s->len + n;

	if (len > s->cap) {
		if (s->cap == 0)
			s->cap = 32;

		while (len > s->cap)
			s->cap *= 2;

		s->str = realloc(s->str, s->cap);
		assert(s->str);
	}

	memcpy(s->str + s->len, c, n);
	s->len = len;
}
