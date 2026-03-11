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

#define VFS_STATUS_CONSISTENT	0x1
#define VFS_STATUS_MOUNTED		0x2

struct fs_mount_t {
	struct fs_mount_t* parent;
	fs_stat_t stat;
	fs_dirst_t dirst;
	fs_dirls_t dirls;
	fs_diren_t diren;
	fs_create_t create;
};

struct fs_node_t {
	struct fs_node_t* parent;
	struct fs_node_t* sub;
	struct fs_node_t* co;

	struct fs_mount_t* mount;

	struct fs_file_handle_t* handle;

	uint64_t accesses;

	char* name;

	struct fs_info_t info;

	uint8_t name_len;
	uint8_t status;
};

struct fs_file_t {
	struct fs_node_t* node;
	struct fs_info_t info;
	union {
		uint64_t off;
		struct fs_node_t* dir;
	} seek;
};

enum node_res_t {
	NODE_RES_TRUE,
	NODE_RES_FALSE,
	NODE_RES_LAST,
	NODE_RES_PARENT,
};

static enum fs_status_t vfs_stat(struct fs_file_handle_t* handle, struct fs_info_t* info) {
	(void)handle;

	info->size = 0;
	info->type = FS_TYPE_DIR;

	return FS_STATUS_OK;
}

static struct fs_dirls_handle_t* vfs_dirst(struct fs_file_handle_t* handle) {
	struct fs_node_t* node = (struct fs_node_t*)handle;
	return (struct fs_dirls_handle_t*)node->sub;
}

static enum fs_status_t vfs_dirls(struct fs_dirls_handle_t** handle, struct fs_ls_info_t* file) {
	if (!handle) {
		return FS_STATUS_EOF;
	}

	struct fs_node_t* walk = (struct fs_node_t*)*handle;

	file->handle = (struct fs_file_handle_t*)walk;
	kmemcpy(file->name, walk->name, walk->name_len);
	file->name_len = walk->name_len;

	*handle = (struct fs_dirls_handle_t*)walk->co;

	return FS_STATUS_OK;
}

static void vfs_diren(struct fs_dirls_handle_t* handle) {
	(void)handle;
}

static struct fs_file_handle_t* vfs_create(
		struct fs_file_handle_t* handle,
		enum fs_file_type_t type,
		const char* name,
		uint8_t name_len) {
	(void)handle;
	(void)type;
	(void)name;
	(void)name_len;

	logging_log_error("Attempted to create virtual file %s (%u)", name, type);
	return 0;
}

static struct fs_mount_t vfs_mount = {
	.parent = &vfs_mount,
	.stat = vfs_stat,
	.dirst = vfs_dirst,
	.dirls = vfs_dirls,
	.diren = vfs_diren,
	.create = vfs_create,
};

static uint8_t vfs_lock;
static struct fs_node_t vfs_root;

void fs_init(void) {
	lock_init(&vfs_lock);

	vfs_root.sub = 0;
	vfs_root.co = 0;
	vfs_root.parent = &vfs_root;
	vfs_root.mount = &vfs_mount;
	vfs_root.name = (char*)"";
	vfs_root.status = VFS_STATUS_CONSISTENT;
	vfs_root.name_len = 0;
	vfs_root.handle = (struct fs_file_handle_t*)&vfs_root;
	vfs_root.accesses = 1;
	vfs_root.info.size = 0;
	vfs_root.info.type = FS_TYPE_DIR;
}

static inline struct fs_node_t* init_node(
		struct fs_node_t* node,
		struct fs_file_handle_t* handle,
		const char* name,
		uint8_t name_len) {
	enum fs_status_t status;
	struct fs_info_t info;

	if ((status = node->mount->stat(handle, &info)) != FS_STATUS_OK) {
		return 0;
	}

	struct fs_node_t* sub = kmalloc(sizeof(struct fs_node_t));

	sub->accesses = 0;
	sub->co = 0;
	sub->handle = handle;
	sub->mount = node->mount;
	sub->name_len = name_len;
	sub->name = kmalloc(name_len + 1);
	kmemcpy(sub->name, name, name_len);
	sub->name[name_len] = 0;
	sub->parent = node;
	sub->status = 0;
	sub->sub = 0;
	sub->info = info;

	return sub;
}

