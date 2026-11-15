#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sdbus.h"
#include "tray.h"

#define ITEM_PATH "/StatusNotifierItem"
#define ITEM_INTERFACE "org.kde.StatusNotifierItem"
#define WATCHER "org.kde.StatusNotifierWatcher"

/* Trays re-read these after every change and wait for the answer, so they must cost nothing. */
static int get_property(sd_bus *bus, const char *path, const char *interface, const char *property,
	sd_bus_message *reply, void *userdata, sd_bus_error *error)
{
	struct tray *tray = userdata;
	const char *icon = tray->badge ? tray->badge->icon_name : "input-keyboard";
	const char *title = tray->badge ? tray->badge->title : "Keyboard layout";

	if (strcmp(property, "Category") == 0)
		return sd_bus_message_append(reply, "s", "Hardware");
	if (strcmp(property, "Id") == 0)
		return sd_bus_message_append(reply, "s", "labwc-layout-indicator");
	if (strcmp(property, "Title") == 0)
		return sd_bus_message_append(reply, "s", title);
	if (strcmp(property, "Status") == 0)
		return sd_bus_message_append(reply, "s", "Active");
	if (strcmp(property, "WindowId") == 0)
		return sd_bus_message_append(reply, "i", 0);
	if (strcmp(property, "IconName") == 0)
		return sd_bus_message_append(reply, "s", icon);
	if (strcmp(property, "IconThemePath") == 0)
		return sd_bus_message_append(reply, "s", "");
	if (strcmp(property, "IconPixmap") == 0)
		return sd_bus_message_append(reply, "a(iiay)", 0);
	if (strcmp(property, "ToolTip") == 0)
		return sd_bus_message_append(reply, "(sa(iiay)ss)", icon, 0, "Keyboard layout", title);
	if (strcmp(property, "ItemIsMenu") == 0)
		return sd_bus_message_append(reply, "b", 0);
	return sd_bus_error_setf(error, SD_BUS_ERROR_UNKNOWN_PROPERTY, "Unknown property %s", property);
}

static int method_activate(sd_bus_message *m, void *userdata, sd_bus_error *error)
{
	struct tray *tray = userdata;
	int r = sd_bus_reply_method_return(m, "");
	tray->listener.activate(tray->listener.data);
	return r;
}

static int method_secondary_activate(sd_bus_message *m, void *userdata, sd_bus_error *error)
{
	struct tray *tray = userdata;
	int r = sd_bus_reply_method_return(m, "");
	tray->listener.secondary_activate(tray->listener.data);
	return r;
}

static int method_context_menu(sd_bus_message *m, void *userdata, sd_bus_error *error)
{
	struct tray *tray = userdata;
	int r = sd_bus_reply_method_return(m, "");
	tray->listener.context_menu(tray->listener.data);
	return r;
}

static int method_scroll(sd_bus_message *m, void *userdata, sd_bus_error *error)
{
	return sd_bus_reply_method_return(m, "");
}

static const sd_bus_vtable item_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_PROPERTY("Category", "s", get_property, 0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_PROPERTY("Id", "s", get_property, 0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_PROPERTY("Title", "s", get_property, 0, 0),
	SD_BUS_PROPERTY("Status", "s", get_property, 0, 0),
	SD_BUS_PROPERTY("WindowId", "i", get_property, 0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_PROPERTY("IconName", "s", get_property, 0, 0),
	SD_BUS_PROPERTY("IconThemePath", "s", get_property, 0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_PROPERTY("IconPixmap", "a(iiay)", get_property, 0, 0),
	SD_BUS_PROPERTY("ToolTip", "(sa(iiay)ss)", get_property, 0, 0),
	SD_BUS_PROPERTY("ItemIsMenu", "b", get_property, 0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_METHOD("Activate", "ii", "", method_activate, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("SecondaryActivate", "ii", "", method_secondary_activate, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("ContextMenu", "ii", "", method_context_menu, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("Scroll", "is", "", method_scroll, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_SIGNAL("NewIcon", "", 0),
	SD_BUS_SIGNAL("NewToolTip", "", 0),
	SD_BUS_SIGNAL("NewTitle", "", 0),
	SD_BUS_SIGNAL("NewStatus", "s", 0),
	SD_BUS_VTABLE_END,
};

static void register_item(struct tray *tray)
{
	sd_bus_call_method_async(tray->bus, NULL, WATCHER, "/StatusNotifierWatcher", WATCHER,
		"RegisterStatusNotifierItem", NULL, NULL, "s", tray->service);
}

/* The tray host (re)appeared, e.g. after the panel restarted. */
static int watcher_appeared(sd_bus_message *m, void *userdata, sd_bus_error *error)
{
	const char *name, *old_owner, *new_owner;
	if (sd_bus_message_read(m, "sss", &name, &old_owner, &new_owner) >= 0 && *new_owner)
		register_item(userdata);
	return 0;
}

int tray_init(struct tray *tray, const struct tray_listener *listener, const struct badge *badge)
{
	memset(tray, 0, sizeof(*tray));
	tray->listener = *listener;
	tray->badge = badge;
	snprintf(tray->service, sizeof(tray->service), "org.kde.StatusNotifierItem-%d-1", (int)getpid());

	int r = sd_bus_open_user(&tray->bus);
	if (r < 0)
		return r;
	r = sd_bus_add_object_vtable(tray->bus, NULL, ITEM_PATH, ITEM_INTERFACE, item_vtable, tray);
	if (r < 0)
		return r;
	r = sd_bus_request_name(tray->bus, tray->service, 0);
	if (r < 0)
		return r;
	r = sd_bus_add_match(tray->bus, NULL,
		"type='signal',sender='org.freedesktop.DBus',path='/org/freedesktop/DBus',"
		"interface='org.freedesktop.DBus',member='NameOwnerChanged',arg0='" WATCHER "'",
		watcher_appeared, tray);
	if (r < 0)
		return r;
	register_item(tray);
	return 0;
}

void tray_show(struct tray *tray, const struct badge *badge)
{
	tray->badge = badge;
	sd_bus_emit_signal(tray->bus, ITEM_PATH, ITEM_INTERFACE, "NewIcon", NULL);
	sd_bus_emit_signal(tray->bus, ITEM_PATH, ITEM_INTERFACE, "NewToolTip", NULL);
	sd_bus_emit_signal(tray->bus, ITEM_PATH, ITEM_INTERFACE, "NewTitle", NULL);
}

int tray_fd(struct tray *tray)
{
	return sd_bus_get_fd(tray->bus);
}

short tray_events(struct tray *tray)
{
	int events = sd_bus_get_events(tray->bus);
	return events < 0 ? POLLIN : (short)events;
}

int tray_timeout_ms(struct tray *tray)
{
	uint64_t deadline;
	if (sd_bus_get_timeout(tray->bus, &deadline) < 0 || deadline == UINT64_MAX)
		return -1;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint64_t now = (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
	return deadline <= now ? 0 : (int)((deadline - now + 999) / 1000);
}

int tray_process(struct tray *tray)
{
	int r;
	while ((r = sd_bus_process(tray->bus, NULL)) > 0)
		;
	return r;
}
