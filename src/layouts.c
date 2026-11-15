#include <stdlib.h>
#include <string.h>

#include "layouts.h"

/*
 * Colours come from each language's flag, darkened where needed so white text
 * keeps a contrast of at least 4.5:1 (see tools/mute-color.py). The alternative
 * colour comes from the country's own flag and is used when two active layouts
 * would otherwise share a colour. Sorted by code.
 */
const struct layout_style layout_styles[] = {
	{ "af",     "FA",   0x007a36, "دری",       0x007a36, NULL,   0xc34030 },
	{ "al",     "SQ",   0xc44037, NULL,        0xc44037, NULL,   0x3a3a3a },
	{ "am",     "HY",   0x06369a, "ՀԱՅ",       0x06369a, NULL,   0x996a0c },
	{ "ara",    "AR",   0x007a3d, "ع",         0x007a3d, "ARA",  0xbe3939 },
	{ "at",     "DE",   0x8d710f, NULL,        0x8d710f, NULL,   0xba343c },
	{ "au",     "EN",   0x4a4a4a, NULL,        0x4a4a4a, NULL,   0x032187 },
	{ "az",     "AZ",   0x137f9e, "АЗ",        0x137f9e, NULL,   0xc43f41 },
	{ "ba",     "BS",   0x04288d, "БС",        0x04288d, NULL,   0x8d710f },
	{ "bd",     "BN",   0x006a4e, NULL,        0x006a4e, NULL,   0xc43e44 },
	{ "be",     "BE",   0xc43f40, NULL,        0xc43f40, NULL,   0x85750a },
	{ "bg",     "BG",   0x118461, "БГ",        0x118461, NULL,   0xc44130 },
	{ "br",     "PT",   0x046a38, NULL,        0x046a38, NULL,   0x082b74 },
	{ "brai",   "BRL",  0x3c4a5c, NULL,        0x3c4a5c, "BRL",  0x5a4a6b },
	{ "bt",     "DZ",   0xc3441f, NULL,        0xc3441f, NULL,   0x8a7210 },
	{ "bw",     "TN",   0x4579a7, NULL,        0x4579a7, NULL,   0x56606e },
	{ "by",     "BE",   0xc23c43, "БЕЛ",       0xc23c43, NULL,   0x258538 },
	{ "ca",     "FR",   0x04288d, NULL,        0x04288d, NULL,   0xc33e3b },
	{ "cd",     "FR",   0x04288d, NULL,        0x04288d, NULL,   0x2272d5 },
	{ "ch",     "DE",   0x8d710f, NULL,        0x8d710f, NULL,   0xc44133 },
	{ "cm",     "EN",   0x4a4a4a, NULL,        0x4a4a4a, NULL,   0x007a5e },
	{ "cn",     "ZH",   0xc44039, NULL,        0xc44039, NULL,   0x867403 },
	{ "cz",     "CS",   0x11457e, NULL,        0x11457e, NULL,   0xc43f37 },
	{ "de",     "DE",   0x8d710f, NULL,        0x8d710f, NULL,   0xc44133 },
	{ "dk",     "DA",   0xba343c, NULL,        0xba343c, NULL,   0x56606e },
	{ "dz",     "KAB",  0x006233, NULL,        0x006233, NULL,   0xc13b43 },
	{ "ee",     "ET",   0x0072ce, NULL,        0x0072ce, NULL,   0x56606e },
	{ "eg",     "AR",   0x007a3d, "ع",         0x007a3d, NULL,   0x916f0e },
	{ "epo",    "EO",   0x1d871a, NULL,        0x1d871a, NULL,   0x56606e },
	{ "es",     "ES",   0xa62021, NULL,        0xa62021, NULL,   0x8f7002 },
	{ "et",     "AM",   0x02872e, NULL,        0x02872e, NULL,   0xc44038 },
	{ "fi",     "FI",   0x002f6c, NULL,        0x002f6c, NULL,   0x56606e },
	{ "fo",     "FO",   0xc43f40, NULL,        0xc43f40, NULL,   0x0065bd },
	{ "fr",     "FR",   0x04288d, NULL,        0x04288d, NULL,   0xc43f40 },
	{ "gb",     "EN",   0x4a4a4a, NULL,        0x4a4a4a, "UK",   0x082b74 },
	{ "ge",     "KA",   0xc44133, "ქა",        0xc44133, NULL,   0x56606e },
	{ "gh",     "EN",   0x4a4a4a, NULL,        0x4a4a4a, NULL,   0x006b3f },
	{ "gn",     "NKO",  0x018556, NULL,        0x018556, NULL,   0xbe3939 },
	{ "gr",     "EL",   0x0d5eaf, "ΕΛ",        0x0d5eaf, NULL,   0x56606e },
	{ "hr",     "HR",   0xc44133, NULL,        0xc44133, NULL,   0x18218b },
	{ "hu",     "HU",   0x477050, NULL,        0x477050, NULL,   0xc33d43 },
	{ "id",     "ID",   0xbe3939, NULL,        0xbe3939, NULL,   0x56606e },
	{ "ie",     "GA",   0x018652, NULL,        0x018652, NULL,   0xb65405 },
	{ "il",     "HE",   0x1340a5, "עב",        0x1340a5, NULL,   0x56606e },
	{ "in",     "IN",   0xa95f09, NULL,        0xa95f09, NULL,   0x208718 },
	{ "iq",     "AR",   0x007a3d, "ع",         0x007a3d, NULL,   0xbe3939 },
	{ "ir",     "FA",   0x0a8730, "فا",        0x0a8730, NULL,   0xc44033 },
	{ "is",     "IS",   0x02529c, NULL,        0x02529c, NULL,   0xc43f43 },
	{ "it",     "IT",   0x048741, NULL,        0x048741, NULL,   0xc33e41 },
	{ "jp",     "JA",   0xb02a37, NULL,        0xb02a37, NULL,   0x56606e },
	{ "jv",     "JV",   0xbe3939, NULL,        0xbe3939, NULL,   0x56606e },
	{ "ke",     "SW",   0x006600, NULL,        0x006600, NULL,   0xaf2b20 },
	{ "kg",     "KY",   0xc43f3f, "КЫР",       0xc43f3f, NULL,   0x7f770d },
	{ "kh",     "KM",   0x0a3398, NULL,        0x0a3398, NULL,   0xc43f3e },
	{ "kr",     "KO",   0x003478, NULL,        0x003478, NULL,   0xc43e42 },
	{ "kz",     "KK",   0x157f92, "ҚАЗ",       0x157f92, NULL,   0x906f0f },
	{ "la",     "LO",   0x052e6f, NULL,        0x052e6f, NULL,   0xbe3939 },
	{ "latam",  "ES",   0xa62021, NULL,        0xa62021, "LAT",  0x2a6f6a },
	{ "lk",     "SI",   0x8d153a, NULL,        0x8d153a, NULL,   0x005f56 },
	{ "lt",     "LT",   0x006a44, NULL,        0x006a44, NULL,   0xba3535 },
	{ "lv",     "LV",   0x9e3039, NULL,        0x9e3039, NULL,   0x56606e },
	{ "ma",     "AR",   0x007a3d, "ع",         0x007a3d, NULL,   0xba3535 },
	{ "mao",    "MI",   0xc44133, NULL,        0xc44133, NULL,   0x3a3a3a },
	{ "md",     "RO",   0x002b7f, NULL,        0x002b7f, NULL,   0xbc363e },
	{ "me",     "CG",   0xb53128, "ЦГ",        0xb53128, NULL,   0x8d710e },
	{ "mk",     "MK",   0xbe3b2e, "МК",        0xbe3b2e, NULL,   0x837607 },
	{ "ml",     "BM",   0x078627, NULL,        0x078627, NULL,   0xbe3939 },
	{ "mm",     "MY",   0x1b871c, NULL,        0x1b871c, NULL,   0xc43f41 },
	{ "mn",     "MN",   0xc43f41, "МОН",       0xc43f41, NULL,   0x015197 },
	{ "mt",     "MT",   0xbf3a3c, NULL,        0xbf3a3c, NULL,   0x56606e },
	{ "mv",     "DV",   0x007e3a, NULL,        0x007e3a, NULL,   0xc13b43 },
	{ "my",     "MS",   0x072482, NULL,        0x072482, NULL,   0xba372b },
	{ "ng",     "EN",   0x4a4a4a, NULL,        0x4a4a4a, NULL,   0x0e8551 },
	{ "nl",     "NL",   0xbd4d06, NULL,        0xbd4d06, NULL,   0x21468b },
	{ "no",     "NO",   0xb02a38, NULL,        0xb02a38, NULL,   0x0c2f6b },
	{ "np",     "NE",   0xc43e49, NULL,        0xc43e49, NULL,   0x003893 },
	{ "nz",     "EN",   0x4a4a4a, NULL,        0x4a4a4a, NULL,   0x01267f },
	{ "ph",     "FIL",  0x093ba0, NULL,        0x093ba0, NULL,   0xbe3939 },
	{ "pk",     "UR",   0x01411c, "اردو",      0x01411c, NULL,   0x56606e },
	{ "pl",     "PL",   0xc43e49, NULL,        0xc43e49, NULL,   0x56606e },
	{ "pt",     "PT",   0x046a38, NULL,        0x046a38, NULL,   0xc44133 },
	{ "ro",     "RO",   0x002b7f, NULL,        0x002b7f, NULL,   0xbe3939 },
	{ "rs",     "SR",   0x0c4076, "СР",        0xc6363c, NULL,   0x8f700e },
	{ "ru",     "RU",   0x083c9f, "РУ",        0x083c9f, NULL,   0xc44133 },
	{ "se",     "SV",   0x006aa7, NULL,        0x006aa7, NULL,   0x8d710f },
	{ "si",     "SL",   0x005da4, NULL,        0x005da4, NULL,   0xc44039 },
	{ "sk",     "SK",   0x0b4ea2, NULL,        0x0b4ea2, NULL,   0xc44039 },
	{ "sn",     "WO",   0x00853f, NULL,        0x00853f, NULL,   0xc44039 },
	{ "sy",     "AR",   0x007a3d, "ع",         0x007a3d, NULL,   0xbe3939 },
	{ "tg",     "FR",   0x04288d, NULL,        0x04288d, NULL,   0x006a4e },
	{ "th",     "TH",   0x312e4e, NULL,        0x312e4e, NULL,   0xa41b32 },
	{ "tj",     "TG",   0x006600, "ТОҶ",       0x006600, NULL,   0xba372b },
	{ "tm",     "TK",   0x00843d, NULL,        0x00843d, NULL,   0xc43f3f },
	{ "tr",     "TR",   0xc44037, NULL,        0xc44037, NULL,   0x56606e },
	{ "tw",     "ZH",   0xc44039, NULL,        0xc44039, NULL,   0x032187 },
	{ "tz",     "SW",   0x006600, NULL,        0x006600, NULL,   0x037daa },
	{ "ua",     "UK",   0x0057b7, "УКР",       0x0057b7, NULL,   0x897300 },
	{ "us",     "EN",   0x4a4a4a, NULL,        0x4a4a4a, NULL,   0xae2745 },
	{ "uz",     "UZ",   0x098098, "ЎЗ",        0x098098, NULL,   0x038824 },
	{ "vn",     "VI",   0xc44134, NULL,        0xc44134, NULL,   0x787803 },
	{ "za",     "EN",   0x4a4a4a, NULL,        0x4a4a4a, NULL,   0x007749 },
};

const size_t layout_styles_count = sizeof(layout_styles) / sizeof(layout_styles[0]);

const uint32_t fallback_palette[] = {
	0x3d5a80, 0x2a6f6a, 0x6b4c7a, 0x5f6b2a, 0x9a3b2e, 0x8a5a14, 0x46607a, 0x8c3b5a,
};

const size_t fallback_palette_count = sizeof(fallback_palette) / sizeof(fallback_palette[0]);

static int compare_code(const void *key, const void *element)
{
	return strcmp(key, ((const struct layout_style *)element)->code);
}

const struct layout_style *layout_style_find(const char *code)
{
	if (!code || !*code)
		return NULL;
	return bsearch(code, layout_styles, layout_styles_count, sizeof(layout_styles[0]), compare_code);
}
