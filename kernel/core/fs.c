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

#include <lib/kmemcmp.h>

struct fs_file_t {
	struct fs_file_info_t info;
};

struct fs_mount_t {
	fs_stat_t stat;
};

struct fs_node_t {
	struct fs_node_t* sub;
	struct fs_node_t* co;

	struct fs_mount_t* mount;
	void* handle;

	const char* name;
	uint8_t name_len;
};

static enum fs_error_t vfs_stat(void* cntx, struct fs_file_info_t* info) {
	(void)cntx;
	info->size = 0;
	info->type = FS_DIR;
	return FS_ERROR_OK;
}

static struct fs_mount_t vfs_mount = {
	.stat = vfs_stat
};

static uint8_t root_fs_lock;
static struct fs_node_t vfs_root;

static uint8_t extract_prefix(const char* path, uint8_t off, char* buf, uint8_t* term) {
	uint8_t i = 0;
	while (path[off] && path[off] != '/') {
		buf[i++] = path[off++];
	}

	if (path[off]) {
		buf[i++] = path[off++];
	}

	*term = !path[off];

	buf[i] = 0;
	return off;
}

static struct fs_node_t* walk_vfs(struct fs_node_t* node, uint8_t off, char* buf, const char* path, uint8_t create) {
	struct fs_node_t* walk, * temp;
	uint8_t term;

	off = extract_prefix(path, off, buf, &term);

	if (kmemcmp(buf, node->name, node->name_len)) {
		return 0;
	}

	if (term) {
		return node;
	}

	for (walk = node->sub; walk; walk = walk->co) {
		temp = walk_vfs(walk, off, buf, path, create);
		if (temp) {
			return temp;
		}
	}

	//TODO: create
	return 0;
}

void fs_init(void) {
	lock_init(&root_fs_lock);

	vfs_root.sub = 0;
	vfs_root.co = 0;
	vfs_root.mount = &vfs_mount;
	vfs_root.name = "/";
	vfs_root.name_len = 1;
}

void fs_mount(void* root, fs_stat_t stat, const char* path) {
	char buf[256];
	struct fs_node_t* node;
	struct fs_file_info_t info;

	lock_acquire(&root_fs_lock);
	node = walk_vfs(&vfs_root, 0, buf, path, 1);

	if (!node) {
		logging_log_error("Failed to mount filesystem");
	}
	else {
		node->mount = kmalloc(sizeof(struct fs_mount_t));
		node->mount->stat = stat;
		node->handle = root;
	}

	lock_release(&root_fs_lock);

	if (stat(root, &info) == FS_ERROR_OK) {
		logging_log_debug("Mounted %s (0x%lx %u)", path, info.size, info.type);
	}
	else {
		logging_log_error("Failed to stat %s", path);
	}
}
