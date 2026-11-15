#include <ctype.h>
#include <linux/input-event-codes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "keymap.h"

#define EVDEV_OFFSET 8

static void copy_string(char *dst, size_t size, const char *src, size_t len)
{
	if (len >= size)
		len = size - 1;
	memcpy(dst, src, len);
	dst[len] = '\0';
}

/* Symbols files that come before or after the layouts in an xkb_symbols name. */
static bool is_model_or_option(const char *token, size_t len)
{
	static const char *const names[] = {
		"altwin", "capslock", "compose", "ctrl", "eurosign", "group", "inet", "keypad",
		"kpdl", "level3", "level5", "nbsp", "parens", "pc", "rupeesign", "shift", "srvr",
		"terminate", "typo",
	};
	if (memchr(token, '/', len))
		return true;
	const char *paren = memchr(token, '(', len);
	size_t base = paren ? (size_t)(paren - token) : len;
	for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++)
		if (strlen(names[i]) == base && strncmp(token, names[i], base) == 0)
			return true;
	return false;
}

/*
 * The name looks like "pc+us+rs(latinyz):2+inet(evdev)", or with '+' and ':'
 * serialised as '_' ("pc_us_rs(latinyz)_2_inet(evdev)"). Variant names may
 * contain '_' themselves, so separators inside parentheses are ignored.
 */
size_t keymap_parse_symbols_name(const char *symbols, char codes[][32], char variants[][64], size_t max)
{
	size_t found = 0;
	for (size_t i = 0; i < max; i++)
		codes[i][0] = variants[i][0] = '\0';

	const char *p = symbols;
	while (*p) {
		const char *start = p;
		int depth = 0;
		while (*p && !(depth == 0 && (*p == '+' || *p == '_' || *p == ':'))) {
			if (*p == '(')
				depth++;
			else if (*p == ')' && depth > 0)
				depth--;
			p++;
		}
		size_t len = (size_t)(p - start);

		unsigned long group = 1;
		if (*p == ':' || *p == '_') {
			const char *digits = p + 1, *q = digits;
			while (isdigit((unsigned char)*q))
				q++;
			if (q > digits && (*q == '\0' || *q == '+' || *q == '_')) {
				group = strtoul(digits, NULL, 10);
				p = q;
			}
		}
		if (*p)
			p++;

		if (len == 0 || group == 0 || group > max || is_model_or_option(start, len))
			continue;
		size_t index = group - 1;
		if (codes[index][0])
			continue;
		const char *paren = memchr(start, '(', len);
		size_t code_len = paren ? (size_t)(paren - start) : len;
		copy_string(codes[index], sizeof(codes[index]), start, code_len);
		if (paren) {
			const char *close = memchr(paren, ')', len - code_len);
			size_t variant_len = close ? (size_t)(close - paren - 1) : len - code_len - 1;
			copy_string(variants[index], sizeof(variants[index]), paren + 1, variant_len);
		}
		if (index + 1 > found)
			found = index + 1;
	}
	return found;
}

static bool is_latin(uint32_t cp)
{
	return cp < 0x0250 || (cp >= 0x1e00 && cp < 0x1f00) || (cp >= 0x2c60 && cp < 0x2c80) ||
		(cp >= 0xa720 && cp < 0xa800);
}

static bool types_native_script(struct xkb_keymap *keymap, xkb_layout_index_t layout)
{
	xkb_keycode_t key = xkb_keymap_key_by_name(keymap, "AC01");
	if (key == XKB_KEYCODE_INVALID)
		return false;
	xkb_layout_index_t layouts = xkb_keymap_num_layouts_for_key(keymap, key);
	if (layouts == 0)
		return false;
	const xkb_keysym_t *syms;
	int n = xkb_keymap_key_get_syms_by_level(keymap, key, layout % layouts, 0, &syms);
	if (n <= 0)
		return false;
	uint32_t cp = xkb_keysym_to_utf32(syms[0]);
	return cp != 0 && !is_latin(cp);
}

