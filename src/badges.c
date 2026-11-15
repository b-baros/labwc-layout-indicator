#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "badges.h"
#include "layouts.h"

/* Bump when the SVG design changes, so trays don't keep showing cached old icons. */
#define DESIGN_VERSION 1

struct candidate {
	char base[32];
	char region[32];
	char variant_tag[8];
	uint32_t color;
	uint32_t alt_color;
};

static void upper_copy(char *dst, size_t size, const char *src, size_t max_chars)
{
	size_t n = 0;
	for (; *src && n < max_chars && n + 1 < size; src++)
		if (isalnum((unsigned char)*src))
			dst[n++] = (char)toupper((unsigned char)*src);
	dst[n] = '\0';
}

static uint32_t fnv1a(const char *s)
{
	uint32_t h = 2166136261u;
	for (; *s; s++) {
		h ^= (unsigned char)*s;
		h *= 16777619u;
	}
	return h;
}

static void make_variant_tag(const char *variant, char *tag, size_t size)
{
	static const struct { const char *needle, *tag; } known[] = {
		{ "dvorak", "DV" }, { "colemak", "CM" }, { "workman", "WM" },
		{ "intl", "INT" }, { "phonetic", "PH" },
	};
	tag[0] = '\0';
	if (!*variant)
		return;
	for (size_t i = 0; i < sizeof(known) / sizeof(known[0]); i++) {
		if (strstr(variant, known[i].needle)) {
			snprintf(tag, size, "%s", known[i].tag);
			return;
		}
	}
	upper_copy(tag, size, variant, 3);
}

static void make_candidate(const struct layout_info *layout, struct candidate *c)
{
	const struct layout_style *style = layout_style_find(layout->code);
	memset(c, 0, sizeof(*c));
	if (style) {
		bool native = style->native_label && layout->native_script;
		snprintf(c->base, sizeof(c->base), "%s", native ? style->native_label : style->label);
		c->color = native ? style->native_color : style->color;
		c->alt_color = style->alt_color;
		if (style->region)
			snprintf(c->region, sizeof(c->region), "%s", style->region);
		else
			upper_copy(c->region, sizeof(c->region), layout->code, 3);
	} else {
		if (layout->code[0])
			upper_copy(c->base, sizeof(c->base), layout->code, 3);
		else
			upper_copy(c->base, sizeof(c->base), layout->name, 2);
		if (!c->base[0])
			snprintf(c->base, sizeof(c->base), "?");
		uint32_t h = fnv1a(layout->code[0] ? layout->code : layout->name);
		c->color = fallback_palette[h % fallback_palette_count];
		c->alt_color = fallback_palette[(h + 1) % fallback_palette_count];
		upper_copy(c->region, sizeof(c->region), layout->code, 3);
	}
	make_variant_tag(layout->variant, c->variant_tag, sizeof(c->variant_tag));
}

static void mark_duplicates(char labels[][32], const size_t *members, size_t count, bool *dup)
{
	for (size_t a = 0; a < count; a++) {
		dup[a] = false;
		for (size_t b = 0; b < count; b++)
			if (a != b && strcmp(labels[members[a]], labels[members[b]]) == 0)
				dup[a] = true;
	}
}

/*
 * Layouts sharing a label are told apart by region (US/UK), then by variant
 * (EN/DV for us and us(dvorak)), and as a last resort by a number (SR1/SR2).
 */
static void resolve_labels(const struct candidate *c, size_t n, char labels[][32])
{
	for (size_t i = 0; i < n; i++)
		snprintf(labels[i], 32, "%.31s", c[i].base);

	for (size_t first = 0; first < n; first++) {
		size_t members[MAX_LAYOUTS], count = 0;
		bool seen_before = false;
		for (size_t j = 0; j < n; j++) {
			if (strcmp(c[j].base, c[first].base) != 0)
				continue;
			if (j < first)
				seen_before = true;
			members[count++] = j;
		}
		if (seen_before || count < 2)
			continue;

		bool same_region = true;
		for (size_t m = 1; m < count; m++)
			same_region &= strcmp(c[members[m]].region, c[members[0]].region) == 0;
		if (!same_region)
			for (size_t m = 0; m < count; m++)
				if (c[members[m]].region[0])
					snprintf(labels[members[m]], 32, "%.31s", c[members[m]].region);

		bool dup[MAX_LAYOUTS];
		mark_duplicates(labels, members, count, dup);
		for (size_t m = 0; m < count; m++)
			if (dup[m] && c[members[m]].variant_tag[0])
				snprintf(labels[members[m]], 32, "%.31s", c[members[m]].variant_tag);

		mark_duplicates(labels, members, count, dup);
		int ordinal = 1;
		for (size_t m = 0; m < count; m++)
			if (dup[m])
				snprintf(labels[members[m]], 32, "%.20s%d", c[members[m]].base, ordinal++ % 100);
	}
}

static bool color_used(const uint32_t *used, size_t count, uint32_t color)
{
	for (size_t i = 0; i < count; i++)
		if (used[i] == color)
			return true;
	return false;
}

