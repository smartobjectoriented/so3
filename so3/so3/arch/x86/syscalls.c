/*
 * Copyright (C) 2026 Daniel Rossier <daniel.rossier@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/*
 * Path syscalls that x86_64 still has and musl prefers over their *at
 * versions, which are the only ones SO3 implements. SO3 has no symbolic
 * links, so lstat is stat.
 */

#include <vfs.h>
#include <syscall.h>

SYSCALL_DEFINE2(stat, const char *, path, struct stat *, st)
{
	return sys_do_newfstatat(AT_FDCWD, path, st, 0);
}

SYSCALL_DEFINE2(lstat, const char *, path, struct stat *, st)
{
	return sys_do_newfstatat(AT_FDCWD, path, st, 0);
}

SYSCALL_DEFINE2(mkdir, const char *, path, mode_t, mode)
{
	return sys_do_mkdirat(AT_FDCWD, path, mode);
}

SYSCALL_DEFINE1(rmdir, const char *, path)
{
	return sys_do_unlinkat(AT_FDCWD, path, AT_REMOVEDIR);
}

SYSCALL_DEFINE1(unlink, const char *, path)
{
	return sys_do_unlinkat(AT_FDCWD, path, 0);
}

SYSCALL_DEFINE2(rename, const char *, oldpath, const char *, newpath)
{
	return sys_do_renameat(AT_FDCWD, oldpath, AT_FDCWD, newpath);
}
