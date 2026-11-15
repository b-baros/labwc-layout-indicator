#ifndef BADGES_H
#define BADGES_H

#include <stddef.h>
#include <stdint.h>

#include "keymap.h"

struct badge {
	char label[32];
	uint32_t color;
	char title[128];
	char icon_name[64];
};

/* Picks a label and colour for every layout so that no two active layouts look alike. */
void badges_assign(const struct keymap_info *keymap, struct badge *badges);

/*
 * Writes missing badge SVGs into the user's hicolor icon theme and fills in
 * icon_name. Returns how many files were written, or -1 on error.
 */
int badges_write_icons(struct badge *badges, size_t count);

/* SVG elements of one 64x64 badge, without the enclosing <svg>. */
int badge_svg_elements(char *buf, size_t size, const char *label, uint32_t color);

#endif
