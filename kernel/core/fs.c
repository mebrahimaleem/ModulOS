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

static struct semaphore_t* fs_sem;

struct fs_mount_t {
	struct mount_cntx_t* cntx;

	fs_open_t open;
	fs_close_t close;
	fs_stat_t stat;
	fs_read_t read;
	fs_get_seek_t get_seek;
	fs_seek_t seek;
	fs_write_t write;
	fs_delete_final_t delete_final;
	fs_open_dir_t open_dir;
	fs_read_dir_t read_dir;
	fs_create_dir_t create_dir;
	fs_delete_dir_t delete_dir;
	fs_is_interactive_t is_interactive;
	fs_truncate_t truncate;
	fs_link_t link;
	fs_unlink_t unlink;
	fs_dup_t dup;
	fs_next_dir_t next_dir;
};

struct fs_open_file_t {
	char* path;
	uint64_t refs;
	uint64_t key;
	uint8_t lock;
	uint8_t pending_delete;
};

struct fs_handle_t {
	struct fs_mount_t* mount;
	struct file_handle_t* handle;
	struct fs_open_file_t* shared;
	uint32_t flags;
};

struct fs_tree_node_t {
	struct fs_tree_node_t* co;
	struct fs_tree_node_t* sub;

	char* name;

	struct fs_mount_t* mount;
};

static struct fs_tree_node_t fs_root;

static struct hash_table_t* open_table;

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

static void fs_reopen_file(struct fs_open_file_t* file) {
	file->refs++;
}

static struct fs_open_file_t* lookup_register(char* path) {
	size_t path_len = kstrlen(path);
	uint64_t key = fnv64_1a(path, path_len);
	struct fs_open_file_t* file = 0;

	while (hash_table_get(open_table, key, (void**)&file)) {
		if (kstrcmp(path, file->path + 1) == 0) {
			break;
		}

		key++;
	}

	if (!file) {
		file = kmalloc(sizeof(struct fs_open_file_t));

		file->path = kmalloc(path_len + 2);
		file->path[0] = '/';
		kstrcpy(file->path + 1, path);

		file->refs = 0;
		file->key = key;
		file->pending_delete = 0;
		lock_init(&file->lock);

		hash_table_insert(open_table, key, file);
	}

	fs_reopen_file(file);

	return file;
}

static uint8_t lookup_close(struct fs_open_file_t* file) {
	file->refs--;

	if (file->refs) {
		return 0;
	}

	if (file->path) {
		kfree(file->path);
		void* ign;
		hash_table_remove(open_table, file->key, &ign);
	}

	uint8_t pending_delete = file->pending_delete;

	kfree(file);
	return pending_delete;
}

void fs_init(void) {
	fs_sem = semaphore_alloc(SEMAPHORE_CAP_UNLIM);

	fs_root.co = 0;
	fs_root.sub = 0;
	fs_root.name = kmalloc(2);
	kstrcpy(fs_root.name, "");
	fs_root.mount = 0;

	open_table = hash_table_alloc(OPEN_TABLE_BUCKETS);

	devfs_init();
}

static struct file_handle_t* fs_open_stub(struct mount_cntx_t*, const char*, uint32_t, uint32_t) {
	return 0;
}

static void fs_close_stub(struct file_handle_t*) {
	return;
}

static enum file_status_t fs_stat_stub(struct file_handle_t*, struct file_info_t*) {
	return FILE_NO_SUPPORT;
}

static size_t fs_read_stub(struct file_handle_t*, void*, size_t) {
	return 0;
}

static uint64_t fs_get_seek_stub(struct file_handle_t*) {
	return 0;
}

static enum file_status_t fs_seek_stub(struct file_handle_t*, uint64_t) {
	return 0;
}

static size_t fs_write_stub(struct file_handle_t*, const void*, size_t) {
	return 0;
}

static void fs_delete_final_stub(struct file_handle_t*) {
	return;
}

static enum file_status_t fs_open_dir_stub(struct file_handle_t*) {
	return FILE_NO_SUPPORT;
}

static enum file_status_t fs_read_dir_stub(struct file_handle_t*, struct dir_info_t*) {
	return FILE_NO_SUPPORT;
}

