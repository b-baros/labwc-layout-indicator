#ifndef LAYOUTS_H
#define LAYOUTS_H

#include <stddef.h>
#include <stdint.h>

struct layout_style {
	const char *code;
	const char *label;
	uint32_t color;
	/* Used when the layout types a non-Latin script; NULL if the language has no such label. */
	const char *native_label;
	uint32_t native_color;
	/* Tells apart layouts of the same language; NULL means the upper-cased code. */
	const char *region;
	uint32_t alt_color;
};

extern const struct layout_style layout_styles[];
extern const size_t layout_styles_count;
extern const uint32_t fallback_palette[];
extern const size_t fallback_palette_count;

const struct layout_style *layout_style_find(const char *code);

#endif
