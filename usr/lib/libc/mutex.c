
#include <syscall.h>

void mutex_lock(int lock_idx) {
	sys_mutex_lock(lock_idx);
}

void mutex_unlock(int lock_idx) {
	sys_mutex_unlock(lock_idx);
}


