#include <errno.h>
#include <poll.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

#include "badges.h"
#include "config.h"
#include "keymap.h"
#include "state.h"
#include "switcher.h"
#include "tray.h"
#include "wayland.h"

#define TAP_INTERVAL_MS 8
#define NEW_DEVICE_DELAY_MS 600
#define TARGET_TIMEOUT_MS 1500
/* GTK checks icon theme directories for changes at most every 5 seconds. */
#define REANNOUNCE_DELAY_MS 7000

struct app {
	struct xkb_context *xkb;
	struct wayland_keyboard wayland;
	struct tray tray;
	bool tray_ready;
	struct switcher switcher;
	uint64_t switcher_ready_at;
	struct keymap_info keymap;
	struct badge badges[MAX_LAYOUTS];
	int group;
	/* Layout a click is heading to while its taps are still on the way, or -1. */
	int target;
	int pending_taps;
	bool tap_scheduled;
	int shown;
	int tap_timer;
	int target_timer;
	int reannounce_timer;
};

static void warn(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fputs("labwc-layout-indicator: ", stderr);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	va_end(args);
}

static uint64_t now_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

static void timer_arm(int fd, int ms)
{
	struct itimerspec spec = { 0 };
	spec.it_value.tv_sec = ms / 1000;
	spec.it_value.tv_nsec = ms > 0 ? (long)(ms % 1000) * 1000000L : 1;
	timerfd_settime(fd, 0, &spec, NULL);
}

static void timer_disarm(int fd)
{
	struct itimerspec spec = { 0 };
	timerfd_settime(fd, 0, &spec, NULL);
}

static int shown_index(const struct app *app)
{
	int i = app->target >= 0 ? app->target : app->group;
	return i >= 0 && (size_t)i < app->keymap.count ? i : 0;
}

static void refresh(struct app *app, bool force)
{
	int i = shown_index(app);
	if (i == app->shown && !force)
		return;
	app->shown = i;
	if (app->tray_ready)
		tray_show(&app->tray, &app->badges[i]);
}

static void update_switcher(struct app *app)
{
	if (!app->keymap.has_toggle) {
		switcher_close(&app->switcher);
		return;
	}
	if (switcher_matches(&app->switcher, &app->keymap.toggle))
		return;
	switcher_close(&app->switcher);
	int r = switcher_open(&app->switcher, &app->keymap.toggle);
	if (r < 0)
		warn("switching by click is unavailable: /dev/uinput: %s", strerror(-r));
	else
		app->switcher_ready_at = now_ms() + NEW_DEVICE_DELAY_MS;
}

static void apply_keymap(struct app *app, const struct keymap_info *info)
{
	app->keymap = *info;
	if (app->keymap.count == 0) {
		memset(&app->keymap.layouts[0], 0, sizeof(app->keymap.layouts[0]));
		app->keymap.count = 1;
	}
	badges_assign(&app->keymap, app->badges);
	int written = badges_write_icons(app->badges, app->keymap.count);
	if (written < 0)
		warn("cannot write the badge icons");
	else if (written > 0)
		timer_arm(app->reannounce_timer, REANNOUNCE_DELAY_MS);
	app->target = -1;
	app->pending_taps = 0;
	app->tap_scheduled = false;
	timer_disarm(app->tap_timer);
	timer_disarm(app->target_timer);
	update_switcher(app);
	refresh(app, true);
}

static void on_keymap(void *data, const char *text)
{
	struct app *app = data;
	struct keymap_info info;
	if (keymap_info_parse(app->xkb, text, &info))
		apply_keymap(app, &info);
	else
		warn("cannot parse the compositor's keymap");
}

static void on_group(void *data, uint32_t group)
{
	struct app *app = data;
	if ((int)group != app->group) {
		app->group = (int)group;
		state_save(app->wayland.compositor_pid, app->group);
	}
	if (app->target >= 0) {
		if (app->group != app->target || app->pending_taps > 0)
			return; /* a layout the taps only pass through */
		app->target = -1;
		timer_disarm(app->target_timer);
	}
	refresh(app, false);
}

/* Shows the target right away; the taps catch up a few milliseconds later. */
static void go_to(struct app *app, int target)
{
	int n = (int)app->keymap.count;
	if (n < 2 || app->switcher.fd < 0)
		return;
	int base = shown_index(app);
	target = (target % n + n) % n;
	int taps = ((target - base) % n + n) % n;
	if (taps == 0)
		return;

	app->pending_taps += taps;
	app->target = target;
	refresh(app, false);
	timer_arm(app->target_timer, TARGET_TIMEOUT_MS);
	if (!app->tap_scheduled) {
		uint64_t now = now_ms();
		timer_arm(app->tap_timer, app->switcher_ready_at > now ? (int)(app->switcher_ready_at - now) : 0);
		app->tap_scheduled = true;
	}
}

static void on_activate(void *data)
{
	struct app *app = data;
	go_to(app, shown_index(app) + 1);
}

static void on_context_menu(void *data)
{
	struct app *app = data;
	go_to(app, shown_index(app) - 1);
}

