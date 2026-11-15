#include <math.h>

#include "badges.h"
#include "check.h"
#include "layouts.h"

static double linear(uint32_t color, int shift)
{
	double v = ((color >> shift) & 0xff) / 255.0;
	return v <= 0.04045 ? v / 12.92 : pow((v + 0.055) / 1.055, 2.4);
}

static double contrast_with_white(uint32_t color)
{
	double l = 0.2126 * linear(color, 16) + 0.7152 * linear(color, 8) + 0.0722 * linear(color, 0);
	return 1.05 / (l + 0.05);
}

static void check_contrast(const char *code, uint32_t color)
{
	if (contrast_with_white(color) < 4.5) {
		fprintf(stderr, "%s: #%06x has contrast %.2f with white\n", code, (unsigned)color,
			contrast_with_white(color));
		failures++;
	}
}

static void test_table(void)
{
	for (size_t i = 0; i < layout_styles_count; i++) {
		const struct layout_style *s = &layout_styles[i];
		check_contrast(s->code, s->color);
		check_contrast(s->code, s->native_color);
		check_contrast(s->code, s->alt_color);
		CHECK(s->label && *s->label);
		if (i > 0 && strcmp(layout_styles[i - 1].code, s->code) >= 0) {
			fprintf(stderr, "table not sorted at %s\n", s->code);
			failures++;
		}
		CHECK(layout_style_find(s->code) == s);
	}
	for (size_t i = 0; i < fallback_palette_count; i++)
		check_contrast("palette", fallback_palette[i]);
	CHECK(layout_style_find("nonexistent") == NULL);
}

static void add(struct keymap_info *info, const char *code, const char *variant, bool native)
{
	struct layout_info *l = &info->layouts[info->count++];
	memset(l, 0, sizeof(*l));
	snprintf(l->code, sizeof(l->code), "%s", code);
	snprintf(l->variant, sizeof(l->variant), "%s", variant);
	snprintf(l->name, sizeof(l->name), "%s(%s)", code, variant);
	l->native_script = native;
}

static void test_serbian(void)
{
	struct keymap_info info = { 0 };
	struct badge b[MAX_LAYOUTS];
	add(&info, "us", "", false);
	add(&info, "rs", "latinyz", false);
	add(&info, "rs", "yz", true);
	badges_assign(&info, b);
	CHECK_STR(b[0].label, "EN");
	CHECK(b[0].color == 0x4a4a4a);
	CHECK_STR(b[1].label, "SR");
	CHECK(b[1].color == 0x0c4076);
	CHECK_STR(b[2].label, "СР");
	CHECK(b[2].color == 0xc6363c);
}

static void test_collisions(void)
{
	struct keymap_info info = { 0 };
	struct badge b[MAX_LAYOUTS];

	add(&info, "us", "", false);
	add(&info, "gb", "", false);
	badges_assign(&info, b);
	CHECK_STR(b[0].label, "US");
	CHECK_STR(b[1].label, "UK");
	CHECK(b[0].color != b[1].color);

	info.count = 0;
	add(&info, "us", "", false);
	add(&info, "us", "dvorak", false);
	badges_assign(&info, b);
	CHECK_STR(b[0].label, "EN");
	CHECK_STR(b[1].label, "DV");

	info.count = 0;
	add(&info, "us", "", false);
	add(&info, "gb", "", false);
	add(&info, "us", "colemak", false);
	badges_assign(&info, b);
	CHECK_STR(b[0].label, "US");
	CHECK_STR(b[1].label, "UK");
	CHECK_STR(b[2].label, "CM");
	CHECK(b[0].color != b[1].color && b[1].color != b[2].color && b[0].color != b[2].color);

	info.count = 0;
	add(&info, "rs", "latin", false);
	add(&info, "rs", "latinyz", false);
	badges_assign(&info, b);
	CHECK_STR(b[0].label, "SR1");
	CHECK_STR(b[1].label, "SR2");
}

static void test_fallbacks(void)
{
	struct keymap_info info = { 0 };
	struct badge b[MAX_LAYOUTS];
	add(&info, "xyz", "", false);
	add(&info, "ru", "", true);
	add(&info, "jp", "", false);
	badges_assign(&info, b);
	CHECK_STR(b[0].label, "XYZ");
	bool in_palette = false;
	for (size_t i = 0; i < fallback_palette_count; i++)
		in_palette |= b[0].color == fallback_palette[i];
	CHECK(in_palette);
	CHECK_STR(b[1].label, "РУ");
	CHECK_STR(b[2].label, "JA");

	info.count = 0;
	struct layout_info *unknown = &info.layouts[info.count++];
	memset(unknown, 0, sizeof(*unknown));
	badges_assign(&info, b);
	CHECK_STR(b[0].label, "?");
}

static void test_svg(void)
{
	char buf[1024];
	badge_svg_elements(buf, sizeof(buf), "A&B", 0x0c4076);
	CHECK(strstr(buf, "A&amp;B") != NULL);
	CHECK(strstr(buf, "#0c4076") != NULL);
	badge_svg_elements(buf, sizeof(buf), "БЕЛ", 0xc8313e);
	CHECK(strstr(buf, "font-size=\"23\"") != NULL);
}

int main(void)
{
	test_table();
	test_serbian();
	test_collisions();
	test_fallbacks();
	test_svg();
	return failures ? 1 : 0;
}
