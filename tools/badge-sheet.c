/* Prints an SVG overview of every predefined badge: badge-sheet > docs/badges.svg */
#include <stdio.h>

#include "badges.h"
#include "layouts.h"

#define COLUMNS 12
#define CELL_W 84
#define CELL_H 92

static void cell(int index, const char *label, uint32_t color, const char *caption)
{
	char elements[1024];
	badge_svg_elements(elements, sizeof(elements), label, color);
	int x = (index % COLUMNS) * CELL_W + 10, y = (index / COLUMNS) * CELL_H + 10;
	printf("<g transform=\"translate(%d %d)\">%s"
		"<text x=\"32\" y=\"80\" text-anchor=\"middle\" fill=\"#555555\" "
		"font-family=\"DejaVu Sans Mono, monospace\" font-size=\"12\">%s</text></g>\n",
		x, y, elements, caption);
}

int main(void)
{
	int cells = 0;
	for (size_t i = 0; i < layout_styles_count; i++)
		cells += layout_styles[i].native_label ? 2 : 1;
	int rows = (cells + COLUMNS - 1) / COLUMNS;
	int width = COLUMNS * CELL_W + 20, height = rows * CELL_H + 20;

	printf("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n",
		width, height, width, height);
	printf("<rect width=\"100%%\" height=\"100%%\" fill=\"#f6f6f6\"/>\n");
	int index = 0;
	for (size_t i = 0; i < layout_styles_count; i++) {
		const struct layout_style *s = &layout_styles[i];
		cell(index++, s->label, s->color, s->code);
		if (s->native_label) {
			char caption[48];
			snprintf(caption, sizeof(caption), "%s \xc2\xb7 native", s->code);
			cell(index++, s->native_label, s->native_color, caption);
		}
	}
	puts("</svg>");
	return 0;
}