static inline enum fs_status_t assert_consistent(struct fs_node_t* node, uint8_t clear) {
	if (node->status & VFS_STATUS_CONSISTENT) {
		return FS_STATUS_OK;
	}

	if (node->sub && clear) {
		logging_log_error("Detected corrupt vfs state. Inconsistent non-leaf node");
		panic(PANIC_STATE);
	}	

	struct fs_ls_info_t file;
	struct fs_dirls_handle_t* handle = node->mount->dirst(node->handle);

	if (!handle) {
		return FS_STATUS_ERROR;
	}

	while (node->mount->dirls(&handle, &file) == FS_STATUS_OK) {
		if (!file.valid) {
			continue;
		}
		
		struct fs_node_t* temp = init_node(node, file.handle, file.name, file.name_len);
		if (!temp) {
			node->mount->diren(handle);
			return FS_STATUS_ERROR;
		}

		temp->co = node->sub;
		node->sub = temp;
	}

	node->mount->diren(handle);

	node->status |= VFS_STATUS_CONSISTENT;

	return FS_STATUS_OK;
}

static inline const char* simplify_path(const char* path) {
	while (
			*path == '/' ||
			(*path == '.' && (*(path+1) == '/' || !*(path+1)))) {
		path++;
	}

	return path;
}

static inline const char* check_path(struct fs_node_t* node, const char* path, enum node_res_t* res) {
	uint8_t i = 0;
	const char* start;

	for (start = path; *path && *path != '/'; path++) {
		i++;
	}
	
	path = simplify_path(path);

	*res = NODE_RES_FALSE;

	if (i == node->name_len && !kmemcmp(node->name, start, i)) {
		if (!*path) {
			*res = NODE_RES_LAST;
		}
		else {
			*res = NODE_RES_TRUE;
		}
	}

	if (i == 2 && !kmemcmp("..", start, i)) {
		*res = NODE_RES_PARENT;
	}

	return path;
}

static struct fs_node_t* resolve_path(struct fs_node_t* root, const char* path) {
	enum node_res_t res;
	struct fs_node_t* node, * walk;

	path = check_path(root, path, &res);

	switch (res) {
		case NODE_RES_LAST:
			root->accesses++;
			return root;

		case NODE_RES_PARENT:
			node = resolve_path(root->parent, path);
			if (node) {
				root->parent->accesses--;
			}
			return node;

		case NODE_RES_TRUE:
			if (root->info.type != FS_TYPE_DIR) {
				return 0;
			}

			if (assert_consistent(root, 1) != FS_STATUS_OK) {
				return 0;
			}

			for (walk = root->sub; walk; walk = walk->co) {
				node = resolve_path(walk, path);
				if (node) {
					root->accesses++;
					return node;
				}
			}
		__attribute__((fallthrough));
		case NODE_RES_FALSE:
		default:
			return 0;
	}
}

static void cleanup_path(struct fs_node_t* root, const char* path) {
	//TODO: cleanup vfs tree
	(void)root;
	(void)path;
}

