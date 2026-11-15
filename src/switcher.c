#include <errno.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "switcher.h"

void switcher_init(struct switcher *switcher)
{
	memset(switcher, 0, sizeof(*switcher));
	switcher->fd = -1;
}

int switcher_open(struct switcher *switcher, const struct toggle_combo *combo)
{
	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0)
		return -errno;

	struct uinput_setup setup = { .id = { .bustype = BUS_VIRTUAL, .version = 1 } };
	snprintf(setup.name, sizeof(setup.name), "labwc-layout-indicator");
	bool ok = ioctl(fd, UI_SET_EVBIT, EV_KEY) == 0 && ioctl(fd, UI_SET_KEYBIT, combo->key) == 0;
	for (int i = 0; ok && i < combo->modifier_count; i++)
		ok = ioctl(fd, UI_SET_KEYBIT, combo->modifiers[i]) == 0;
	ok = ok && ioctl(fd, UI_DEV_SETUP, &setup) == 0 && ioctl(fd, UI_DEV_CREATE) == 0;
	if (!ok) {
		int err = errno;
		close(fd);
		return -err;
	}
	switcher->fd = fd;
	switcher->combo = *combo;
	return 0;
}

bool switcher_matches(const struct switcher *switcher, const struct toggle_combo *combo)
{
	return switcher->fd >= 0 && memcmp(&switcher->combo, combo, sizeof(*combo)) == 0;
}

/* Each key event goes in its own frame, as a real keyboard would send it. */
static bool send_key(int fd, int code, int value)
{
	struct input_event events[2] = {
		{ .type = EV_KEY, .code = (unsigned short)code, .value = value },
		{ .type = EV_SYN, .code = SYN_REPORT },
	};
	return write(fd, events, sizeof(events)) == (ssize_t)sizeof(events);
}

int switcher_tap(struct switcher *switcher)
{
	const struct toggle_combo *c = &switcher->combo;
	if (switcher->fd < 0)
		return -1;
	bool ok = true;
	for (int i = 0; ok && i < c->modifier_count; i++)
		ok = send_key(switcher->fd, c->modifiers[i], 1);
	ok = ok && send_key(switcher->fd, c->key, 1) && send_key(switcher->fd, c->key, 0);
	for (int i = c->modifier_count - 1; i >= 0; i--)
		ok &= send_key(switcher->fd, c->modifiers[i], 0);
	return ok ? 0 : -1;
}

void switcher_close(struct switcher *switcher)
{
	if (switcher->fd >= 0) {
		ioctl(switcher->fd, UI_DEV_DESTROY);
		close(switcher->fd);
	}
	switcher_init(switcher);
}
