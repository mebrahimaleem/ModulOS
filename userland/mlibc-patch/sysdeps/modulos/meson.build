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
	'glibc': false,
	'bsd': false,
  'posix': false,
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
		'include/abi-bits/auxv.h',
		'include/abi-bits/clockid_t.h',
		'include/abi-bits/errno.h',
		'include/abi-bits/fcntl.h',
		'include/abi-bits/limits.h',
		'include/abi-bits/mode_t.h',
		'include/abi-bits/pid_t.h',
		'include/abi-bits/seek-whence.h',
		'include/abi-bits/signal.h',
		'include/abi-bits/stat.h',
    'include/abi-bits/uid_t.h',
		'include/abi-bits/vm-flags.h',
		'include/abi-bits/wait.h',
		subdir: 'abi-bits',
		follow_symlinks: true
    )

    install_headers('include/syscall.h')
    install_headers('include/syscall_vectors.h')
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

