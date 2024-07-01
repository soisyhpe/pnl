// SPDX-License-Identifier: GPL-2.0
/*
 * ouiche_fs - a simple educational filesystem for Linux
 *
 * Copyright (C) 2018 Redha Gouicem <redha.gouicem@lip6.fr>
 */

#define CEIL(a, b) (((a) / (b)) + (((a) % (b)) != 0))
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__

#include "asm-generic/errno-base.h"
#include "asm-generic/int-ll64.h"
#include "linux/bitops.h"
#include "linux/mm_types.h"
#include "linux/net_namespace.h"
#include "linux/sbitmap.h"

#include "linux/types.h"
#include "linux/uaccess.h"
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mpage.h>
#include <linux/minmax.h>

#include "bitmap.h"
#include "ouichefs.h"

inline void pr_separator(char *title)
{
	pr_debug("---------- %s ----------\n", title);
}

/*
 * Map the buffer_head passed in argument with the iblock-th block of the file
 * represented by inode. If the requested block is not allocated and create is
 * true, allocate a new block on disk and map it.
 */
static int ouichefs_file_get_block(struct inode *inode, sector_t iblock,
				   struct buffer_head *bh_result, int create)
{
	struct super_block *sb = inode->i_sb;
	struct ouichefs_sb_info *sbi = OUICHEFS_SB(sb);
	struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);
	struct ouichefs_file_index_block *index;
	struct buffer_head *bh_index;
	int ret = 0, bno;

	/* If block number exceeds filesize, fail */
	if (iblock >= OUICHEFS_BLOCK_SIZE >> 2)
		return -EFBIG;

	/* Read index block from disk */
	bh_index = sb_bread(sb, ci->index_block);
	if (!bh_index)
		return -EIO;
	index = (struct ouichefs_file_index_block *)bh_index->b_data;

	/*
	 * Check if iblock is already allocated. If not and create is true,
	 * allocate it. Else, get the physical block number.
	 */

	if (index->blocks[iblock] == 0) {
		if (!create) {
			ret = 0;
			goto brelse_index;
		}
		bno = get_free_block(sbi);
		if (!bno) {
			ret = -ENOSPC;
			goto brelse_index;
		}
		index->blocks[iblock] = bno;
	} else {
		bno = index->blocks[iblock];
	}

	/* Map the physical block to the given buffer_head */
	map_bh(bh_result, sb, bno);

	pr_debug("bno = %d", bno);

brelse_index:
	brelse(bh_index);

	return ret;
}

/*
 * Called by the page cache to read a page from the physical disk and map it in
 * memory.
 */
static void ouichefs_readahead(struct readahead_control *rac)
{
	mpage_readahead(rac, ouichefs_file_get_block);
}

/*
 * Called by the page cache to write a dirty page to the physical disk (when
 * sync is called or when memory is needed).
 */
static int ouichefs_writepage(struct page *page, struct writeback_control *wbc)
{
	return block_write_full_page(page, ouichefs_file_get_block, wbc);
}

/*
 * Called by the VFS when a write() syscall occurs on file before writing the
 * data in the page cache. This functions checks if the write will be able to
 * complete and allocates the necessary blocks through block_write_begin().
 */
static int ouichefs_write_begin(struct file *file,
				struct address_space *mapping, loff_t pos,
				unsigned int len, struct page **pagep,
				void **fsdata)
{
	struct ouichefs_sb_info *sbi = OUICHEFS_SB(file->f_inode->i_sb);
	int err;
	uint32_t nr_allocs = 0;

	/* Check if the write can be completed (enough space?) */
	if (pos + len > OUICHEFS_MAX_FILESIZE)
		return -ENOSPC;
	nr_allocs = max(pos + len, file->f_inode->i_size) / OUICHEFS_BLOCK_SIZE;
	if (nr_allocs > file->f_inode->i_blocks - 1)
		nr_allocs -= file->f_inode->i_blocks - 1;
	else
		nr_allocs = 0;
	if (nr_allocs > sbi->nr_free_blocks)
		return -ENOSPC;

	/* prepare the write */
	err = block_write_begin(mapping, pos, len, pagep,
				ouichefs_file_get_block);
	/* if this failed, reclaim newly allocated blocks */
	if (err < 0) {
		pr_err("%s:%d: newly allocated blocks reclaim not implemented yet\n",
		       __func__, __LINE__);
	}
	return err;
}

