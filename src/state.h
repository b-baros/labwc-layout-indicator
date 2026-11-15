#ifndef STATE_H
#define STATE_H

#include <sys/types.h>

/* Returns a lock fd, -1 if another instance holds the lock, or -2 if locking failed. */
int state_lock(void);

/*
 * A new client gets no modifiers event until the next layout or modifier
 * change, so the last known layout is kept, tied to the compositor's PID.
 */
int state_load(pid_t compositor_pid);
void state_save(pid_t compositor_pid, int group);

#endif
