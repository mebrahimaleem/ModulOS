/* fs.c - kernel file system layer */
/* Copyright (C) 2026  Ebrahim Aleem
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>
*/

#include <stdint.h>

#include <core/fs.h>
#include <core/lock.h>
#include <core/alloc.h>
#include <core/logging.h>
#include <core/panic.h>

#include <lib/kmemcmp.h>
#include <lib/kmemcpy.h>
#include <lib/kstrcmp.h>
#include <lib/kstrcpy.h>
#include <lib/kstrlen.h>

static uint8_t fs_lock;

struct vfs_mount_t {
	struct mount_cntx_t* cntx;
	fs_stat_t stat;
};

struct vfs_tree_node_t {
	struct vfs_tree_node_t* co;
	struct vfs_tree_node_t* sub;

	const char* name;

	struct vfs_mount_t* mount;
};

static struct vfs_tree_node_t vfs_root;

static inline const char* simplify_path(const char* path) {
	while (
			*path == '/' ||
			(*path == '.' && (*(path+1) == '/' || !*(path+1)))) {
		path++;
	}

	return path;
}

static inline const char* path_next(const char* path, uint8_t* len) {
	*len = 0;

	while (*path && *path != '/') {
		path++;
		(*len)++;
	}

	return simplify_path(path);
}

static struct vfs_mount_t* find_mountpoint_sub(struct vfs_tree_node_t* root, const char* path) {
	struct vfs_tree_node_t* walk;
	struct vfs_mount_t* sub_mount = 0;
	const char* next = path;
	uint8_t len;

	path = path_next(path, &len);

	for (walk = root->sub; walk; walk = walk->co) {
		if (len == kstrlen(walk->name) && kmemcmp(walk->name, next, len) == 0) {
			if (*next) {
				sub_mount = find_mountpoint_sub(walk, path);
			}

			break;
		}
	}

	if (sub_mount) {
		return sub_mount;
	}

	return walk->mount;
}

static struct vfs_mount_t* find_mountpoint(const char* path) {
	path = simplify_path(path);

	if (kstrcmp(path, "") == 0) {
		return vfs_root.mount;
	}

	return find_mountpoint_sub(&vfs_root, path);
}

void fs_init(void) {
	lock_init(&fs_lock);

	vfs_root.co = 0;
	vfs_root.sub = 0;
	vfs_root.name = "";
	vfs_root.mount = 0;
}

enum file_status_t fs_mount(
		const char* mountpoint,
		struct mount_cntx_t* cntx,
		fs_stat_t stat
		) {

	if (kstrcmp(mountpoint, "") && !vfs_root.mount) {
		vfs_root.mount = kmalloc(sizeof(struct vfs_mount_t));

		if (!vfs_root.mount) {
			logging_log_error("Failed to mount root");
			return FILE_ERROR;
		}

		vfs_root.mount->cntx = cntx;
		vfs_root.mount->stat = stat;

		return FILE_OK;
	}

	//TODO: support non root mount
	logging_log_error("Can only mount root");
	return FILE_ERROR;
}

enum file_status_t fs_stat(const char* name, struct file_info_t* info) {
	struct vfs_mount_t* mount = find_mountpoint(name);

	if (!mount) {
		return FILE_DNE;
	}

	return mount->stat(mount->cntx, name, info);
}
