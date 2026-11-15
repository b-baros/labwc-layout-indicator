#ifndef TRAY_H
#define TRAY_H

#include "badges.h"

struct tray_listener {
	void (*activate)(void *data);
	void (*secondary_activate)(void *data);
	void (*context_menu)(void *data);
	void *data;
};

struct tray {
	struct sd_bus *bus;
	char service[64];
	const struct badge *badge;
	struct tray_listener listener;
};

/* Publishes a StatusNotifierItem showing badge and registers it with the tray host. */
int tray_init(struct tray *tray, const struct tray_listener *listener, const struct badge *badge);
void tray_show(struct tray *tray, const struct badge *badge);
int tray_fd(struct tray *tray);
short tray_events(struct tray *tray);
int tray_timeout_ms(struct tray *tray);
int tray_process(struct tray *tray);

#endif