static bool add_modifier_keys(struct xkb_keymap *keymap, xkb_mod_mask_t mask, struct toggle_combo *combo)
{
	static const struct { const char *name; int key; } map[] = {
		{ XKB_MOD_NAME_SHIFT, KEY_LEFTSHIFT }, { XKB_MOD_NAME_CTRL, KEY_LEFTCTRL },
		{ XKB_MOD_NAME_ALT, KEY_LEFTALT }, { "Alt", KEY_LEFTALT },
		{ XKB_MOD_NAME_LOGO, KEY_LEFTMETA }, { "Super", KEY_LEFTMETA },
	};
	for (xkb_mod_index_t i = 0; i < xkb_keymap_num_mods(keymap) && i < 32; i++) {
		if (!(mask & (1u << i)))
			continue;
		const char *name = xkb_keymap_mod_get_name(keymap, i);
		int key = -1;
		for (size_t j = 0; name && j < sizeof(map) / sizeof(map[0]); j++)
			if (strcmp(name, map[j].name) == 0)
				key = map[j].key;
		if (key < 0 || key == combo->key || combo->modifier_count == MAX_TOGGLE_MODIFIERS)
			return false;
		combo->modifiers[combo->modifier_count++] = key;
	}
	return combo->modifier_count > 0;
}

struct toggle_search {
	int rank; /* 0 none, 1 key with modifiers, 2 single key, 3 KEY_KBD_LAYOUT_NEXT */
	struct toggle_combo combo;
};

static void find_toggle(struct xkb_keymap *keymap, xkb_keycode_t key, void *data)
{
	struct toggle_search *search = data;
	if (search->rank == 3 || xkb_keymap_num_layouts_for_key(keymap, key) == 0)
		return;
	xkb_level_index_t levels = xkb_keymap_num_levels_for_key(keymap, key, 0);
	for (xkb_level_index_t level = 0; level < levels; level++) {
		const xkb_keysym_t *syms;
		int n = xkb_keymap_key_get_syms_by_level(keymap, key, 0, level, &syms);
		bool next_group = false;
		for (int i = 0; i < n; i++)
			next_group |= syms[i] == XKB_KEY_ISO_Next_Group;
		if (!next_group)
			continue;

		struct toggle_combo combo = { .key = (int)key - EVDEV_OFFSET };
		int rank = level > 0 ? 1 : combo.key == KEY_KBD_LAYOUT_NEXT ? 3 : 2;
		if (level > 0) {
			xkb_mod_mask_t masks[4];
			size_t count = xkb_keymap_key_get_mods_for_level(keymap, key, 0, level, masks, 4);
			if (count == 0 || !add_modifier_keys(keymap, masks[0], &combo))
				continue;
		}
		if (rank > search->rank) {
			search->rank = rank;
			search->combo = combo;
		}
		return;
	}
}

bool keymap_info_parse(struct xkb_context *ctx, const char *text, struct keymap_info *out)
{
	struct xkb_keymap *keymap = xkb_keymap_new_from_string(ctx, text, XKB_KEYMAP_FORMAT_TEXT_V1,
		XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!keymap)
		return false;

	memset(out, 0, sizeof(*out));
	out->count = xkb_keymap_num_layouts(keymap);
	if (out->count > MAX_LAYOUTS)
		out->count = MAX_LAYOUTS;

	char codes[MAX_LAYOUTS][32], variants[MAX_LAYOUTS][64];
	const char *symbols = strstr(text, "xkb_symbols \"");
	if (symbols) {
		symbols += strlen("xkb_symbols \"");
		const char *end = strchr(symbols, '"');
		char name[1024];
		copy_string(name, sizeof(name), symbols, end ? (size_t)(end - symbols) : strlen(symbols));
		keymap_parse_symbols_name(name, codes, variants, MAX_LAYOUTS);
	} else {
		for (size_t i = 0; i < MAX_LAYOUTS; i++)
			codes[i][0] = variants[i][0] = '\0';
	}

	for (size_t i = 0; i < out->count; i++) {
		struct layout_info *layout = &out->layouts[i];
		const char *name = xkb_keymap_layout_get_name(keymap, (xkb_layout_index_t)i);
		copy_string(layout->name, sizeof(layout->name), name ? name : "", name ? strlen(name) : 0);
		copy_string(layout->code, sizeof(layout->code), codes[i], strlen(codes[i]));
		copy_string(layout->variant, sizeof(layout->variant), variants[i], strlen(variants[i]));
		layout->native_script = types_native_script(keymap, (xkb_layout_index_t)i);
	}

	struct toggle_search search = { 0 };
	xkb_keymap_key_for_each(keymap, find_toggle, &search);
	out->has_toggle = search.rank > 0;
	out->toggle = search.combo;

	xkb_keymap_unref(keymap);
	return true;
}

