#include <linux/input-event-codes.h>
#include <stdlib.h>

#include "check.h"
#include "keymap.h"

static struct xkb_context *ctx;

static void test_symbols_name(void)
{
	char codes[MAX_LAYOUTS][32], variants[MAX_LAYOUTS][64];

	size_t n = keymap_parse_symbols_name(
		"pc_us_rs(latinyz)_2_rs(yz)_3_inet(evdev)_group(toggle)_1_group(toggle)_2_group(toggle)_3",
		codes, variants, MAX_LAYOUTS);
	CHECK(n == 3);
	CHECK_STR(codes[0], "us");
	CHECK_STR(variants[0], "");
	CHECK_STR(codes[1], "rs");
	CHECK_STR(variants[1], "latinyz");
	CHECK_STR(codes[2], "rs");
	CHECK_STR(variants[2], "yz");

	n = keymap_parse_symbols_name("pc+de(nodeadkeys)+us:2+inet(evdev)", codes, variants, MAX_LAYOUTS);
	CHECK(n == 2);
	CHECK_STR(codes[0], "de");
	CHECK_STR(variants[0], "nodeadkeys");
	CHECK_STR(codes[1], "us");

	n = keymap_parse_symbols_name("pc_us_ru(phonetic_winkeys)_2_inet(evdev)_srvr_ctrl(fkey2vt)",
		codes, variants, MAX_LAYOUTS);
	CHECK(n == 2);
	CHECK_STR(codes[1], "ru");
	CHECK_STR(variants[1], "phonetic_winkeys");

	n = keymap_parse_symbols_name("pc_gb_inet(evdev)", codes, variants, MAX_LAYOUTS);
	CHECK(n == 1);
	CHECK_STR(codes[0], "gb");
}

static char *compile(const char *layout, const char *variant, const char *options)
{
	struct xkb_rule_names names = {
		.rules = "evdev", .model = "pc105", .layout = layout, .variant = variant, .options = options,
	};
	struct xkb_keymap *keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!keymap)
		return NULL;
	char *text = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
	xkb_keymap_unref(keymap);
	return text;
}

/* libxkbcommon before 1.12 writes "xkb_symbols {" without the name that carries the codes. */
static void remove_symbols_name(char *text)
{
	char *name = strstr(text, "xkb_symbols \"");
	if (!name)
		return;
	name += strlen("xkb_symbols ");
	char *end = strchr(name + 1, '"');
	if (end)
		memmove(name, end + 1, strlen(end + 1) + 1);
}

/* Keymaps without inet(evdev) have no dedicated layout key. */
static void remove_layout_key(char *text)
{
	char *line = strstr(text, "key <I592>");
	if (!line)
		return;
	char *end = strchr(line, '\n');
	memmove(line, end ? end + 1 : line + strlen(line), strlen(end ? end + 1 : "") + 1);
}

static void test_serbian(void)
{
	struct keymap_info info;
	char *text = compile("us,rs,rs", ",latinyz,yz", "grp:toggle");
	CHECK(text && keymap_info_parse(ctx, text, &info));
	CHECK(info.count == 3);
	CHECK_STR(info.layouts[0].code, "us");
	CHECK_STR(info.layouts[1].code, "rs");
	CHECK_STR(info.layouts[1].variant, "latinyz");
	CHECK_STR(info.layouts[2].variant, "yz");
	CHECK_STR(info.layouts[1].name, "Serbian (Latin, QWERTY)");
	CHECK(!info.layouts[0].native_script);
	CHECK(!info.layouts[1].native_script);
	CHECK(info.layouts[2].native_script);
	CHECK(info.has_toggle);
	CHECK(info.toggle.key == KEY_KBD_LAYOUT_NEXT);
	CHECK(info.toggle.modifier_count == 0);

	remove_symbols_name(text);
	CHECK(strstr(text, "xkb_symbols \"") == NULL);
	CHECK(keymap_info_parse(ctx, text, &info));
	CHECK(info.count == 3);
	CHECK_STR(info.layouts[0].code, "us");
	CHECK_STR(info.layouts[1].code, "rs");
	CHECK_STR(info.layouts[1].variant, "latinyz");
	CHECK_STR(info.layouts[2].code, "rs");
	CHECK_STR(info.layouts[2].variant, "yz");

	remove_layout_key(text);
	CHECK(keymap_info_parse(ctx, text, &info));
	CHECK(info.has_toggle);
	CHECK(info.toggle.key == KEY_RIGHTALT);
	CHECK(info.toggle.modifier_count == 0);
	free(text);
}

static void test_toggle_with_modifier(void)
{
	struct keymap_info info;
	char *text = compile("us,de", "", "grp:alt_shift_toggle");
	CHECK(text != NULL);
	remove_layout_key(text);
	CHECK(keymap_info_parse(ctx, text, &info));
	CHECK(info.has_toggle);
	CHECK(info.toggle.modifier_count == 1);
	free(text);

	text = compile("us,de", "", "");
	CHECK(text != NULL);
	remove_layout_key(text);
	CHECK(keymap_info_parse(ctx, text, &info));
	CHECK(!info.has_toggle);
	free(text);
}

static void test_scripts(void)
{
	struct keymap_info info;
	char *text = compile("ru,gr,il,de", "", "");
	CHECK(text && keymap_info_parse(ctx, text, &info));
	CHECK(info.count == 4);
	CHECK(info.layouts[0].native_script);
	CHECK(info.layouts[1].native_script);
	CHECK(info.layouts[2].native_script);
	CHECK(!info.layouts[3].native_script);
	free(text);
}

int main(void)
{
	ctx = xkb_context_new(XKB_CONTEXT_NO_ENVIRONMENT_NAMES);
	test_symbols_name();
	test_serbian();
	test_toggle_with_modifier();
	test_scripts();
	xkb_context_unref(ctx);
	return failures ? 1 : 0;
}
