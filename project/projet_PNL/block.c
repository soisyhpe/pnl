// SPDX-License-Identifier: GPL-2.0

/*
 * Ce fichier est responsable de la gestion de l'encodage du numéro et de
 * la taille d'un bloc (20 bits pour le numéro et 12 bits pour la taille).
 */

#define NUMBER_MASK (1 << 20)
#define SIZE_MASK 0xfff00000

int get_block_number(int block_number)
{
	return block_number & (NUMBER_MASK - 1);
}

void set_block_number(int *field, int block_number)
{
	*field &= (~(NUMBER_MASK - 1));
	*field = *field | (block_number & (NUMBER_MASK - 1));
}

int get_block_size(int block_number)
{
	return (block_number & SIZE_MASK) >> 20;
}

void set_block_size(int *field, int block_size)
{
	*field &= (~SIZE_MASK);
	*field |= (block_size << 20) & SIZE_MASK;
}
