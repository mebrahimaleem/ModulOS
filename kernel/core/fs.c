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
#include <core/semaphore.h>

#include <lib/kmemcmp.h>
#include <lib/kmemcpy.h>
#include <lib/kstrcmp.h>
#include <lib/kstrcpy.h>
#include <lib/kstrlen.h>
#include <lib/hash.h>
#include <lib/hash_table.h>

#include <devfs/devfs.h>

#define OPEN_TABLE_BUCKETS		100

/*
 * Locking conventions:
 *
 * The vfs layer guarantees non concurrent access to the same file, up to absolute paths.
 * The vfs layer does not guarnatee non concurrent access to directory creation, file creation,
 * multiple references to the same file via hard links, or atomic operations for multiple step
 * calls (e.g. directory listing).
 *
 * In other words, the vfs is only responsible for ensuring that no two access are made to the
 * inode at the same time. It is the actual fs driver's responsibility to ensure consistent access
 * to filesystem metadata such as inode tables and journals
 *
 * Internally, the vfs must guarantee locked access to the open_table via the fs_lock. The vfs must
 * also guarantee locked access to the vfs tree.
 */

static struct semaphore_t* fs_sem;

struct vfs_mount_t {
	struct mount_cntx_t* cntx;

	fs_open_t open;
	fs_close_t close;
	fs_stat_t stat;
	fs_read_t read;
	fs_get_seek_t get_seek;
	fs_seek_t seek;
	fs_write_t write;
	fs_create_t create;
};

struct vfs_open_file_t {
	uint8_t lock;
	const char* path;
	uint64_t refs;
	uint64_t key;
};

struct fs_handle_t {
	struct vfs_mount_t* mount;
	struct file_handle_t* handle;
	struct vfs_open_file_t* shared;
};

struct vfs_tree_node_t {
	struct vfs_tree_node_t* co;
	struct vfs_tree_node_t* sub;

	const char* name;

	struct vfs_mount_t* mount;
};

static struct vfs_tree_node_t vfs_root;
static struct vfs_tree_node_t dev_root;

static struct hash_table_t* open_table;

static struct vfs_mount_t dev_mount = {
	.cntx = 0,
	.open = devfs_open,
	.close = devfs_close,
	.stat = devfs_stat,
	.read = devfs_read,
	.get_seek = devfs_get_seek,
	.seek = devfs_seek,
	.write = devfs_write,
	.create = devfs_create
};

static inline char* path_next(char* path, size_t* len) {
	*len = 0;

	while (*path && *path != '/') {
		path++;
		(*len)++;
	}

	if (*path) {
		return path + 1;
	}

	return path;
}

static struct vfs_open_file_t* lookup_register(char* path) {
	uint64_t key = fnv64_1a(path, kstrlen(path));
	struct vfs_open_file_t* file;

	while ((file = hash_table_get(open_table, key))) {
		if (kstrcmp(path, file->path) == 0) {
			break;
		}

		key++;
	}

	if (!file) {
		file = kmalloc(sizeof(struct vfs_open_file_t));

		file->path = kmalloc(kstrlen(path) + 1);
		kstrcpy((char*)file->path, path);

		file->refs = 0;
		file->key = key;
		lock_init(&file->lock);

		hash_table_insert(open_table, key, file);
	}

	file->refs++;

	return file;
}

static void lookup_close(struct vfs_open_file_t* file) {
	file->refs--;

	if (file->refs) {
		return;
	}

	kfree((char*)file->path);
	hash_table_remove(open_table, file->key);

	kfree(file);
}

void fs_init(void) {
	fs_sem = semaphore_alloc(SEMAPHORE_CAP_UNLIM);

	vfs_root.co = 0;
	vfs_root.sub = &dev_root;
	vfs_root.name = "";
	vfs_root.mount = 0;

	dev_root.co = 0;
	dev_root.sub = 0;
	dev_root.name = "dev";
	dev_root.mount = &dev_mount;

	open_table = hash_table_alloc(OPEN_TABLE_BUCKETS);
}

enum file_status_t fs_mount(
		const char* mountpoint,
		struct mount_cntx_t* cntx,
		fs_open_t open,
		fs_close_t close,
		fs_stat_t stat,
		fs_read_t read,
		fs_get_seek_t get_seek,
		fs_seek_t seek,
		fs_write_t write,
		fs_create_t create
		) {

	if (kstrcmp(mountpoint, "") && !vfs_root.mount) {
		vfs_root.mount = kmalloc(sizeof(struct vfs_mount_t));

		if (!vfs_root.mount) {
			logging_log_error("Failed to mount root");
			return FILE_ERROR;
		}

		vfs_root.mount->cntx = cntx;
		vfs_root.mount->open = open;
		vfs_root.mount->close = close;
		vfs_root.mount->stat = stat;
		vfs_root.mount->read = read;
		vfs_root.mount->get_seek = get_seek;
		vfs_root.mount->seek = seek;
		vfs_root.mount->write = write;
		vfs_root.mount->create = create;

		return FILE_OK;
	}

	//TODO: support non root mount
	logging_log_error("Can only mount root");
	return FILE_ERROR;
}

