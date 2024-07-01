// SPDX-License-Identifier: GPL-2.0

/*
 * Ce fichier est responsable de la fragmentation.
 */

#include "net/dst_metadata.h"
#include <linux/file.h>

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "bitmap.h"
#include "ouichefs.h"

#define READ_CMD _IOR(DEFRAG_NUM, 0, char *)

/*
 * Implémentation de l'algorithme de défragmentation dont le pseudo-code,
 * les détails et la complexité sont discutés dans le rapport.
 */
void defrag(struct file *file)
{
	// Récupérer les informations de l'inode
	struct inode *inode = file->f_inode;
	struct ouichefs_inode_info *inode_info = OUICHEFS_INODE(inode);

	inode_lock(inode);

	// Récupérer les informations du super block
	struct super_block *sb = inode->i_sb;
	struct ouichefs_sb_info *sb_info = OUICHEFS_SB(sb);

	struct buffer_head *bh_index, *bh_i, *bh_j, *bh_new_block;
	struct ouichefs_file_index_block *index;

	bh_index = sb_bread(sb, inode_info->index_block);

	if (!bh_index) {
		pr_debug("error when loading buffer\n");
		goto unlock;
	}

	index = (struct ouichefs_file_index_block *)bh_index->b_data;

	int i, j = 1;

	// Le nombre de blocs effectifs
	int nr_blocks = inode->i_blocks - 1;

	// Le nombre d'octets restants
	int left_empty_bytes;

	// Le numéro du bloc qu'on partout
	int i_cur_block_number, j_cur_block_number;

	// La taille du bloc qu'on parcourt (i ou j)
	int i_cur_block_size, j_cur_block_size;

	// Le nombre d'octets qu'on doit déplacer vers le bloc i
	int bytes_to_move;

	// Le nombre d'octets restants à copier dans un bloc j
	int left_bytes;

	// Curseur des octets restants qu'on doit copier
	char *offset_left_bytes;

	// Nouveau bloc qu'on va allouer
	int new_block_number;

	// Taille précédente du bloc i
	int previous_size;

	// Nombre de blocs qu'on a désaoullé
	int nr_removed_blocks = 0;

	for (i = 0; i < nr_blocks; i = j) {
		// On récupère le nombre d'octets vides du bloc
		left_empty_bytes = OUICHEFS_MAX_BLOCK_SIZE -
				   get_block_size(index->blocks[i]);

		if (left_empty_bytes == 0) {
			pr_debug("Aucun octet restant");
			j = i + 1;
			continue;
		}

		i_cur_block_number = get_block_number(index->blocks[i]);
		bh_i = sb_bread(sb, i_cur_block_number);

		if (!bh_i) {
			pr_debug("Error when loading buffer");
			goto release_bh_index;
		}

		// On parcourt les blocs suivants.
		// Cela sert à trouver des données à rapprocher
		for (j = i + 1; j < nr_blocks; j++) {
			j_cur_block_number = get_block_number(index->blocks[j]);
			i_cur_block_size = get_block_size(index->blocks[i]);
			j_cur_block_size = get_block_size(index->blocks[j]);

			// Si la taille du bloc j est nulle
			if (j_cur_block_size == 0) {
				// On désalloue j
				put_block(sb_info, j_cur_block_number);
				nr_removed_blocks++;

				index->blocks[j] = 0;

				continue;
			}

			bytes_to_move = min(left_empty_bytes, j_cur_block_size);
			left_bytes = j_cur_block_size - bytes_to_move;
			bh_j = sb_bread(sb, j_cur_block_number);

			if (!bh_j) {
				pr_debug("Error when loading buffer");
				goto release_bh_i;
			}

			offset_left_bytes = bh_j->b_data + bytes_to_move;

			memcpy(bh_i->b_data + i_cur_block_size, bh_j->b_data,
			       bytes_to_move);

			previous_size = get_block_size(index->blocks[i]);
			set_block_size(&index->blocks[i],
				       previous_size + bytes_to_move);

			left_empty_bytes -= bytes_to_move;

			// S'il n'y a plus rien à copier dans ce j
			if (left_bytes == 0) {
				brelse(bh_j);
				put_block(sb_info, j_cur_block_number);
				nr_removed_blocks++;
				index->blocks[j] = 0;
				continue;
			}

			new_block_number = get_free_block(sb_info);
			bh_new_block = sb_bread(sb, new_block_number);

			if (!bh_new_block) {
				pr_debug("Error when loading buffer");
				goto release_bh_j;
			}

			memcpy(bh_new_block->b_data, offset_left_bytes,
			       left_bytes);
			set_block_size(&index->blocks[j], left_bytes);
			set_block_number(&index->blocks[j], new_block_number);
			put_block(sb_info, j_cur_block_number);

			mark_buffer_dirty(bh_new_block);
			sync_dirty_buffer(bh_new_block);
			brelse(bh_new_block);

			// Si le bloc i est plein
			if (get_block_size(index->blocks[i]) ==
			    OUICHEFS_MAX_BLOCK_SIZE) {
				break;
			}
		}

		mark_buffer_dirty(bh_i);
		sync_dirty_buffer(bh_i);
		brelse(bh_i);
	}

	// On met à jour
	inode->i_blocks -= nr_removed_blocks;

	mark_buffer_dirty(bh_index);
	sync_dirty_buffer(bh_index);
	brelse(bh_index);

	mark_inode_dirty(inode);

	/*
	 * On décale tous les blocs à gauche et on désallouer les blocs
	 * qu'on décale.
	 */

	int cur_block_size;

	for (i = 0; i < nr_blocks; i++) {
		cur_block_size = get_block_size(index->blocks[i]);

		if (cur_block_size != 0)
			continue;

		j = i;
		while (get_block_size(index->blocks[j]) == 0) {
			j++;

			if (j == nr_blocks - 1)
				goto metadata;
		}

		index->blocks[i] = index->blocks[j];

		put_block(sb_info, get_block_size(index->blocks[j]));
		index->blocks[j] = 0;
	}

	pr_debug("He ho, le bloc est complètement plein\n");

release_bh_j:
	brelse(bh_j);

release_bh_i:
	brelse(bh_i);

release_bh_index:
	brelse(bh_index);

unlock:
	inode_unlock(inode);
	return;

metadata:
	inode->i_mtime = current_time(inode);
	inode_unlock(inode);
}

long defrag_ioctl(struct file *device_file, unsigned int cmd, unsigned long arg)
{
	int error;

	switch (cmd) {
	case READ_CMD: {
		if ((int)arg == -1)
			return -EBADF;

		struct fd fd = fdget(arg);

		defrag(fd.file);

		error = 0;
		break;
	}
	default:
		error = -ENOTTY;
		break;
	}

	return error;
}

const struct file_operations defrag_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = defrag_ioctl,
};

int defrag_major;
char *defrag_device = "defrag";

void load_defrag_ioctl(void)
{
	defrag_major = register_chrdev(0, defrag_device, &defrag_fops);
	// MKDEV(defrag_major, 0);
}

void unload_defrag_ioctl(void)
{
	unregister_chrdev(defrag_major, defrag_device);
}