static enum file_status_t fs_next_dir_stub(struct file_handle_t*) {
	return FILE_NO_SUPPORT;
}

static enum file_status_t fs_create_dir_stub(struct file_handle_t*) {
	return FILE_NO_SUPPORT;
}

static enum file_status_t fs_delete_dir_stub(struct file_handle_t*) {
	return FILE_NO_SUPPORT;
}

static enum file_status_t fs_truncate_stub(struct file_handle_t*, size_t) {
	return FILE_NO_SUPPORT;
}

static enum file_status_t fs_link_stub(struct file_handle_t*, struct file_handle_t*) {
	return FILE_NO_SUPPORT;
}

static enum file_status_t fs_unlink_stub(struct file_handle_t*) {
	return FILE_NO_SUPPORT;
}

static struct file_handle_t* fs_dup_stub(struct file_handle_t*) {
	return 0;
}

static uint8_t fs_is_interactive_stub(struct file_handle_t*) {
	return 0;
}

struct fs_mount_t* fs_mount(const char* mountpoint, struct mount_cntx_t* cntx) {
	//TODO: support non root mounts

	mountpoint++; // remove prefix /

	struct fs_mount_t* mount = kmalloc(sizeof(struct fs_mount_t));

	if (!mount) {
		return 0;
	}

	semaphore_wait_full(fs_sem);
	if (!kstrcmp(mountpoint, "")) {
		// root mount

		if (fs_root.mount) {
			// busy
			semaphore_signal_full(fs_sem);
			return 0;
		}

		fs_root.mount = mount;
	}
	else {
		//TODO: check for mounting on busy mountpoints

		struct fs_tree_node_t* node = kmalloc(sizeof(struct fs_tree_node_t));

		node->sub = 0;
		node->mount = mount;
		node->name = kmalloc(kstrlen(mountpoint) + 1);
		kstrcpy(node->name, mountpoint);
		node->co = fs_root.sub;
		fs_root.sub = node;
	}

	mount->cntx = cntx;
	mount->open = fs_open_stub;
	mount->close = fs_close_stub;
	mount->stat = fs_stat_stub;
	mount->read = fs_read_stub;
	mount->get_seek = fs_get_seek_stub;
	mount->seek = fs_seek_stub;
	mount->write = fs_write_stub;
	mount->delete_final = fs_delete_final_stub;
	mount->open_dir = fs_open_dir_stub;
	mount->read_dir = fs_read_dir_stub;
	mount->create_dir = fs_create_dir_stub;
	mount->delete_dir = fs_delete_dir_stub;
	mount->truncate = fs_truncate_stub;
	mount->link = fs_link_stub;
	mount->unlink = fs_unlink_stub;
	mount->dup = fs_dup_stub;
	mount->next_dir = fs_next_dir_stub;
	mount->is_interactive = fs_is_interactive_stub;

	semaphore_signal_full(fs_sem);

	return mount;
}

void fs_mount_assign_open(struct fs_mount_t* mount, fs_open_t open) {
	mount->open = open;
}

void fs_mount_assign_close(struct fs_mount_t* mount, fs_close_t close) {
	mount->close = close;
}

void fs_mount_assign_stat(struct fs_mount_t* mount, fs_stat_t stat) {
	mount->stat = stat;
}

void fs_mount_assign_read(struct fs_mount_t* mount, fs_read_t read) {
	mount->read = read;
}

void fs_mount_assign_get_seek(struct fs_mount_t* mount, fs_get_seek_t get_seek) {
	mount->get_seek = get_seek;
}

void fs_mount_assign_seek(struct fs_mount_t* mount, fs_seek_t seek) {
	mount->seek = seek;
}

void fs_mount_assign_write(struct fs_mount_t* mount, fs_write_t write) {
	mount->write = write;
}

void fs_mount_assign_delete_final(struct fs_mount_t* mount, fs_delete_final_t delete_final) {
	mount->delete_final = delete_final;
}

void fs_mount_assign_open_dir(struct fs_mount_t* mount, fs_open_dir_t open_dir) {
	mount->open_dir = open_dir;
}

void fs_mount_assign_read_dir(struct fs_mount_t* mount, fs_read_dir_t read_dir) {
	mount->read_dir = read_dir;
}

