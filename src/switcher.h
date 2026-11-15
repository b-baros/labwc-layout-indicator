#ifndef SWITCHER_H
#define SWITCHER_H

#include <stdbool.h>

#include "keymap.h"

/*
 * labwc has no request for setting the layout, so switching taps the
 * layout-toggle key on a virtual uinput keyboard. labwc treats it like a real
 * keyboard, so the tap changes the shared layout exactly as a key press would.
 */
struct switcher {
	int fd;
	struct toggle_combo combo;
};

void switcher_init(struct switcher *switcher);
int switcher_open(struct switcher *switcher, const struct toggle_combo *combo);
bool switcher_matches(const struct switcher *switcher, const struct toggle_combo *combo);
int switcher_tap(struct switcher *switcher);
void switcher_close(struct switcher *switcher);

#endif