void badges_assign(const struct keymap_info *keymap, struct badge *badges)
{
	struct candidate c[MAX_LAYOUTS];
	char labels[MAX_LAYOUTS][32];
	uint32_t used[MAX_LAYOUTS];
	size_t n = keymap->count;

	for (size_t i = 0; i < n; i++)
		make_candidate(&keymap->layouts[i], &c[i]);
	resolve_labels(c, n, labels);

	for (size_t i = 0; i < n; i++) {
		uint32_t color = c[i].color;
		if (color_used(used, i, color))
			color = c[i].alt_color;
		for (size_t p = 0; p < fallback_palette_count && color_used(used, i, color); p++)
			color = fallback_palette[p];
		used[i] = color;

		struct badge *b = &badges[i];
		memset(b, 0, sizeof(*b));
		snprintf(b->label, sizeof(b->label), "%.31s", labels[i]);
		b->color = color;
		const struct layout_info *layout = &keymap->layouts[i];
		snprintf(b->title, sizeof(b->title), "%s", layout->name[0] ? layout->name : layout->code);
	}
}

static size_t utf8_length(const char *s)
{
	size_t n = 0;
	for (; *s; s++)
		n += ((unsigned char)*s & 0xc0) != 0x80;
	return n;
}

static void xml_escape(const char *src, char *dst, size_t size)
{
	size_t n = 0;
	for (; *src && n + 7 < size; src++) {
		const char *entity = *src == '&' ? "&amp;" : *src == '<' ? "&lt;" : *src == '>' ? "&gt;" : NULL;
		if (entity) {
			memcpy(dst + n, entity, strlen(entity));
			n += strlen(entity);
		} else {
			dst[n++] = *src;
		}
	}
	dst[n] = '\0';
}

int badge_svg_elements(char *buf, size_t size, const char *label, uint32_t color)
{
	size_t glyphs = utf8_length(label);
	int font_size = glyphs <= 2 ? 31 : glyphs == 3 ? 23 : 18;
	int baseline_tenths = 320 + font_size * 37 / 10;
	char text[128];
	xml_escape(label, text, sizeof(text));
	return snprintf(buf, size,
		"<rect x=\"1\" y=\"6\" width=\"62\" height=\"52\" rx=\"11\" fill=\"#%06x\"/>"
		"<text x=\"32\" y=\"%d.%d\" text-anchor=\"middle\" fill=\"#ffffff\" "
		"font-family=\"Nunito Sans, DejaVu Sans, sans-serif\" font-size=\"%d\" font-weight=\"800\">%s</text>",
		(unsigned)color & 0xffffff, baseline_tenths / 10, baseline_tenths % 10, font_size, text);
}

static int mkdir_p(char *path)
{
	for (char *p = path + 1; *p; p++) {
		if (*p != '/')
			continue;
		*p = '\0';
		int r = mkdir(path, 0755);
		*p = '/';
		if (r < 0 && errno != EEXIST)
			return -1;
	}
	return mkdir(path, 0755) < 0 && errno != EEXIST ? -1 : 0;
}

static int write_file(const char *path, const char *content)
{
	char tmp[4500];
	snprintf(tmp, sizeof(tmp), "%s.%d.tmp", path, (int)getpid());
	FILE *f = fopen(tmp, "w");
	if (!f)
		return -1;
	bool ok = fputs(content, f) >= 0;
	ok &= fclose(f) == 0;
	if (!ok || rename(tmp, path) < 0) {
		unlink(tmp);
		return -1;
	}
	return 0;
}

/*
 * wf-panel-pi looks the icon up by name in the default theme only, ignoring
 * IconThemePath, so the badges must live in the user's hicolor theme. GTK
 * notices new icons only when the theme's top directory changes, so that
 * directory's mtime is bumped whenever an icon is added.
 */
int badges_write_icons(struct badge *badges, size_t count)
{
	char theme[4096], dir[4200];
	const char *data_home = getenv("XDG_DATA_HOME");
	const char *home = getenv("HOME");
	if (data_home && *data_home)
		snprintf(theme, sizeof(theme), "%s/icons/hicolor", data_home);
	else
		snprintf(theme, sizeof(theme), "%s/.local/share/icons/hicolor", home ? home : "");
	snprintf(dir, sizeof(dir), "%s/scalable/apps", theme);
	if (mkdir_p(dir) < 0)
		return -1;

	int written = 0;
	bool failed = false;
	for (size_t i = 0; i < count; i++) {
		struct badge *b = &badges[i];
		char key[64];
		snprintf(key, sizeof(key), "%s|%06x|%d", b->label, (unsigned)b->color, DESIGN_VERSION);
		snprintf(b->icon_name, sizeof(b->icon_name), "labwc-layout-indicator-%08x", fnv1a(key));

		char path[4400];
		snprintf(path, sizeof(path), "%s/%s.svg", dir, b->icon_name);
		if (access(path, F_OK) == 0)
			continue;
		char elements[1024], svg[1280];
		badge_svg_elements(elements, sizeof(elements), b->label, b->color);
		snprintf(svg, sizeof(svg),
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"64\" height=\"64\" viewBox=\"0 0 64 64\">%s</svg>\n",
			elements);
		if (write_file(path, svg) < 0)
			failed = true;
		else
			written++;
	}
	if (written > 0)
		utimensat(AT_FDCWD, theme, NULL, 0);
	return failed ? -1 : written;
}