static char* find_mount(const char* path, struct vfs_mount_t** mount_out, void** clean_path_out, char** path_write_out) {
	struct vfs_tree_node_t* node = &vfs_root, * walk = 0;
	char* mount_path = (char*)"";
	struct vfs_mount_t* mount = 0;

	size_t len = kstrlen(path);
	char* clean_path = kmalloc(len + 1);

	uint64_t num_chars = 0;
	uint64_t skip = 0;
	uint8_t dot_only = 1;

	const char* path_read = path + len;
	char* path_write = clean_path + len;

	for (; path_read >= path; --path_read) {
		switch (*path_read) {
			case '/': // absolute paths must start with /
				if (dot_only && num_chars == 1) {
					path_write += num_chars;
				}
				else if (dot_only && num_chars == 2) {
					skip++;
					path_write += num_chars;
				}
				else if (num_chars > 0) {
					if (skip) {
						path_write += num_chars;
						skip--;
					}
					else {
						*path_write = '/';
						path_write--;
					}
				}

				num_chars = 0;
				dot_only = 1;
				break;
			default:
				dot_only = 0;
				__attribute__((fallthrough));
			case '.':
				num_chars++;
				__attribute__((fallthrough));
			case 0:
				*path_write = *path_read;
				path_write--;
				break;
		}
	}

	path_write++; // one to rewind write pointer

	if (*path_write == '/') {
		path_write++; // one to skip /
	}

	*path_write_out = path_write;

	semaphore_wait(fs_sem);

	do {
		if (node->mount) {
			mount_path = path_write;
			mount = node->mount;
		}

		path_read = path_write;
		path_write = path_next(path_write, &len);

		for (walk = node->sub; walk; walk = walk->co) {
			if (kstrlen(walk->name) == len && kmemcmp(walk->name, path_read, len) == 0) {
				node = walk;
				break;
			}
		}

	} while (walk && node == walk);

	semaphore_signal(fs_sem);

	*mount_out = mount;
	*clean_path_out = clean_path;

	return mount_path;
}

struct fs_handle_t* fs_open(const char* path) {
	struct vfs_mount_t* mount;
	void* clean_path;
	char* path_write;
	char* mount_path = find_mount(path, &mount, &clean_path, &path_write);

	if (!mount) {
		kfree(clean_path);
		return 0;
	}

	struct file_handle_t* handle = mount->open(mount->cntx, mount_path);

	if (!handle) {
		return 0;
	}

	semaphore_wait(fs_sem);
	struct vfs_open_file_t* open_file = lookup_register(path_write);
	semaphore_signal(fs_sem);

	kfree(clean_path);

	struct fs_handle_t* fs_handle = kmalloc(sizeof(struct fs_handle_t));
	fs_handle->handle = handle;
	fs_handle->mount = mount;
	fs_handle->shared = open_file;
	return fs_handle;
}

void fs_close(struct fs_handle_t* handle) {

	lock_acquire(&handle->shared->lock);
	handle->mount->close(handle->handle);
	lock_release(&handle->shared->lock);

	semaphore_wait_full(fs_sem);
	lookup_close(handle->shared);
	semaphore_signal_full(fs_sem);

	kfree(handle);
}

enum file_status_t fs_stat(struct fs_handle_t* handle, struct file_info_t* info) {

	lock_acquire(&handle->shared->lock);
	enum file_status_t ret = handle->mount->stat(handle->handle, info);
	lock_release(&handle->shared->lock);

	return ret;
}

size_t fs_read(struct fs_handle_t* handle, void* buffer, size_t count) {
	size_t ret;

	lock_acquire(&handle->shared->lock);
	ret = handle->mount->read(handle->handle, buffer, count);
	lock_release(&handle->shared->lock);

	return ret;
}

uint64_t fs_get_seek(struct fs_handle_t* handle) {
	uint64_t seek;

	lock_acquire(&handle->shared->lock);
	seek = handle->mount->get_seek(handle->handle);
	lock_release(&handle->shared->lock);

	return seek;
}

enum file_status_t fs_seek(struct fs_handle_t* handle, uint64_t seek) {
	enum file_status_t sts;

	lock_release(&handle->shared->lock);
	sts = handle->mount->seek(handle->handle, seek);
	lock_release(&handle->shared->lock);

	return sts;
}

size_t fs_write(struct fs_handle_t* handle, void* buffer, size_t count) {
	size_t ret;

	lock_release(&handle->shared->lock);
	ret = handle->mount->write(handle->handle, buffer, count);
	lock_release(&handle->shared->lock);

	return ret;
}

enum file_status_t fs_create(const char* path) {
	enum file_status_t sts;

	struct vfs_mount_t* mount;
	void* clean_path;
	char* path_write;
	char* mount_path = find_mount(path, &mount, &clean_path, &path_write);

	if (!mount) {
		kfree(clean_path);
		return FILE_DNE;
	}

	sts = mount->create(mount->cntx, mount_path);

	kfree(clean_path);

	return sts;
}
