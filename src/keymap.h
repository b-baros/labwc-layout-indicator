#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <xkbcommon/xkbcommon.h>

#define MAX_LAYOUTS 32
#define MAX_TOGGLE_MODIFIERS 4

struct layout_info {
	char name[128];
	char code[32];
	char variant[64];
	bool native_script;
};

/* A key (plus modifiers to hold) that advances to the next layout; evdev codes. */
struct toggle_combo {
	int key;
	int modifiers[MAX_TOGGLE_MODIFIERS];
	int modifier_count;
};

struct keymap_info {
	struct layout_info layouts[MAX_LAYOUTS];
	size_t count;
	bool has_toggle;
	struct toggle_combo toggle;
};

bool keymap_info_parse(struct xkb_context *ctx, const char *text, struct keymap_info *out);

/* Layout codes and variants from the keymap's xkb_symbols name, indexed by layout. */
size_t keymap_parse_symbols_name(const char *symbols, char codes[][32], char variants[][64], size_t max);

/* The keymap labwc applies, compiled from XKB_DEFAULT_* in its environment file. Caller frees. */
char *keymap_compile_from_labwc_env(void);

#endif
