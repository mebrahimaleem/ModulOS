# meson.build - ModulOS mlibc meson build config
# Copyright (C) 2026  Ebrahim Aleem
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

sysdep_supported_options = {
	'linux': false,
	'glibc': true,
	'bsd': true,
  'posix': true,
}

rtld_dso_sources += files(
  'syscall.S',
  'sysdeps.cpp'
)

libc_sources += files(
  'syscall.S',
  'entry.cpp',
  'sysdeps.cpp'
)

if not no_headers
  install_headers(
    'include/abi-bits/vm-flags.h',
    'include/abi-bits/wait.h',
    'include/abi-bits/fcntl.h',
    'include/abi-bits/sigevent.h',
    'include/abi-bits/poll.h',
    'include/abi-bits/socket.h',
    'include/abi-bits/stat.h',
    'include/abi-bits/blkcnt_t.h',
    'include/abi-bits/termios.h',
    'include/abi-bits/clockid_t.h',
    'include/abi-bits/nlink_t.h',
    'include/abi-bits/rlim_t.h',
    'include/abi-bits/dev_t.h',
    'include/abi-bits/errno.h',
    'include/abi-bits/uid_t.h',
    'include/abi-bits/msg.h',
    'include/abi-bits/socklen_t.h',
    'include/abi-bits/access.h',
    'include/abi-bits/suseconds_t.h',
    'include/abi-bits/mqueue.h',
    'include/abi-bits/time.h',
    'include/abi-bits/statvfs.h',
    'include/abi-bits/shm.h',
    'include/abi-bits/utmp-defines.h',
    'include/abi-bits/mode_t.h',
    'include/abi-bits/resource.h',
    'include/abi-bits/limits.h',
    'include/abi-bits/fsblkcnt_t.h',
    'include/abi-bits/signal.h',
    'include/abi-bits/ipc.h',
    'include/abi-bits/sigval.h',
    'include/abi-bits/pid_t.h',
    'include/abi-bits/utmpx.h',
    'include/abi-bits/blksize_t.h',
    'include/abi-bits/ino_t.h',
    'include/abi-bits/in.h',
    'include/abi-bits/utsname.h',
    'include/abi-bits/auxv.h',
    'include/abi-bits/seek-whence.h',
    'include/abi-bits/fsfilcnt_t.h',
    'include/abi-bits/ioctls.h',
    'include/abi-bits/gid_t.h',
		subdir: 'abi-bits',
		follow_symlinks: true
    )

    install_headers('include/sys/syscall.h', subdir: 'sys')
    install_headers('include/sys/syscall_vectors.h', subdir: 'sys')

    install_headers('include/sys/sysmacros.h', subdir: 'sys')
    install_headers('include/features.h')
endif

if not headers_only
  crt = custom_target('crt1',
    build_by_default: true,
    command: c_compiler.cmd_array() + ['-c', '-o', '@OUTPUT@', '@INPUT@'],
    input: 'crt1.S',
    output: 'crt1.o',
    install: true,
    install_dir: get_option('libdir')
  )
endif