/*
 * Called by the VFS after writing data from a write() syscall to the page
 * cache. This functions updates inode metadata and truncates the file if
 * necessary.
 */
static int ouichefs_write_end(struct file *file, struct address_space *mapping,
			      loff_t pos, unsigned int len, unsigned int copied,
			      struct page *page, void *fsdata)
{
	int ret;
	struct inode *inode = file->f_inode;
	struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);
	struct super_block *sb = inode->i_sb;

	/* Complete the write() */
	ret = generic_write_end(file, mapping, pos, len, copied, page, fsdata);
	if (ret < len) {
		pr_err("%s:%d: wrote less than asked... what do I do? nothing for now...\n",
		       __func__, __LINE__);
	} else {
		uint32_t nr_blocks_old = inode->i_blocks;

		/* Update inode metadata */
		inode->i_blocks = inode->i_size / OUICHEFS_BLOCK_SIZE + 2;
		inode->i_mtime = inode->i_ctime = current_time(inode);
		mark_inode_dirty(inode);

		/* If file is smaller than before, free unused blocks */
		if (nr_blocks_old > inode->i_blocks) {
			int i;
			struct buffer_head *bh_index;
			struct ouichefs_file_index_block *index;

			/* Free unused blocks from page cache */
			truncate_pagecache(inode, inode->i_size);

			/* Read index block to remove unused blocks */
			bh_index = sb_bread(sb, ci->index_block);
			if (!bh_index) {
				pr_err("failed truncating '%s'. we just lost %llu blocks\n",
				       file->f_path.dentry->d_name.name,
				       nr_blocks_old - inode->i_blocks);
				goto end;
			}
			index = (struct ouichefs_file_index_block *)
					bh_index->b_data;

			for (i = inode->i_blocks - 1; i < nr_blocks_old - 1;
			     i++) {
				put_block(OUICHEFS_SB(sb), index->blocks[i]);
				index->blocks[i] = 0;
			}
			mark_buffer_dirty(bh_index);
			brelse(bh_index);
		}
	}
end:
	return ret;
}

const struct address_space_operations ouichefs_aops = {
	.readahead = ouichefs_readahead,
	.writepage = ouichefs_writepage,
	.write_begin = ouichefs_write_begin,
	.write_end = ouichefs_write_end
};

static int ouichefs_open(struct inode *inode, struct file *file)
{
	bool wronly = (file->f_flags & O_WRONLY) != 0;
	bool rdwr = (file->f_flags & O_RDWR) != 0;
	bool trunc = (file->f_flags & O_TRUNC) != 0;

	if ((wronly || rdwr) && trunc && (inode->i_size != 0)) {
		struct super_block *sb = inode->i_sb;
		struct ouichefs_sb_info *sbi = OUICHEFS_SB(sb);
		struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);
		struct ouichefs_file_index_block *index;
		struct buffer_head *bh_index;
		sector_t iblock;

		/* Read index block from disk */
		bh_index = sb_bread(sb, ci->index_block);
		if (!bh_index)
			return -EIO;
		index = (struct ouichefs_file_index_block *)bh_index->b_data;

		for (iblock = 0; index->blocks[iblock] != 0; iblock++) {
			put_block(sbi, index->blocks[iblock]);
			index->blocks[iblock] = 0;
		}
		inode->i_size = 0;
		inode->i_blocks = 0;

		brelse(bh_index);
	}

	return 0;
}

