#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "state.h"

static const char *state_dir(void)
{
	static char dir[4096];
	if (!dir[0]) {
		const char *runtime = getenv("XDG_RUNTIME_DIR");
		if (runtime && *runtime)
			snprintf(dir, sizeof(dir), "%s/labwc-layout-indicator", runtime);
		else
			snprintf(dir, sizeof(dir), "/run/user/%d/labwc-layout-indicator", (int)getuid());
		mkdir(dir, 0700);
	}
	return dir;
}

int state_lock(void)
{
	char path[4200];
	snprintf(path, sizeof(path), "%s/lock", state_dir());
	int fd = open(path, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
	if (fd < 0)
		return -2;
	if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
		int err = errno;
		close(fd);
		return err == EWOULDBLOCK ? -1 : -2;
	}
	return fd;
}

int state_load(pid_t compositor_pid)
{
	char path[4200];
	snprintf(path, sizeof(path), "%s/state", state_dir());
	FILE *f = fopen(path, "r");
	if (!f)
		return 0;
	long pid = 0;
	int group = 0;
	int n = fscanf(f, "%ld %d", &pid, &group);
	fclose(f);
	return n == 2 && pid == (long)compositor_pid && group >= 0 ? group : 0;
}

void state_save(pid_t compositor_pid, int group)
{
	char path[4200], tmp[4300];
	snprintf(path, sizeof(path), "%s/state", state_dir());
	snprintf(tmp, sizeof(tmp), "%s.tmp", path);
	FILE *f = fopen(tmp, "w");
	if (!f)
		return;
	fprintf(f, "%ld %d\n", (long)compositor_pid, group);
	if (fclose(f) == 0)
		rename(tmp, path);
}
