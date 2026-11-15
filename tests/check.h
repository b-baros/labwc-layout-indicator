#ifndef CHECK_H
#define CHECK_H

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
			failures++; \
		} \
	} while (0)

#define CHECK_STR(actual, expected) \
	do { \
		const char *a_ = (actual), *e_ = (expected); \
		if (strcmp(a_, e_) != 0) { \
			fprintf(stderr, "%s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, a_, e_); \
			failures++; \
		} \
	} while (0)

#endif
