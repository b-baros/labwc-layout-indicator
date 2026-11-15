#ifndef WAYLAND_H
#define WAYLAND_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

struct wayland_listener {
	void (*keymap)(void *data, const char *text);
	void (*group)(void *data, uint32_t group);
	void *data;
};

struct wayland_keyboard {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_seat *seat;
	struct wl_keyboard *keyboard;
	struct wayland_listener listener;
	pid_t compositor_pid;
	bool have_keymap;
};

/* Connects and waits until the compositor has sent everything it will send now. */
int wayland_keyboard_connect(struct wayland_keyboard *wk, const struct wayland_listener *listener);
int wayland_keyboard_fd(struct wayland_keyboard *wk);
/* Call before poll(); returns -1 if the connection is broken. */
int wayland_keyboard_prepare(struct wayland_keyboard *wk);
/* Call after poll() with the revents of the Wayland fd. */
int wayland_keyboard_dispatch(struct wayland_keyboard *wk, short revents);

#endif
