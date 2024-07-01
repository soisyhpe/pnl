// SPDX-License-Identifier: GPL-2.0
#include <linux/file.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/buffer_head.h>

#include "ouichefs.h"

#define READ_CMD _IOR(MAGIC_NUM, 0, char *)

long fileinfo_ioctl(struct file *device_file, unsigned int cmd,
		    unsigned long arg)
{
	switch (cmd) {
	case READ_CMD: {
		if ((int)arg == -1)
			return -EBADF;

		struct fd fd = fdget(arg);
		struct file *file = fd.file;
		struct inode *inode = file->f_inode;

		inode_lock_shared(inode);

		struct super_block *sb = inode->i_sb;
		struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);

		struct buffer_head *bh;
		struct ouichefs_file_index_block *index;

		int block_index, block_size, block_number, waste,
			partially_filled_blocks = 0;
		u64 internal_fragmentation_total = 0;

		// Afficher le nombre de blocs utilisés
		pr_info("used_blocks = %llu\n", (u64)inode->i_blocks - 1);

		bh = sb_bread(sb, ci->index_block);
		if (!bh) {
			pr_debug("error when loading buffer\n");
			inode_unlock_shared(inode);
			return -EIO;
		}

		index = (struct ouichefs_file_index_block *)bh->b_data;

		// pr_debug("avant for");
		for (int i = 0; i < inode->i_blocks - 1; i++) {
			block_index = index->blocks[i];
			block_size = get_block_size(block_index);
			block_number = get_block_number(block_index);

			waste = OUICHEFS_MAX_BLOCK_SIZE - block_size;
			if (waste != 0) {
				partially_filled_blocks++;
				internal_fragmentation_total += waste;
			}

			pr_info("number = %d, size = %d", block_number,
				block_size);
		}

		pr_info("partially filled blocks = %d",
			partially_filled_blocks);
		pr_info("internal fragmentation = %llu bytes\n",
			internal_fragmentation_total);

		brelse(bh);
		inode_unlock_shared(inode);

		return 0;
	}
	default:
		return -ENOTTY;
	}
	return -1;
}

const struct file_operations fileinfo_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = fileinfo_ioctl,
};

int fileinfo_major;
char *fileinfo_device = "fileinfo";

void load_fileinfo_ioctl(void)
{
	fileinfo_major = register_chrdev(0, fileinfo_device, &fileinfo_fops);
	// MKDEV(fileinfo_major, 0);
}

void unload_fileinfo_ioctl(void)
{
	unregister_chrdev(fileinfo_major, fileinfo_device);
}