ssize_t ouichefs_read(struct file *file, char __user *space, size_t len,
		      loff_t *offset)
{
	// Cas où on a rien à lire
	if (len == 0)
		return 0;

	// Cas où l'offset dépasse la taille maximale
	if (*offset > OUICHEFS_MAX_FILESIZE - 1024) {
		pr_debug("Unable to read after maximum file size\n");
		return -ESPIPE;
	}

	inode_lock_shared(file->f_inode);

	// On récupère les informations de l'inode
	struct inode *inode = file->f_inode;
	struct ouichefs_inode_info *ci = OUICHEFS_INODE(inode);

	// On récupère les informations du super block
	struct super_block *sb = inode->i_sb;

	struct buffer_head *bh_index, *cur_bh;
	struct ouichefs_file_index_block *index;

	int ret, error, i, j;

	// Le nombre de blocs effectivements utilisés (sans le bloc d'inode)
	int nr_blocks = inode->i_blocks - 1;

	// Le nombre d'octets qu'on a sauté (0 par défaut car on commence toujours par premier bloc)
	int nr_jumped_bytes = 0;

	int cur_block_number, cur_block_size, cur_len, cur_jump;

	// Le nombre d'octets lus
	int read_bytes = 0;

	int bytes_to_read = 0, left_to_read = 0;

	// Si l'offset dépasse la taille du fichier
	// Si la taille du fichier est nulle
	if (*offset >= inode->i_size || inode->i_size == 0) {
		pr_debug(
			"L'offset dépasse la taille du fichier ou la taille du fichier est nulle\n");
		error = -ESPIPE;
		goto unlock;
	}

	bh_index = sb_bread(sb, ci->index_block);

	if (!bh_index) {
		pr_debug("Error when loading buffer\n");
		error = -EIO;
		goto unlock;
	}

	// On récupère le bloc d'index
	index = (struct ouichefs_file_index_block *)bh_index->b_data;

	// On récupère le nombre d'octets qui nous reste à lire
	left_to_read = min_t(long long, len, inode->i_size);

	/*
	 * On parcourt tous les blocs jusqu'au dernier
	 */

	pr_separator("Avant parcours jusqu'au dernier");

	for (i = 0; i < nr_blocks; i++) {
		cur_block_number = get_block_number(index->blocks[i]);
		cur_block_size = get_block_size(index->blocks[i]);

		// On cumule le nombre d'octets qu'on a sauté
		nr_jumped_bytes += cur_block_size;

		// Si l'offset ne se situe dans le bloc courant
		if (*offset > nr_jumped_bytes)
			continue;

		// On est forcément sur le bon bloc

		// La position par rapport au bloc
		cur_len = nr_jumped_bytes - *offset;
		cur_jump = cur_block_size - cur_len;
		break;
	}

	pr_separator("Après parcours jusqu'au dernier");

	/*
	 * On parcourt tous les blocs à partir du bon bloc
	 */

	pr_separator("Avant le parcours a partir du bon bloc");

	for (j = i; left_to_read > 0 && j < nr_blocks; j++) {
		cur_block_number = get_block_number(index->blocks[j]);
		cur_block_size = get_block_size(index->blocks[j]);

		// On récupère le nombre d'octets à lire
		bytes_to_read = min(left_to_read, cur_block_size - cur_jump);
		cur_bh = sb_bread(sb, cur_block_number);

		if (!cur_bh) {
			pr_debug("Error when loading buffer\n");
			error = -EIO;
			goto release_bh_index;
		}

		// On copie dans l'espace utilisateur
		ret = copy_to_user(space, cur_bh->b_data + cur_jump,
				   bytes_to_read);

		if (ret) {
			pr_debug("Unable to copy to userspace\n");
			error = -EFAULT;
			goto release_cur_bh;
		}

		// On remet le saut à 0, après la première lecture
		if (i == j)
			cur_jump = 0;

		space += bytes_to_read;
		read_bytes += bytes_to_read;
		left_to_read -= bytes_to_read;
	}
	pr_separator("Apres le parcours a partir du bon bloc");

	/*
	 * Mise à jour des méta-données
	 */

	// Dernière modification de l'inode
	inode->i_atime = current_time(inode);

	brelse(cur_bh);
	brelse(bh_index);

	*offset += read_bytes;

	inode_unlock_shared(inode);

	return read_bytes;

release_cur_bh:
	brelse(cur_bh);

release_bh_index:
	brelse(bh_index);

unlock:
	inode_unlock_shared(inode);

	return error;
}