void fs_mount_assign_create_dir(struct fs_mount_t* mount, fs_create_dir_t create_dir) {
	mount->create_dir = create_dir;
}

void fs_mount_assign_delete_dir(struct fs_mount_t* mount, fs_delete_dir_t delete_dir) {
	mount->delete_dir = delete_dir;
}

void fs_mount_assign_truncate(struct fs_mount_t* mount, fs_truncate_t truncate) {
	mount->truncate = truncate;
}

void fs_mount_assign_link(struct fs_mount_t* mount, fs_link_t link) {
	mount->link = link;
}

void fs_mount_assign_unlink(struct fs_mount_t* mount, fs_unlink_t unlink) {
	mount->unlink = unlink;
}

void fs_mount_assign_dup(struct fs_mount_t* mount, fs_dup_t dup) {
	mount->dup = dup;
}

void fs_mount_assign_next_dir(struct fs_mount_t* mount, fs_next_dir_t next_dir) {
	mount->next_dir = next_dir;
}

void fs_mount_assign_is_interactive(struct fs_mount_t* mount, fs_is_interactive_t is_interactive) {
	mount->is_interactive = is_interactive;
}

static const char* find_mount(const char* path, struct fs_mount_t** mount_out, void** clean_path_out, char** path_write_out) {
	struct fs_tree_node_t* node = &fs_root, * walk = 0;
	const char* mount_path = "";
	struct fs_mount_t* mount = 0;

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

struct fs_handle_t* fs_open_mode(const char* path, uint32_t flags, uint32_t mode) {
	struct fs_mount_t* mount;
	void* clean_path;
	char* path_write;
	const char* mount_path = find_mount(path, &mount, &clean_path, &path_write);

	if (!mount) {
		kfree(clean_path);
		return 0;
	}

	struct file_handle_t* handle = mount->open(mount->cntx, mount_path, flags, mode);

	if (!handle) {
		return 0;
	}

	semaphore_wait(fs_sem);
	struct fs_open_file_t* open_file = lookup_register(path_write);
	semaphore_signal(fs_sem);

	kfree(clean_path);

	struct fs_handle_t* fs_handle = kmalloc(sizeof(struct fs_handle_t));
	fs_handle->handle = handle;
	fs_handle->mount = mount;
	fs_handle->shared = open_file;
	fs_handle->flags = flags;
	return fs_handle;
}

struct fs_handle_t* fs_anon(struct fs_mount_t* mount, struct file_handle_t* handle, uint32_t flags) {
	struct fs_open_file_t* open_file = kmalloc(sizeof(struct fs_open_file_t));

	open_file->path = 0;
	open_file->pending_delete = 0;
	open_file->refs = 1;
	lock_init(&open_file->lock);

	struct fs_handle_t* fs_handle = kmalloc(sizeof(struct fs_handle_t));
	fs_handle->handle = handle;
	fs_handle->mount = mount;
	fs_handle->shared = open_file;
	fs_handle->flags = flags;
	return fs_handle;
}

struct fs_handle_t* fs_open(const char* path, uint32_t flags) {
	return fs_open_mode(path, flags, 0);
}

struct fs_handle_t* fs_openat(const char* path, uint32_t flags, struct fs_handle_t* at, uint32_t mode) {

	if (*path == '/') {
		// absolute, no at
		return fs_open_mode(path, flags, mode);
	}
	else {
		if (!at->shared->path) {
			return 0;
		}

		size_t prefix_len = kstrlen(at->shared->path);
		size_t suffix_len = kstrlen(path);
		char* full_path = kmalloc(prefix_len + suffix_len + 2);
		kmemcpy(full_path, at->shared->path, prefix_len);
		full_path[prefix_len] = '/';
		kmemcpy(full_path + prefix_len + 1, path, suffix_len);
		full_path[prefix_len + suffix_len + 1] = 0;
		struct fs_handle_t* ret = fs_open_mode(full_path, flags, mode);
		kfree(full_path);
		return ret;
	}
}

void fs_close(struct fs_handle_t* handle) {
	semaphore_wait_full(fs_sem);
	if (lookup_close(handle->shared)) {
		handle->mount->delete_final(handle->handle);
	}
	semaphore_signal_full(fs_sem);

	lock_acquire(&handle->shared->lock);
	handle->mount->close(handle->handle);
	lock_release(&handle->shared->lock);

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

	if (handle->flags != O_RDONLY && handle->flags != O_RDWR) {
		return 0;
	}

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

	lock_acquire(&handle->shared->lock);
	sts = handle->mount->seek(handle->handle, seek);
	lock_release(&handle->shared->lock);

	return sts;
}

size_t fs_write(struct fs_handle_t* handle, const void* buffer, size_t count) {
	size_t ret;

	if (handle->flags != O_WRONLY && handle->flags != O_RDWR) {
		return 0;
	}

	lock_acquire(&handle->shared->lock);
	ret = handle->mount->write(handle->handle, buffer, count);
	lock_release(&handle->shared->lock);

	return ret;
}

struct fs_handle_t* fs_open_dir(struct fs_handle_t* handle) {
	enum file_status_t sts;

	lock_acquire(&handle->shared->lock);
	sts = handle->mount->open_dir(handle->handle);
	lock_release(&handle->shared->lock);

	if (sts != FILE_OK) {
		return 0;
	}

	return handle;
}

enum file_status_t fs_create_dir(struct fs_handle_t* handle) {
	enum file_status_t sts;

	lock_acquire(&handle->shared->lock);
	sts = handle->mount->create_dir(handle->handle);
	lock_release(&handle->shared->lock);

	return sts;
}

enum file_status_t fs_delete_dir(struct fs_handle_t* handle) {
	enum file_status_t sts;

	lock_acquire(&handle->shared->lock);
	sts = handle->mount->delete_dir(handle->handle);
	lock_release(&handle->shared->lock);

	return sts;
}

enum file_status_t fs_read_dir(struct fs_handle_t* handle, struct dir_info_t* info) {
	enum file_status_t sts;

	lock_acquire(&handle->shared->lock);
	sts = handle->mount->read_dir(handle->handle, info);
	lock_release(&handle->shared->lock);

	return sts;
}

enum file_status_t fs_next_dir(struct fs_handle_t* handle) {
	enum file_status_t sts;

	lock_acquire(&handle->shared->lock);
	sts = handle->mount->next_dir(handle->handle);
	lock_release(&handle->shared->lock);

	return sts;
}

enum file_status_t fs_truncate(struct fs_handle_t* handle, size_t size) {
	enum file_status_t sts;

	lock_acquire(&handle->shared->lock);
	sts = handle->mount->truncate(handle->handle, size);
	lock_release(&handle->shared->lock);

	return sts;
}

enum file_status_t fs_link(struct fs_handle_t* handle, struct fs_handle_t* replace) {
	enum file_status_t sts;

	if (handle->mount != replace->mount) {
		return FILE_BAD_FLAGS; // cannot create hardlink between filesystems
	}

	lock_acquire(&handle->shared->lock);
	sts = handle->mount->link(handle->handle, replace->handle);
	lock_release(&handle->shared->lock);

	return sts;
}

enum file_status_t fs_unlink(struct fs_handle_t* handle) {
	enum file_status_t sts;

	lock_acquire(&handle->shared->lock);
	sts = handle->mount->unlink(handle->handle);
	lock_release(&handle->shared->lock);

	return sts;

}

void fs_path(struct fs_handle_t* handle, size_t max_len, char* buf) {
	if (!handle->shared->path) {
		*buf = 0;
		return;
	}

	kstrncpy(buf, handle->shared->path, max_len);
}

uint8_t fs_is_interactive(struct fs_handle_t* handle) {
	return handle->mount->is_interactive(handle->handle);
}

struct fs_handle_t* fs_dup(struct fs_handle_t* handle) {
	struct file_handle_t* dup = handle->mount->dup(handle->handle);

	if (!dup) {
		return 0;
	}

	semaphore_wait(fs_sem);
	fs_reopen_file(handle->shared);
	semaphore_signal(fs_sem);

	struct fs_handle_t* fs_handle = kmalloc(sizeof(struct fs_handle_t));
	fs_handle->handle = dup;
	fs_handle->mount = handle->mount;
	fs_handle->shared = handle->shared;
	fs_handle->flags = handle->flags;
	return fs_handle;
}
