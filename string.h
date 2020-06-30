#ifndef STRING_H
#define STRING_H

#include <stdlib.h>

struct string {
	char *str;
	size_t len, cap;
};

#define string_init(s) do {\
        (s)->str = NULL;\
        (s)->len = (s)->cap = 0;\
} while (0)

#define string_destroy(s) do {\
	free((s)->str);\
} while (0)

#define string_move(d, s) do {\
	(d)->str = (s)->str;\
	(d)->len = (s)->len;\
	(d)->cap = (s)->cap;\
        string_init((s));\
} while (0)

void string_append(struct string *, const char *, size_t);

#endif