extern enum fs_status_t fs_mount(
		struct fs_file_handle_t* root,
		const char* path,
		fs_stat_t stat,
		fs_dirst_t dirst,
		fs_dirls_t dirls,
		fs_diren_t diren,
		fs_create_t create) {

	lock_acquire(&vfs_lock);

	struct fs_node_t* node = resolve_path(&vfs_root, path);

	if (node->status & VFS_STATUS_MOUNTED) {
		return FS_STATUS_BUSY;
	}

	struct fs_node_t revert;
	revert = *node;
	struct fs_mount_t* mount = kmalloc(sizeof(struct fs_mount_t));
	mount->parent = node->mount;
	mount->stat = stat;
	mount->dirst = dirst;
	mount->dirls = dirls;
	mount->diren = diren;
	mount->create = create;

	node->mount = mount;
	node->handle = root;
	node->status = VFS_STATUS_MOUNTED;

	struct fs_info_t info;
	enum fs_status_t status = node->mount->stat(node->handle, &info);
	if (status != FS_STATUS_OK) {
		kfree(mount);
		*node = revert;
		return status;
	}

	node->info = info;
	
	if ((status = assert_consistent(node, 0)) != FS_STATUS_OK) {
		kfree(mount);
		*node = revert;
		return status;
	}	

	lock_release(&vfs_lock);
	return FS_STATUS_OK;
}

struct fs_file_t* fs_open(const char* path) {
	lock_acquire(&vfs_lock);
	struct fs_node_t* node = resolve_path(&vfs_root, simplify_path(path));
	lock_release(&vfs_lock);

	if (!node) {
		return 0;
	}

	struct fs_file_t* file = kmalloc(sizeof(struct fs_file_t));
	file->node = node;
	file->seek.off = 0;
	return file;
}

void fs_close(struct fs_file_t* file) {
	lock_acquire(&vfs_lock);
	cleanup_path(&vfs_root, simplify_path(file->node->name));
	lock_release(&vfs_lock);

	kfree(file);
}

enum fs_status_t fs_stat(struct fs_file_t* file, struct fs_info_t* info) {
	*info = file->node->info;
	return FS_STATUS_OK;
}

enum fs_status_t fs_list(struct fs_file_t* file, char* buffer) {

	if (!file->seek.dir) {
		lock_acquire(&vfs_lock);
		file->seek.dir = file->node->sub;
		lock_release(&vfs_lock);
	}

	if (!file->seek.dir) {
		return FS_STATUS_EOF;
	}

	kmemcpy(buffer, file->seek.dir->name, file->seek.dir->name_len);
	buffer[file->seek.dir->name_len] = 0;

	lock_acquire(&vfs_lock);
	file->seek.dir = file->seek.dir->co;
	lock_release(&vfs_lock);

	return FS_STATUS_OK;
}

enum fs_status_t fs_create(struct fs_file_t* file, enum fs_file_type_t type, const char* name) {
	uint8_t name_len;
	for (name_len = 0; name[name_len]; name_len++); //TODO: use strlen

	lock_acquire(&vfs_lock);
	struct fs_file_handle_t* handle = file->node->mount->create(file->node->handle, type, name, name_len);

	if (!handle) {
		lock_release(&vfs_lock);
		return FS_STATUS_ERROR;
	}

	struct fs_node_t* node = init_node(file->node, handle, name, name_len);
	node->co = file->node->sub;
	file->node->sub = node;

	lock_release(&vfs_lock);
	return FS_STATUS_OK;
}

#ifdef DEBUG

static void fs_log_tree(struct fs_node_t* node, char* prefix, size_t prefix_len) {
	logging_log_debug("vfs tree: %s%256s (%u/0x%lx)", prefix, node->name, node->info.type, node->info.size);

	if (node->info.type != FS_TYPE_DIR) {
		return;
	}

	if (assert_consistent(node, 1) != FS_STATUS_OK) {
		logging_log_error("Failed to log vfs tree");
		return;
	}

	char* new_prefix = kmalloc(prefix_len + 2);
	kmemcpy(new_prefix+2, prefix, prefix_len);
	new_prefix[0] = new_prefix[1] = ' ';

	for (struct fs_node_t* walk = node->sub; walk; walk = walk->co) {
		fs_log_tree(walk, new_prefix, prefix_len + 2);
	}

	kfree(new_prefix);
}

void fs_log_vfs_tree(void) {
	lock_acquire(&vfs_lock);

	fs_log_tree(&vfs_root, (char*)"", 1);	

	lock_release(&vfs_lock);
}

#endif /* DEBUG */
