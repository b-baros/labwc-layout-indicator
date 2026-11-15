#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wayland-client.h>

#include "wayland.h"

static void keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size)
{
	struct wayland_keyboard *wk = data;
	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 || size == 0) {
		close(fd);
		return;
	}
	char *map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (map == MAP_FAILED)
		return;
	char *text = strndup(map, size);
	munmap(map, size);
	if (!text)
		return;
	wk->have_keymap = true;
	wk->listener.keymap(wk->listener.data, text);
	free(text);
}

static void keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial,
	struct wl_surface *surface, struct wl_array *keys)
{
}

static void keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface)
{
}

static void keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time,
	uint32_t key, uint32_t state)
{
}

/* labwc sends modifiers, which carry the active layout, to clients without keyboard focus too. */
static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t depressed,
	uint32_t latched, uint32_t locked, uint32_t group)
{
	struct wayland_keyboard *wk = data;
	wk->listener.group(wk->listener.data, group);
}

static void keyboard_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay)
{
}

static const struct wl_keyboard_listener keyboard_listener = {
	.keymap = keyboard_keymap,
	.enter = keyboard_enter,
	.leave = keyboard_leave,
	.key = keyboard_key,
	.modifiers = keyboard_modifiers,
	.repeat_info = keyboard_repeat_info,
};

static void seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities)
{
	struct wayland_keyboard *wk = data;
	bool has_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
	if (has_keyboard && !wk->keyboard) {
		wk->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(wk->keyboard, &keyboard_listener, wk);
	} else if (!has_keyboard && wk->keyboard) {
		wl_keyboard_release(wk->keyboard);
		wk->keyboard = NULL;
	}
}

static void seat_name(void *data, struct wl_seat *seat, const char *name)
{
}

static const struct wl_seat_listener seat_listener = {
	.capabilities = seat_capabilities,
	.name = seat_name,
};

static void registry_global(void *data, struct wl_registry *registry, uint32_t name,
	const char *interface, uint32_t version)
{
	struct wayland_keyboard *wk = data;
	if (wk->seat || strcmp(interface, wl_seat_interface.name) != 0 || version < 3)
		return;
	wk->seat = wl_registry_bind(registry, name, &wl_seat_interface, version < 5 ? version : 5);
	wl_seat_add_listener(wk->seat, &seat_listener, wk);
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove,
};

int wayland_keyboard_connect(struct wayland_keyboard *wk, const struct wayland_listener *listener)
{
	memset(wk, 0, sizeof(*wk));
	wk->listener = *listener;
	wk->display = wl_display_connect(NULL);
	if (!wk->display)
		return -1;

	struct ucred cred;
	socklen_t len = sizeof(cred);
	if (getsockopt(wl_display_get_fd(wk->display), SOL_SOCKET, SO_PEERCRED, &cred, &len) == 0)
		wk->compositor_pid = cred.pid;

	wk->registry = wl_display_get_registry(wk->display);
	wl_registry_add_listener(wk->registry, &registry_listener, wk);
	/* globals -> seat capabilities -> keymap */
	for (int i = 0; i < 3; i++)
		if (wl_display_roundtrip(wk->display) < 0)
			return -1;
	return 0;
}

int wayland_keyboard_fd(struct wayland_keyboard *wk)
{
	return wl_display_get_fd(wk->display);
}

int wayland_keyboard_prepare(struct wayland_keyboard *wk)
{
	while (wl_display_prepare_read(wk->display) != 0)
		if (wl_display_dispatch_pending(wk->display) < 0)
			return -1;
	if (wl_display_flush(wk->display) < 0 && errno != EAGAIN) {
		wl_display_cancel_read(wk->display);
		return -1;
	}
	return 0;
}

int wayland_keyboard_dispatch(struct wayland_keyboard *wk, short revents)
{
	if (revents) {
		if (wl_display_read_events(wk->display) < 0)
			return -1;
	} else {
		wl_display_cancel_read(wk->display);
	}
	return wl_display_dispatch_pending(wk->display) < 0 ? -1 : 0;
}
