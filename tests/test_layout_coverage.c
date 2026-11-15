#include <stdlib.h>

#include "check.h"
#include "layouts.h"

/* Every base layout xkeyboard-config ships should have a hand-picked badge. */
int main(void)
{
	FILE *f = fopen(EVDEV_XML, "r");
	if (!f) {
		fprintf(stderr, "%s not found, skipping\n", EVDEV_XML);
		return 77;
	}
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);
	char *xml = malloc((size_t)size + 1);
	if (!xml || fread(xml, 1, (size_t)size, f) != (size_t)size)
		return 1;
	xml[size] = '\0';
	fclose(f);

	size_t checked = 0;
	const char *p = strstr(xml, "<layoutList>");
	while (p && (p = strstr(p, "<layout")) != NULL) {
		p += strlen("<layout");
		if (*p != '>' && *p != ' ')
			continue;
		const char *name = strstr(p, "<name>");
		const char *end = name ? strstr(name, "</name>") : NULL;
		if (!end)
			break;
		name += strlen("<name>");
		char code[64];
		snprintf(code, sizeof(code), "%.*s", (int)(end - name), name);
		if (strcmp(code, "custom") != 0) {
			if (!layout_style_find(code)) {
				fprintf(stderr, "no badge for layout \"%s\"\n", code);
				failures++;
			}
			checked++;
		}
		p = end;
	}
	free(xml);
	CHECK(checked > 50);
	printf("%zu layouts checked\n", checked);
	return failures ? 1 : 0;
}
