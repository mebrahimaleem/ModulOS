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
/*
struct vfs_mount_t {
	fs_stat_t stat;
	fs_opendir_t opendir;
	fs_closedir_t closedir;
};

struct vfs_node_t {
	struct vfs_node_t* co;
	struct vfs_node_t* sub;

	const char* name;
	struct vfs_mount_t* mount;
};

static struct vfs_node_t vfs_root;
static struct vfs_mount_t vfs_mount;
static uint8_t vfs_lock;

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

static struct vfs_node_t* resolve_sub_path(struct vfs_node_t* root, const char* path) {
	struct vfs_node_t* walk, * node;
	const char* next = path;
	uint8_t len;

	path = path_next(path, &len);

	for (walk = root->sub; walk; walk = walk->co) {
		if (len == kstrlen(walk->name) && kmemcmp(walk->name, next, len) == 0) {
			if (*next) {
				return resolve_sub_path(walk, path);
			}

			return walk;
		}
	}

	struct fs_file_open_handle_t* file = root->mount->opendir(root->handle);

	if (!file) {
		return 0;
	}

	char* buffer = kmalloc(256);
	struct fs_file_handle_t* handle;
	while (root->mount->read_dir(file, &handle, buffer) == FS_STATUS_OK) {
		if (kstrcmp(buffer, next) == 0) {
			walk = init_node(root, handle, buffer);
			if (!walk) {
				kfree(buffer);
				return 0;
			}

			walk->co = root->sub;
			root->sub = walk;

			root->mount->close(file);

			if (*next) {
				node = resolve_sub_path(walk, path);
				if (node) {
					walk->accesses++;
					return node;
				}
				return 0;
			}

			walk->accesses++;
			return walk;
		}
	}


	root->mount->close(file);
	kfree(buffer);
	return 0;
}

static struct fs_node_t* resolve_absolute_path(const char* path) {
	path = simplify_path(path);

	if (kstrcmp(path, "") == 0) {
		return &vfs_root;
	}

	return resolve_sub_path(&vfs_root, path);
}

void fs_init(void) {
	vfs_root.co = 0;
	vfs_root.sub = 0;
	vfs_root.name = "";
	vfs_root.mount = 0;

	lock_init(&vfs_lock);
}

void fs_mount(
		const char* path,
		fs_stat_t stat,
		fs_opendir_t opendir,
		fs_closedir_t closedir) {
}



*/








/* old implementation */

#define VFS_STATUS_MOUNTED		0x1

struct fs_mount_t {
	struct fs_mount_t* parent;
	fs_open_t open;
	fs_close_t close;
	fs_stat_t stat;
	fs_read_dir_t read_dir;
};

struct fs_node_t {
	struct fs_node_t* parent;
	struct fs_node_t* sub;
	struct fs_node_t* co;

	struct fs_mount_t* mount;

	struct fs_file_handle_t* handle;

	uint64_t accesses;

	const char* name;

	struct fs_info_t info;

	uint8_t status;
};

struct vfs_file_open_handle_t {
	struct fs_node_t* walk;
};

static struct fs_file_open_handle_t* vfs_open(struct fs_file_handle_t* handle) {
	struct fs_node_t* node = (struct fs_node_t*)handle;
	struct vfs_file_open_handle_t* open;

	open = kmalloc(sizeof(struct vfs_file_open_handle_t));
	open->walk = node->sub;
	return (struct fs_file_open_handle_t*)open;
}

static void vfs_close(struct fs_file_open_handle_t* handle) {
	kfree(handle);
}

static enum fs_status_t vfs_stat(struct fs_file_open_handle_t* handle, struct fs_info_t* info) {
	(void)handle;
	info->size = 0;
	info->type = FS_TYPE_DIR;

	return FS_STATUS_OK;
}

static enum fs_status_t vfs_read_dir(struct fs_file_open_handle_t* handle, struct fs_file_handle_t** file, char* name) {
	struct vfs_file_open_handle_t* open = (struct vfs_file_open_handle_t*)handle;
	if (!open->walk) {
		return FS_STATUS_EOF;
	}

	*file = open->walk->handle;
	kstrcpy(name, open->walk->name);

	open->walk = open->walk->co;
	return FS_STATUS_OK;
}

static struct fs_mount_t vfs_mount = {
	.parent = &vfs_mount,
	.open = vfs_open,
	.close = vfs_close,
	.stat = vfs_stat,
	.read_dir = vfs_read_dir
};

static struct fs_node_t vfs_root;

static uint8_t vfs_lock;

void fs_init(void) {
	lock_init(&vfs_lock);

	vfs_root.handle = (struct fs_file_handle_t*)&vfs_root;
	vfs_root.co = 0;
	vfs_root.sub = 0;
	vfs_root.parent = &vfs_root;
	vfs_root.mount = &vfs_mount;
	vfs_root.name = "";
	vfs_root.status = 0;
	vfs_root.accesses = 1;
	vfs_root.info.size = 0;
	vfs_root.info.type = FS_TYPE_DIR;
}