static char *trim(char *s)
{
	while (isspace((unsigned char)*s))
		s++;
	char *end = s + strlen(s);
	while (end > s && isspace((unsigned char)end[-1]))
		*--end = '\0';
	size_t len = strlen(s);
	if (len >= 2 && (s[0] == '"' || s[0] == '\'') && s[len - 1] == s[0]) {
		s[len - 1] = '\0';
		s++;
	}
	return s;
}

static const char *const env_keys[5] = {
	"XKB_DEFAULT_RULES", "XKB_DEFAULT_MODEL", "XKB_DEFAULT_LAYOUT",
	"XKB_DEFAULT_VARIANT", "XKB_DEFAULT_OPTIONS",
};

static bool read_labwc_env(const char *path, char values[5][256])
{
	FILE *f = fopen(path, "r");
	if (!f)
		return false;
	bool found = false;
	char line[512];
	while (fgets(line, sizeof(line), f)) {
		char *s = trim(line);
		char *eq = strchr(s, '=');
		if (*s == '#' || !eq)
			continue;
		*eq = '\0';
		char *key = trim(s), *value = trim(eq + 1);
		for (size_t i = 0; i < 5; i++) {
			if (strcmp(key, env_keys[i]) == 0) {
				snprintf(values[i], 256, "%s", value);
				found = true;
			}
		}
	}
	fclose(f);
	return found;
}

/*
 * labwc re-reads its environment file on --reconfigure, so the file is more
 * current than our own environment, which may predate a layout change; the
 * latter is used only when no file sets XKB_DEFAULT_*.
 */
char *keymap_compile_from_labwc_env(void)
{
	char values[5][256] = { { 0 } };
	char path[4096];
	const char *config_home = getenv("XDG_CONFIG_HOME");
	const char *home = getenv("HOME");
	bool found = false;

	if (config_home && *config_home)
		snprintf(path, sizeof(path), "%s/labwc/environment", config_home);
	else
		snprintf(path, sizeof(path), "%s/.config/labwc/environment", home ? home : "");
	found = read_labwc_env(path, values);

	const char *dirs = getenv("XDG_CONFIG_DIRS");
	char list[4096];
	snprintf(list, sizeof(list), "%s", dirs && *dirs ? dirs : "/etc/xdg");
	for (char *save = NULL, *dir = strtok_r(list, ":", &save); dir && !found; dir = strtok_r(NULL, ":", &save)) {
		snprintf(path, sizeof(path), "%s/labwc/environment", dir);
		found = read_labwc_env(path, values);
	}

	for (size_t i = 0; i < 5 && !found; i++) {
		const char *value = getenv(env_keys[i]);
		if (value)
			snprintf(values[i], sizeof(values[i]), "%s", value);
	}

	struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_ENVIRONMENT_NAMES);
	if (!ctx)
		return NULL;
	struct xkb_rule_names names = {
		.rules = values[0][0] ? values[0] : NULL,
		.model = values[1][0] ? values[1] : NULL,
		.layout = values[2][0] ? values[2] : NULL,
		.variant = values[3][0] ? values[3] : NULL,
		.options = values[4][0] ? values[4] : NULL,
	};
	struct xkb_keymap *keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
	char *text = keymap ? xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1) : NULL;
	xkb_keymap_unref(keymap);
	xkb_context_unref(ctx);
	return text;
}
