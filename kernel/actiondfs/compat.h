/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef ACTIONDFS_COMPAT_H
#define ACTIONDFS_COMPAT_H

#include <linux/version.h>

/* These are the two API profiles maintained by this branch. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(7, 2, 0) && \
    LINUX_VERSION_CODE < KERNEL_VERSION(7, 3, 0)
#define ACTIONDFS_API_7_2 1
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 18, 0) && \
      LINUX_VERSION_CODE < KERNEL_VERSION(6, 19, 0)
#define ACTIONDFS_API_7_2 0
#else
#error "actiondfs: kernel API needs review (supported profiles: 6.18 and 7.2)"
#endif

static inline struct file *actiondfs_backing_file_open(
	const struct file *user_file, int flags, const struct path *path,
	const struct cred *cred)
{
#if ACTIONDFS_API_7_2
	return backing_file_open(user_file, flags, path, cred);
#else
	return backing_file_open(file_user_path(user_file), flags, path, cred);
#endif
}

static inline unsigned long actiondfs_inode_state(struct inode *inode)
{
#if ACTIONDFS_API_7_2
	return inode_state_read_once(inode);
#else
	return inode->i_state;
#endif
}

static inline struct dentry *actiondfs_vfs_mkdir(struct mnt_idmap *idmap,
	struct inode *dir, struct dentry *dentry, umode_t mode)
{
#if ACTIONDFS_API_7_2
	return vfs_mkdir(idmap, dir, dentry, mode, NULL);
#else
	return vfs_mkdir(idmap, dir, dentry, mode);
#endif
}

static inline int actiondfs_vfs_create(struct mnt_idmap *idmap,
	struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
#if ACTIONDFS_API_7_2
	return vfs_create(idmap, dentry, mode, NULL);
#else
	return vfs_create(idmap, dir, dentry, mode, excl);
#endif
}

static inline int actiondfs_vfs_symlink(struct mnt_idmap *idmap,
	struct inode *dir, struct dentry *dentry, const char *target)
{
#if ACTIONDFS_API_7_2
	return vfs_symlink(idmap, dir, dentry, target, NULL);
#else
	return vfs_symlink(idmap, dir, dentry, target);
#endif
}

static inline int actiondfs_vfs_rmdir(struct mnt_idmap *idmap,
	struct inode *dir, struct dentry *dentry)
{
#if ACTIONDFS_API_7_2
	return vfs_rmdir(idmap, dir, dentry, NULL);
#else
	return vfs_rmdir(idmap, dir, dentry);
#endif
}

struct actiondfs_rename_context {
	struct renamedata data;
#if !ACTIONDFS_API_7_2
	struct dentry *trap;
#endif
};

static inline void actiondfs_end_renaming(struct actiondfs_rename_context *ctx)
{
	struct renamedata *rd = &ctx->data;

#if ACTIONDFS_API_7_2
	end_renaming(rd);
#else
	dput(rd->new_dentry);
	dput(rd->old_dentry);
	unlock_rename(rd->new_parent, rd->old_parent);
#endif
}

static inline int actiondfs_start_renaming(struct actiondfs_rename_context *ctx,
	struct qstr *old_name, struct qstr *new_name)
{
	struct renamedata *rd = &ctx->data;

#if ACTIONDFS_API_7_2
	return start_renaming(rd, 0, old_name, new_name);
#else
	int err;

	ctx->trap = lock_rename(rd->new_parent, rd->old_parent);
	if (IS_ERR(ctx->trap))
		return PTR_ERR(ctx->trap);
	rd->old_dentry = lookup_one(rd->mnt_idmap, old_name, rd->old_parent);
	if (IS_ERR(rd->old_dentry)) {
		err = PTR_ERR(rd->old_dentry);
		rd->old_dentry = NULL;
		goto fail;
	}
	rd->new_dentry = lookup_one(rd->mnt_idmap, new_name, rd->new_parent);
	if (IS_ERR(rd->new_dentry)) {
		err = PTR_ERR(rd->new_dentry);
		rd->new_dentry = NULL;
		goto fail;
	}
	return 0;
fail:
	actiondfs_end_renaming(ctx);
	return err;
#endif
}

static inline int actiondfs_check_rename_cycle(struct actiondfs_rename_context *ctx)
{
#if !ACTIONDFS_API_7_2
	if (ctx->data.old_dentry == ctx->trap)
		return -EINVAL;
	if (ctx->data.new_dentry == ctx->trap)
		return -ENOTEMPTY;
#endif
	/* Linux 7.2's start_renaming() performs the ancestry checks. */
	return 0;
}

#endif