static inline struct fs_node_t* init_node(
		struct fs_node_t* parent,
		struct fs_file_handle_t* handle,
		const char* name) {

	struct fs_node_t* node;
	enum fs_status_t status;
	struct fs_info_t info;

	struct fs_file_open_handle_t* open = parent->mount->open(handle);

	if (!open) {
		return 0;
	}

	status = parent->mount->stat(open, &info);
	parent->mount->close(open);

	if (status != FS_STATUS_OK) {
		return 0;
	}


	node = kmalloc(sizeof(struct fs_node_t));

	node->handle = handle;
	node->co = 0;
	node->sub = 0;
	node->parent = parent;
	node->mount = parent->mount;
	node->name = name;
	node->status = 0;
	node->accesses = 0;
	node->info = info;

	return node;
}

static const inline char* simplify_path(const char* path) {
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

static struct fs_node_t* resolve_sub_path(struct fs_node_t* root, const char* path) {
	struct fs_node_t* walk, * node;
	const char* next = path;
	uint8_t len;

	path = path_next(path, &len);

	for (walk = root->sub; walk; walk = walk->co) {
		if (len == kstrlen(walk->name) && kmemcmp(walk->name, next, len) == 0) {
			if (*next) {
				node = resolve_sub_path(walk, path);
				if (node) {
					walk->accesses++;
					return node;
				}
				return 0;
			}

			walk->accesses++;
			return walk;
		}
	}

	struct fs_file_open_handle_t* file = root->mount->open(root->handle);

	if (!file) {
		return 0;
	}

	char* buffer = kmalloc(256);
	struct fs_file_handle_t* handle;
	while (root->mount->read_dir(file, &handle, buffer) == FS_STATUS_OK) {
		if (kstrcmp(buffer, next) == 0) {
			walk = init_node(root, handle, buffer);
			if (!walk) {
				kfree(buffer);
				return 0;
			}

			walk->co = root->sub;
			root->sub = walk;

			root->mount->close(file);

			if (*next) {
				node = resolve_sub_path(walk, path);
				if (node) {
					walk->accesses++;
					return node;
				}
				return 0;
			}

			walk->accesses++;
			return walk;
		}
	}


	root->mount->close(file);
	kfree(buffer);
	return 0;
}

static struct fs_node_t* resolve_absolute_path(const char* path) {
	path = simplify_path(path);

	if (kstrcmp(path, "") == 0) {
		return &vfs_root;
	}

	return resolve_sub_path(&vfs_root, path);
}

static void cleanup_path(struct fs_node_t* root, const char* path) {
	//TODO: cleanup vfs tree
	(void)root;
	(void)path;
}

extern enum fs_status_t fs_mount(
		struct fs_file_handle_t* root,
		const char* path,
		fs_open_t open,
		fs_close_t close,
		fs_stat_t stat,
		fs_read_dir_t read_dir) {

	lock_acquire(&vfs_lock);

	struct fs_node_t* node = resolve_absolute_path(path);

	if (node->status & VFS_STATUS_MOUNTED) {
		lock_release(&vfs_lock);
		return FS_STATUS_BUSY;
	}

	struct fs_node_t revert;
	revert = *node;
	struct fs_mount_t* mount = kmalloc(sizeof(struct fs_mount_t));
	mount->parent = node->mount;

	mount->open = open;
	mount->close = close;
	mount->stat = stat;
	mount->read_dir = read_dir;

	node->mount = mount;
	node->handle = root;
	node->status = VFS_STATUS_MOUNTED;

	struct fs_info_t info;
	enum fs_status_t status = node->mount->stat(node->handle, &info);
	if (status != FS_STATUS_OK) {
		kfree(mount);
		*node = revert;
		lock_release(&vfs_lock);
		return status;
	}

	node->info = info;
	
	lock_release(&vfs_lock);
	return FS_STATUS_OK;
}

struct fs_file_t* fs_open(const char* path) {
	lock_acquire(&vfs_lock);
	struct fs_node_t* node = resolve_absolute_path(path);
	lock_release(&vfs_lock);

	if (!node) {
		return 0;
	}

	return (struct fs_file_t*)node;
}

void fs_close(struct fs_file_t* file) {
	struct fs_node_t* node = (struct fs_node_t*)file;

	lock_acquire(&vfs_lock);
	cleanup_path(&vfs_root, simplify_path(node->name));
	lock_release(&vfs_lock);
}

enum fs_status_t fs_stat(struct fs_file_t* file, struct fs_info_t* info) {
	struct fs_node_t* node = (struct fs_node_t*)file;
	*info = node->info;

	return FS_STATUS_OK;
}

enum fs_status_t fs_read_dir(struct fs_file_t* file, char* buffer) {
	struct fs_node_t* node = (struct fs_node_t*)file;

	lock_acquire(&vfs_lock);
	enum fs_status_t sts = node->mount->read_dir(node->handle, buffer, "");
	lock_release(&vfs_lock);

	return sts;
}