ssize_t ouichefs_write(struct file *file, const char __user *space, size_t len,
		       loff_t *offset)
{
	// Cas où il n'y a rien à écrire
	if (len == 0)
		return 0;

	// Cas où l'offset dépasse la taille maximale
	if (*offset > OUICHEFS_MAX_FILESIZE - 1024) {
		pr_debug("Unable to write after max file size\n");
		return -ESPIPE;
	}

	inode_lock(file->f_inode);

	// On récupère l'inode et ces informations
	struct inode *inode = file->f_inode;
	struct ouichefs_inode_info *inode_info = OUICHEFS_INODE(inode);

	// On récupère le super bloc et ces informations
	struct super_block *sb = inode->i_sb;
	struct ouichefs_sb_info *sb_info = OUICHEFS_SB(sb);

	struct buffer_head *bh_index, *buf_offset, *buf_alloc, *bh_loop;
	struct ouichefs_file_index_block *index;

	int ret, error, i, j;

	// Le nombre de blocs effectivements utilisés (sans le bloc d'inode)
	int nr_blocks = inode->i_blocks - 1;

	// Le nombre d'octets qu'on a sauté (0 par défaut car on commence
	// toujours par le premier bloc)
	int nr_jumped_bytes = 0;

	int cur_block_number, cur_block_size, cur_len;

	// Le nombre d'octets écrits
	int write_bytes = 0;

	// Le nombre d'octets qui reste à écrire
	int left_to_write = len;

	int block_jump, byte_jump;

	// Si la taille de ce qu'on va écrire ne dépasse pas
	// la taille maximale d'un fichier
	if (*offset + len > OUICHEFS_MAX_FILESIZE - 1024) {
		pr_debug("La taille de ce qu'on souhaite ecrire depasse la taille maximale d'un fichier\n");
		error = -ENOSPC;
		goto unlock;
	}

	/*
	 * Si l'offset dépasse la taille du fichier, on compte la différence
	 * d'octets. Cela nous permet d'avoir un fichier avec la bonne taille.
	 */
	int empty_space = 0;

	if (*offset > inode->i_size)
		empty_space = *offset - inode->i_size;

	/*
	 * Si la taille du fichier est supérieure à 0, cela veut dire qu'il
	 * a au moins un bloc qui est actuellement alloué.
	 */
	int not_a_new_file = inode->i_size > 0;

	// Lire l'index bloc depuis le disque
	bh_index = sb_bread(sb, inode_info->index_block);

	if (!bh_index) {
		pr_debug("Error when loading buffer");
		error = -EIO;
		goto release_bh_index;
	}

	index = (struct ouichefs_file_index_block *)bh_index->b_data;

	/*
	 * On parcourt tous les blocs présents jusqu'à trouver le bloc concerné
	 * par la lecture (celui qui contient l'offset).
	 */

	pr_separator(
		"Debut parcourt de tous les blocs presents jusqu'a offset");

	for (i = 0; i < nr_blocks; i++) {
		cur_block_number = get_block_number(index->blocks[i]);
		cur_block_size = get_block_size(index->blocks[i]);

		/*
		 * On cumule le nombre d'octets qu'on a sauté.
		 * Les blocs ne sont pas forcément plein.
		 */
		nr_jumped_bytes += cur_block_size;

		/*
		 * Si l'offset ne se situe dans le bloc courant et
		 * n'est pas sur le dernier bloc.
		 */
		if (nr_jumped_bytes < *offset && i != nr_blocks - 1) {
			pr_debug("offset n'est pas dans le bloc courant");
			pr_debug("on n'est pas sur le dernier bloc");
			continue;
		}

		// On est forcément sur le bon bloc

		// Le bloc qui correspond à l'offset recherché
		block_jump = i;

		// La taille de ce qu'il reste comme donné après l'offset
		cur_len = nr_jumped_bytes - *offset;

		// Le début de ce qu'on souhaite écrire dans le bloc courant
		byte_jump = cur_block_size - cur_len;

		break;
	}

	pr_separator("Fin parcourt de tous les blocs presents jusqu'a offset");

	/*
	 * Indice du dernier bloc à écrire.
	 * byte_jump + len correspond à l'offset du dernier octet qu'on
	 * va écrire. On récupère la valeur entière supérieure.
	 */
	int nr_blocks_to_allocate =
		CEIL(byte_jump + len, OUICHEFS_MAX_BLOCK_SIZE);

	// Si on dépasse la limite du nombre de blocs pour un fichier on tronque
	if (nr_blocks + nr_blocks_to_allocate > OUICHEFS_MAX_BLOCKS_PER_FILE) {
		nr_blocks_to_allocate =
			OUICHEFS_MAX_BLOCKS_PER_FILE - nr_blocks;
	}

	// Il n'y a aucun nouveau bloc à allouer
	if (nr_blocks_to_allocate == 0) {
		pr_debug("Il n'y a aucun nouveau bloc à allouer\n");
		error = -EFBIG;
		goto unlock;
	}

	// Il n'y a pas assez de blocs disponibles
	if (nr_blocks_to_allocate > sb_info->nr_free_blocks) {
		pr_debug("Il n'y a pas assez de blocs disponibles\n");
		error = -ENOSPC;
		goto unlock;
	}

	/*
	 * Shift : On décalle les blocs
	 */

	pr_separator("Debut du shift");

	for (j = nr_blocks - 1; j > block_jump; j--)
		index->blocks[j + nr_blocks_to_allocate] = index->blocks[j];

	pr_separator("Fin du shift");

	/*
	 * Écrire la 2e partie du bloc dans le dernier bloc alloué
	 * Allocation des nouveaux blocs
	 */

	pr_separator("Début de l'allocation des nouveaux blocs");

	int index_last_buf_alloc = 0;

	/*
	 * Indice du premier bloc qu'on est censé allouer.
	 *
	 * not_a_new_file permet de déterminer si on doit commencer l'allocation
	 * à partir de block_jump ou à partir du bloc suivant. Cela permet de
	 * gérer le cas où le premier bloc est déjà alloué.
	 */
	int first_block_to_allocate = block_jump + not_a_new_file;

	// L'indice du dernier bloc qu'on est censé allouer.
	int last_block_to_allocate =
		first_block_to_allocate + nr_blocks_to_allocate;

	for (i = first_block_to_allocate; i < last_block_to_allocate; i++) {
		// On récupère un bloc non-utilisé
		cur_block_number = get_free_block(sb_info);

		if (!cur_block_number) {
			pr_debug("Error when copying data from userspace\n");
			error = -ENOSPC;
			goto release_bh_index;
		}

		// On écrit au sein du bloc, son numéro de bloc alloué
		set_block_number(&(index->blocks[i]), cur_block_number);

		// On écrit au sein du bloc, sa taille (0 car fraîchement alloué)
		set_block_size(&(index->blocks[i]), 0);

		// On récupère le buffer head du bloc courant
		buf_alloc = sb_bread(sb, cur_block_number);

		if (!buf_alloc) {
			pr_debug("Unable to read block %d from disk",
				 cur_block_number);
			error = -ENOSPC;
			goto release_bh_index;
		}

		map_bh(buf_alloc, sb, cur_block_number);

		index_last_buf_alloc = i;
	}

	pr_separator("Fin de l'allocation des nouveaux blocs");

	// On met à jour le nombre de blocs
	inode->i_blocks += nr_blocks_to_allocate;

	/*
	 * memcpy
	 */

	pr_separator("Avant memcpy");

	if (not_a_new_file) {
		// On récupère le buffer head du bloc dont on doit déplacer le contenu
		buf_offset = sb_bread(
			sb, get_block_number(index->blocks[block_jump]));

		if (!buf_offset) {
			pr_debug("Unable to read block %d from disk",
				 block_jump);
			error = -ENOSPC;
			goto release_bh_index;
		}

		// On copie la deuxième partie du bloc dedans
		memcpy(buf_alloc->b_data, buf_offset->b_data + byte_jump,
		       OUICHEFS_MAX_BLOCK_SIZE - byte_jump);

		/* On met à jour la taille des deux blocs qu'on modifie */
		int tmp_block_size, tmp;

		// Bloc dont on enlève la moitié
		tmp_block_size = get_block_size(index->blocks[block_jump]);
		set_block_size(&(index->blocks[block_jump]), byte_jump);

		tmp = get_block_size(index->blocks[block_jump]);

		// Dernier bloc qu'on alloue
		tmp = tmp_block_size - byte_jump;

		/*
		 * On s'assurer que la valeur ne peut jamais être négative
		 * Ce cas arrive lorsqu'on écrit après la fin du fichier.
		 */
		tmp = tmp < 0 ? 0 : tmp;
		set_block_size(&(index->blocks[index_last_buf_alloc]), tmp);

	} else {
		// Cas particulier où on vient de créer le fichier.
		buf_offset = sb_bread(sb, get_block_number(index->blocks[0]));

		if (!buf_offset) {
			pr_debug("Unable to read block 0 from disk");
			error = -ENOSPC;
			goto release_bh_index;
		}
	}

	pr_separator("Après memcpy");

	/*
	 * memset
	 */

	pr_separator("Avant memset");

	// Cas où l'écriture rentre dans un seul bloc.
	// On rajoute du padding après l'écriture.
	if (byte_jump + len < OUICHEFS_MAX_BLOCK_SIZE) {
		memset(buf_offset->b_data + byte_jump + len, 0,
		       OUICHEFS_MAX_BLOCK_SIZE - byte_jump - len);
	}
	brelse(buf_offset);
	mark_buffer_dirty(buf_alloc);
	sync_dirty_buffer(buf_alloc);

	pr_separator("Après memset");

	/*
	 * Écriture des données dans les blocs
	 */

	pr_separator("Avant boucle d'ecriture dans les blocs");

	int n, last_block_to_write;

	// L'indice du dernier bloc à écrire
	last_block_to_write = block_jump + nr_blocks_to_allocate;

	for (i = block_jump; left_to_write > 0 && i < last_block_to_write;
	     i++) {
		pr_debug("i = %d, left_to_write = %d", i, left_to_write);

		cur_block_number = get_block_number(index->blocks[i]);
		bh_loop = sb_bread(sb, cur_block_number);

		if (!bh_loop) {
			pr_debug("Unable to read bloc %d from disk",
				 cur_block_number);
			error = -EIO;
			goto release_bh_index;
		}

		// Cas général où on est le dernier bloc à écrire
		if (i == last_block_to_write - 1 || nr_blocks == 1) {
			pr_separator("Cas où on est le dernier bloc");

			ret = copy_from_user(bh_loop->b_data + byte_jump, space,
					     left_to_write);

			if (ret) {
				pr_debug("Unable to copy data to userspace");
				error = -EFAULT;
				goto release_bh_index;
			}

			int previous_block_size =
				get_block_size(index->blocks[i]);

			// La taille du dernier bloc correspond à la taille précédente
			// du bloc additionner au nombre d'octets qu'il reste à écrire
			set_block_size(&(index->blocks[i]),
				       previous_block_size + left_to_write);

			space += left_to_write;
			write_bytes += left_to_write;
			left_to_write -= left_to_write;

			pr_separator("Cas où on est le dernier bloc");
		}

		// Cas où on n'est pas le dernier bloc
		else {
			pr_separator("Cas où on n'est pas le dernier bloc");

			n = OUICHEFS_MAX_BLOCK_SIZE - byte_jump;
			ret = copy_from_user(bh_loop->b_data + byte_jump, space,
					     n);

			if (ret) {
				pr_debug("Unable to copy data to userspace");
				error = -EFAULT;
				goto release_bh_index;
			}

			set_block_size(&(index->blocks[i]),
				       OUICHEFS_MAX_BLOCK_SIZE);

			space += n;
			write_bytes += n;
			left_to_write -= n;

			pr_separator("Cas où on n'est pas le dernier bloc");
		}

		// Après le premier bloc, mettre byte_jump à 0
		if (i == block_jump)
			byte_jump = 0;

		brelse(bh_loop);
		mark_buffer_dirty(bh_loop);
		sync_dirty_buffer(bh_loop);
	}

	pr_separator("Après boucle d'ecriture dans les blocs");

	// pr_debug("avant opt1\n");
	// opti1:
	// pr_debug("apres opt1\n");
	mark_buffer_dirty(buf_alloc);
	brelse(buf_alloc);
	sync_dirty_buffer(buf_alloc);

	/*
	 * Mise à jour des méta-données
	 */

	// Taille du fichier
	inode->i_size += write_bytes;
	inode->i_size += empty_space;

	// Dernière modification de l'inode
	inode->i_ctime = current_time(inode);

	// Dernière modification
	inode->i_mtime = current_time(inode);

	*offset += write_bytes;

	mark_inode_dirty(inode);
	brelse(bh_index);
	inode_unlock(inode);

	return write_bytes;

release_bh_index:
	brelse(bh_index);

unlock:
	inode_unlock(inode);

	return error;
}

const struct file_operations ouichefs_file_ops = {
	.owner = THIS_MODULE,
	.open = ouichefs_open,
	.llseek = generic_file_llseek,
	.read = ouichefs_read,
	// .read_iter = generic_file_read_iter,
	.write = ouichefs_write,
	// .write_iter = generic_file_write_iter,
};