static void on_secondary_activate(void *data)
{
	go_to(data, 0);
}

static void on_tap_timer(struct app *app)
{
	uint64_t expirations;
	if (read(app->tap_timer, &expirations, sizeof(expirations)) < 0)
		return;
	app->tap_scheduled = false;
	if (app->pending_taps <= 0)
		return;
	if (switcher_tap(&app->switcher) < 0)
		app->pending_taps = 0;
	else
		app->pending_taps--;
	if (app->pending_taps > 0) {
		timer_arm(app->tap_timer, TAP_INTERVAL_MS);
		app->tap_scheduled = true;
	}
}

/* The clicked layout never arrived (a tap failed, or a key was pressed meanwhile). */
static void on_target_timer(struct app *app)
{
	uint64_t expirations;
	if (read(app->target_timer, &expirations, sizeof(expirations)) < 0)
		return;
	if (app->target >= 0) {
		app->target = -1;
		refresh(app, false);
	}
}

/* New icons may have been invisible to the tray when first announced. */
static void on_reannounce_timer(struct app *app)
{
	uint64_t expirations;
	if (read(app->reannounce_timer, &expirations, sizeof(expirations)) >= 0)
		refresh(app, true);
}

/*
 * labwc only sends a keymap while the seat has a current keyboard. It has none
 * from the moment the device that produced the last key goes away (such as our
 * own virtual key, when this program restarts) until the next key press.
 */
static void load_fallback_keymap(struct app *app)
{
	struct keymap_info info = { 0 };
	char *text = keymap_compile_from_labwc_env();
	if (!text || !keymap_info_parse(app->xkb, text, &info))
		warn("no keymap from the compositor and none could be compiled");
	free(text);
	apply_keymap(app, &info);
}

static int run(struct app *app)
{
	struct pollfd fds[5];
	for (;;) {
		if (tray_process(&app->tray) < 0) {
			warn("lost the session bus connection");
			return 1;
		}
		if (wayland_keyboard_prepare(&app->wayland) < 0) {
			warn("lost the compositor connection");
			return 1;
		}
		fds[0] = (struct pollfd){ .fd = wayland_keyboard_fd(&app->wayland), .events = POLLIN };
		fds[1] = (struct pollfd){ .fd = tray_fd(&app->tray), .events = tray_events(&app->tray) };
		fds[2] = (struct pollfd){ .fd = app->tap_timer, .events = POLLIN };
		fds[3] = (struct pollfd){ .fd = app->target_timer, .events = POLLIN };
		fds[4] = (struct pollfd){ .fd = app->reannounce_timer, .events = POLLIN };

		int r = poll(fds, 5, tray_timeout_ms(&app->tray));
		if (r < 0 && errno != EINTR) {
			warn("poll: %s", strerror(errno));
			return 1;
		}
		if (wayland_keyboard_dispatch(&app->wayland, r > 0 ? fds[0].revents : 0) < 0) {
			warn("lost the compositor connection");
			return 1;
		}
		if (r > 0 && (fds[2].revents & POLLIN))
			on_tap_timer(app);
		if (r > 0 && (fds[3].revents & POLLIN))
			on_target_timer(app);
		if (r > 0 && (fds[4].revents & POLLIN))
			on_reannounce_timer(app);
	}
}

static void usage(FILE *out)
{
	fputs("Usage: labwc-layout-indicator [--help] [--version]\n\n"
		"Shows the active keyboard layout in the system tray.\n"
		"Left click: next layout. Right click: previous layout. Middle click: first layout.\n", out);
}

int main(int argc, char **argv)
{
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--version") == 0) {
			puts("labwc-layout-indicator " VERSION);
			return 0;
		}
		usage(strcmp(argv[i], "--help") == 0 ? stdout : stderr);
		return strcmp(argv[i], "--help") == 0 ? 0 : 2;
	}

	if (state_lock() == -1) {
		warn("already running");
		return 0;
	}

	static struct app app;
	app.target = -1;
	app.shown = -1;
	switcher_init(&app.switcher);
	app.xkb = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	app.tap_timer = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
	app.target_timer = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
	app.reannounce_timer = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
	if (!app.xkb || app.tap_timer < 0 || app.target_timer < 0 || app.reannounce_timer < 0) {
		warn("initialisation failed");
		return 1;
	}

	struct wayland_listener wayland_listener = { on_keymap, on_group, &app };
	if (wayland_keyboard_connect(&app.wayland, &wayland_listener) < 0) {
		warn("cannot connect to the Wayland compositor");
		return 1;
	}
	app.group = state_load(app.wayland.compositor_pid);
	if (!app.wayland.have_keymap)
		load_fallback_keymap(&app);

	struct tray_listener tray_listener = { on_activate, on_secondary_activate, on_context_menu, &app };
	app.shown = shown_index(&app);
	int r = tray_init(&app.tray, &tray_listener, &app.badges[app.shown]);
	if (r < 0) {
		warn("cannot publish the tray icon: %s", strerror(-r));
		return 1;
	}
	app.tray_ready = true;
	return run(&app);
}
