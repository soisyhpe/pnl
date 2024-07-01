#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define LOOP 10
#define BUF_SIZE 128
#define LINE_LENGTH 80

int tests_count = 0;
char buf[BUF_SIZE] = { 0 };

/*
 * Afficher le contenu d'un buffer.
 * Permet notamment d'afficher les \0 qu'on ne peut pas visualiser
 * avec le print par défaut.
 */
void print_buf(char *buffer, size_t size, FILE *fd_results, int mode)
{
	size_t line_length = 0;

	if (mode == 1) {
		for (size_t i = 0; i < size; i++) {
			if (buffer[i] == '\0') {
				if (line_length + 2 > LINE_LENGTH) {
					fprintf(fd_results, "\n");
					line_length = 0;
				}
				fprintf(fd_results, "\\0");
				line_length += 2;
			} else if (isprint(buffer[i])) {
				if (line_length + 1 > LINE_LENGTH) {
					fprintf(fd_results, "\n");
					line_length = 0;
				}
				fprintf(fd_results, "%c", buffer[i]);
				line_length += 1;
			} else {
				if (line_length + 1 > LINE_LENGTH) {
					fprintf(fd_results, "\n");
					line_length = 0;
				}
				fprintf(fd_results, "?");
				line_length += 1;
			}
		}
	} else {
		fprintf(fd_results, "%s", buffer);
	}

	fprintf(fd_results, "\n");
}

/*
 * Créer un fichier.
 */
int create_file(char *filename)
{
	int fd;

	fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, 00660);

	if (fd < 0) {
		perror("Erreur lors de l'ouverture du fichier \n");
		exit(EXIT_FAILURE);
	}

	return fd;
}

/*
 * Mesurer la durée de l'opération d'écriture.
 */
int measure_write(int fd, const void *str, size_t len, FILE *fd_results)
{
	clock_t start, end;
	int ret;

	start = clock();
	ret = write(fd, str, len);
	end = clock();

	fprintf(fd_results, "Temps mis : %4f secondes\n",
		(double)((end - start) / (double)CLOCKS_PER_SEC));

	return ret;
}

/*
 * Mesurer la durée de l'opération de lecture.
 */
int measure_read(int fd, void *buf, size_t len, FILE *fd_results)
{
	clock_t start, end;
	int ret;

	start = clock();
	ret = read(fd, buf, len);
	end = clock();

	fprintf(fd_results, "Temps mis : %4f secondes\n",
		(double)((end - start) / (double)CLOCKS_PER_SEC));

	return ret;
}

/*
 * Vérifier la cohérence entre le nombre d'octets lues ou écrites et le nombre
 * d'octets qu'on attend.
 */
void check_data_coherence(int actual_size, int expected_size, FILE *fd_results)
{
	fprintf(fd_results, "Données écrites ou lues : %d octets sur %d octets",
		actual_size, expected_size);
	if (actual_size != expected_size)
		fprintf(fd_results, " [!!! INCOHÉRENCE !!!]\n");
	else
		fprintf(fd_results, "\n");
}

/*
 * Instancier un nouveau test.
 */
void test(char *test_name, int fd, void (*test)(int fd, FILE *fd_res),
	  FILE *fd_results)
{
	tests_count++;

	fprintf(fd_results,
		"\n[TEST %d] "
		"---------------------------------------------------------------------"
		"--\n",
		tests_count);
	fprintf(fd_results, "Nom du test : %s\n", test_name);
	fprintf(fd_results, "Fichier : %d\n", fd);
	fprintf(fd_results, "\n");

	test(fd, fd_results);

	fprintf(fd_results, "\n");
	fprintf(fd_results,
		"[TEST %d] "
		"---------------------------------------------------------------------"
		"--\n\n",
		tests_count);
}

/*
 * Test 1 : Écriture et lecture simple
 */
void test_write_read(int fd, FILE *fd_results)
{
	char *str = "bonjour, est-ce que ca marche ?";
	int ret, len;

	// Écriture de la donnée dans le fichier
	fprintf(fd_results, "Écriture des données\n");
	fprintf(fd_results, "Offset = %d\n", 0);
	len = strlen(str);
	ret = measure_write(fd, str, len, fd_results);
	fprintf(fd_results, "Données écrites : \n");
	print_buf(str, len, fd_results, 0);
	check_data_coherence(ret, len, fd_results);

	fprintf(fd_results, "\n");

	// Lecture de la donnée dans le fichier
	fprintf(fd_results, "Lecture des données écrites\n");
	fprintf(fd_results, "Offset = %d\n", 0);
	lseek(fd, 0, SEEK_SET); // repositionner l'offset au début du fichier
	len = strlen(str);
	ret = measure_read(fd, buf, len, fd_results);
	fprintf(fd_results, "Données lues : %s\n", buf);
	print_buf(buf, len, fd_results, 0);
	check_data_coherence(ret, len, fd_results);
}

/*
 * Test 2 : Écriture et lecture au milieu du fichier
 */
void test_write_read_middle(int fd, FILE *fd_results)
{
	int ret, offset = 10;

	lseek(fd, offset, SEEK_SET);
	memset(buf, 0, BUF_SIZE);
	char *str = "COUCOU !";
	int len = strlen(str);

	/* Écrire dans le fichier, à un certain offet */
	fprintf(fd_results, "Écriture des données\n");
	fprintf(fd_results, "Offset = %d\n", offset);
	ret = measure_write(fd, str, len, fd_results);
	fprintf(fd_results, "Données écrites : %s\n", str);
	print_buf(str, len, fd_results, 0);
	check_data_coherence(ret, len, fd_results);

	fprintf(fd_results, "\n");

	/* Lire dans le fichier, à un offset intermédiaire */
	offset = 3;
	fprintf(fd_results, "Lecture des données\n");
	fprintf(fd_results, "Offset = %d\n", offset);
	lseek(fd, offset, SEEK_SET);
	memset(buf, 0, BUF_SIZE);
	measure_read(fd, buf, BUF_SIZE, fd_results);
	fprintf(fd_results, "Données lues : \n");
	print_buf(buf, BUF_SIZE, fd_results, 0);
}

/*
 * Test 3 : Écriture et lecture plus loin que la fin du fichier
 */
void test_write_read_past_eof(int fd, FILE *fd_results)
{
	int ret, offset, len;

	/* Ecrire plus loin que la fin du fichier */
	char *str = "hellooooo";
	len = strlen(str);
	offset = lseek(fd, 4, SEEK_END);

	fprintf(fd_results, "Écriture des données\n");
	fprintf(fd_results, "Offset = %d\n", offset);
	ret = measure_write(fd, str, strlen(str), fd_results);
	fprintf(fd_results, "Données écrites : %s\n", str);
	check_data_coherence(ret, len, fd_results);

	fprintf(fd_results, "\n");

	/* Lire plus loin que la fin du fichier */

	// 1er cas : on lit davantage que ce qu'il n'y a à lire dans le fichier
	offset = lseek(fd, 0, SEEK_SET);

	fprintf(fd_results,
		"Lecture qui dépasse la taille du contenu du fichier\n");
	fprintf(fd_results, "Offset = %d\n", offset);
	ret = measure_read(fd, buf, BUF_SIZE, fd_results);
	fprintf(fd_results, "Données lues : %d octets\n", ret);
	print_buf(buf, BUF_SIZE, fd_results, 1);

	fprintf(fd_results, "\n");

	// 2e cas : on positionne l'offset après la fin du fichier
	offset = lseek(fd, 4, SEEK_END);
	memset(buf, 0, BUF_SIZE);

	fprintf(fd_results, "Lecture qui commence après la fin du fichier\n");
	fprintf(fd_results, "Offset = %d\n", offset);
	ret = measure_read(fd, buf, BUF_SIZE, fd_results);
	fprintf(fd_results, "Données lues : %d octets\n", ret);
}

/*
 * Test 4 : Écriture et lecure d'un grand fichier
 */
void test_write_read_big_file(int fd, FILE *fd_results)
{
	int ret = 0;
	clock_t start, end;

	// écriture
	// char *str = "ABCDEFGHIJKLMNOPQRSTUVWXYZ____\n";
	char *str =
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam fringilla dui ut massa tincidunt, sit amet interdum mauris dictum. Donec suscipit odio ante, et porta dolor aliquet nec. Nullam in velit mi. Duis risus massa, tristique vel convallis eu, consequat in nulla. Nullam ac elit nunc. Sed pretium rhoncus velit, ut viverra lorem accumsan eget. Quisque vestibulum suscipit urna quis sagittis. Proin mollis lorem sit amet cursus tincidunt. Proin id mattis tellus. Nam ac risus lorem. Sed quis elit laoreet.\n";

	fprintf(fd_results, "Écriture des données\n");
	fprintf(fd_results, "Offset = %d\n", 0);

	start = clock();
	for (int i = 0; i < LOOP; i++) {
		ret += write(fd, str, strlen(str));
	}
	end = clock();

	fprintf(fd_results, "Temps mis : %4f secondes\n",
		(double)((end - start) / (double)CLOCKS_PER_SEC));
	fprintf(fd_results, "Données écrites : %d octets\n", ret);

	fprintf(fd_results, "\n");

	// lecture
	lseek(fd, 0, SEEK_SET);
	memset(buf, 0, BUF_SIZE);

	fprintf(fd_results, "Lecture des données\n");
	fprintf(fd_results, "Offset = %d\n", 0);

	start = clock();
	int total = 0;
	while ((ret = read(fd, buf, BUF_SIZE - 1)) > 0) {
		buf[BUF_SIZE - 1] = '\0';
		memset(buf, 0, BUF_SIZE);
		total += ret;
	}
	end = clock();
	fprintf(fd_results, "Temps mis : %4f secondes\n",
		(double)((end - start) / (double)CLOCKS_PER_SEC));
	fprintf(fd_results, "Données lues : %d octets\n", total);
}

void test_write_read_anywhere(int fd, FILE *fd_results)
{
	int ret;
	clock_t start, end;

	// écriture
	fprintf(fd_results, "Écriture des données\n");
	fprintf(fd_results, "Offset = aléatoire\n");

	int taille_fichier = lseek(fd, 0, SEEK_END);
	char *mot = "n'importe ou !";
	int len = strlen(mot);

	start = clock();
	for (int i = 0; i < LOOP; i++) {
		// mettre l'offset quelque part au milieu du fichier
		lseek(fd, (rand() / (float)RAND_MAX) * taille_fichier,
		      SEEK_SET);
		ret += write(fd, mot, len);
	}
	end = clock();
	fprintf(fd_results, "Temps mis : %4f secondes\n",
		(double)((end - start) / (double)CLOCKS_PER_SEC));
	fprintf(fd_results, "Données écrites : %d octets\n", ret);

	fprintf(fd_results, "\n");

	// Lecture
	fprintf(fd_results, "Lecture des données\n");
	fprintf(fd_results, "Offset = 0\n");

	lseek(fd, 0, SEEK_SET);
	memset(buf, 0, BUF_SIZE);

	start = clock();
	int total;
	while ((ret = read(fd, buf, BUF_SIZE - 1)) > 0) {
		buf[BUF_SIZE - 1] = '\0';
		memset(buf, 0, BUF_SIZE);
		total += ret;
	}
	end = clock();
	fprintf(fd_results, "Temps mis : %4f secondes\n",
		(double)((end - start) / (double)CLOCKS_PER_SEC));

	fprintf(fd_results, "\n");
	
	// execl(".", "defrag_user", "./disk/file2", NULL);
	// system("./defrag_user ./disk/file2");

	// Lecture
	fprintf(fd_results, "Lecture des données\n");
	fprintf(fd_results, "Offset = 0\n");

	lseek(fd, 0, SEEK_SET);
	memset(buf, 0, BUF_SIZE);

	start = clock();
	while ((ret = read(fd, buf, BUF_SIZE - 1)) > 0) {
		buf[BUF_SIZE - 1] = '\0';
		memset(buf, 0, BUF_SIZE);
		total += ret;
	}
	end = clock();
	fprintf(fd_results, "Temps mis : %4f secondes\n",
		(double)((end - start) / (double)CLOCKS_PER_SEC));

	fprintf(fd_results, "\n");
	
	// Cas 2 : Lecture du dernier bloc
	// struct stat fi;
	// stat("./disk", &fi);
	// fprintf(fd_results, "Block size = %ld octets\n", fi.st_blksize);
	// fprintf(fd_results, "File size = %d octets\n", total);

	// fprintf(fd_results, "\n");

	// int offset, last_block = total / fi.st_blksize;
	// offset = lseek(fd, (fi.st_blksize * last_block) - fi.st_blksize, SEEK_SET);

	// fprintf(fd_results, "Lecture du dernier bloc\n");
	// fprintf(fd_results, "Offset = %d\n", offset);

	// memset(buf, 0, BUF_SIZE);
	// ret = measure_read(fd, buf, BUF_SIZE);
	// fprintf(fd_results, "Données lues : %s\n", buf);
}

void test_write_read_between_two_blocks(int fd, FILE *fd_results)
{
	int ret, len, offset;
	struct stat fi;
	stat("./disk", &fi);
	fprintf(fd_results, "Block size = %ld octets\n", fi.st_blksize);

	fprintf(fd_results, "\n");

	// Écriture
	offset = lseek(fd, fi.st_blksize - 16, SEEK_SET);

	fprintf(fd_results, "Écriture des données\n");
	fprintf(fd_results, "Offset = %d\n", offset);

	char *str =
		"salut, je m'éclate beaucoup trop avec Nicolas, Mélissa et Eros en PNL";
	len = strlen(str);
	ret = measure_write(fd, str, len, fd_results);

	fprintf(fd_results, "Données écrites : %s\n", str);
	check_data_coherence(ret, len, fd_results);

	fprintf(fd_results, "\n");

	// Lecture

	// Cas 1 : À partir du bon offset
	offset = lseek(fd, fi.st_blksize - 16, SEEK_SET);

	fprintf(fd_results, "Lecture des données à partir du bon offset\n");
	fprintf(fd_results, "Offset = %d\n", offset);

	memset(buf, 0, BUF_SIZE);
	ret = measure_read(fd, buf, len, fd_results);

	fprintf(fd_results, "Données lues : %s\n", buf);
	check_data_coherence(ret, len, fd_results);

	fprintf(fd_results, "\n");

	// Cas 2 : À partir du 2e bloc
	offset = lseek(fd, fi.st_blksize, SEEK_SET);

	fprintf(fd_results, "Lecture des données à partir du 2e bloc\n");
	fprintf(fd_results, "Offset = %d\n", offset);

	memset(buf, 0, BUF_SIZE);
	ret = measure_read(fd, buf, len, fd_results);

	fprintf(fd_results, "Données lues : %s\n", buf);
	check_data_coherence(ret, len, fd_results);
}

void test_write_after_eof(int fd, FILE *fd_results)
{
	int ret, offset;
	char *buff_begin = "Goodbye my lover";
	int len_begin = strlen(buff_begin);
	offset = lseek(fd, 0, SEEK_SET);

	fprintf(fd_results, "Écriture des données\n");
	fprintf(fd_results, "Offset = 0\n");
	ret = measure_write(fd, buff_begin, len_begin, fd_results);
	fprintf(fd_results, "Données écrites : %s\n", buff_begin);
	check_data_coherence(ret, len_begin, fd_results);

	fprintf(fd_results, "\n");

	// Écriture après la fin du fichier
	char buff_end[] = "Goodbye my friend";
	int len_end = strlen(buff_end);
	offset = lseek(fd, (4095 * 1024) + 32, SEEK_SET);

	fprintf(fd_results, "Écriture des données\n");
	fprintf(fd_results, "Offset = %d\n", offset);
	ret += measure_write(fd, buff_end, len_end, fd_results);
	fprintf(fd_results, "Données écrites : %s\n", buff_end);
	//check_data_coherence(ret, len_end);

	fprintf(fd_results, "\n");

	// Lecture de l'ensemble des données
	offset = lseek(fd, (4095 * 1024) + 32, SEEK_SET);

	fprintf(fd_results, "Lecture des données\n");
	fprintf(fd_results, "Offset = %d\n", offset);
	char buf[2048];
	// memset(buf, 0, 2048);
	ret = measure_read(fd, buf, 2048, fd_results);
	//check_data_coherence(ret, len_end);

	fprintf(fd_results, "\n");

	print_buf(buf, BUF_SIZE, fd_results, 1);

	fprintf(fd_results, "\n");
}

void test_write_file_max_size(int fd, FILE *fd_results)
{
	int ret = 0;
	clock_t start, end;

	// écriture
	char *str =
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam fringilla dui ut massa tincidunt, sit amet interdum mauris dictum. Donec suscipit odio ante, et porta dolor aliquet nec. Nullam in velit mi. Duis risus massa, tristique vel convallis eu, consequat in nulla. Nullam ac elit nunc. Sed pretium rhoncus velit, ut viverra lorem accumsan eget. Quisque vestibulum suscipit urna quis sagittis. Proin mollis lorem sit amet cursus tincidunt. Proin id mattis tellus. Nam ac risus lorem. Sed quis elit laoreet.\n";

	fprintf(fd_results, "Écriture des données\n");
	fprintf(fd_results, "Offset = %d\n", 0);

	int nb_iter = 8 * 1028;

	start = clock();
	for (int i = 0; i < nb_iter; i++) {
		ret += write(fd, str, strlen(str));
	}
	end = clock();

	check_data_coherence(ret, nb_iter * strlen(str), fd_results);

	fprintf(fd_results, "Temps mis : %4f secondes\n",
		(double)((end - start) / (double)CLOCKS_PER_SEC));
	fprintf(fd_results,
		"Données écrites : %d octets (taille initialement demandée : %lu octets)\n",
		ret, nb_iter * strlen(str));

	fprintf(fd_results, "\n");

	// lecture
	lseek(fd, 0, SEEK_SET);
	memset(buf, 0, BUF_SIZE);

	fprintf(fd_results, "Lecture des données\n");
	fprintf(fd_results, "Offset = %d\n", 0);

	start = clock();
	int total = 0;
	while ((ret = read(fd, buf, BUF_SIZE - 1)) > 0) {
		buf[BUF_SIZE - 1] = '\0';
		memset(buf, 0, BUF_SIZE);
		total += ret;
	}
	end = clock();
	fprintf(fd_results, "Temps mis : %4f secondes\n",
		(double)((end - start) / (double)CLOCKS_PER_SEC));
	fprintf(fd_results, "Données lues : %d octets\n", total);
}

int main()
{
	srand(time(NULL));

	int fd;
	FILE *fd_results = fopen("./results", "w");

	fd = create_file("./disk/file1");

	test("Écriture et lecture au debut du fichier", fd, &test_write_read,
	     fd_results);
	test("Écriture et lecture au milieu du fichier", fd,
	     &test_write_read_middle, fd_results);
	test("Écriture et lecture après la fin du fichier", fd,
	     &test_write_read_past_eof, fd_results);

	close(fd);

	fd = create_file("./disk/file2");

	test("Écritures successives et lecture à la fin d'un gros fichier", fd,
	     &test_write_read_big_file, fd_results);
	test("Écriture à plusieurs reprises n'importe où du fichier et lecture",
	     fd, &test_write_read_anywhere, fd_results);
	test("Écriture à cheval sur deux blocs et lecture", fd,
	     &test_write_read_between_two_blocks, fd_results);

	close(fd);

	// fd = create_file("./disk/file3");

	// Cas non pris en compte
	// test("Écriture et lecture après la fin du fichier", fd,
	// &test_write_after_eof, fd_results);

	// close(fd);

	fd = create_file("./disk/file4");

	test("Ecriture d'un fichier plus gros que la taille maximum d'un fichier",
	     fd, &test_write_file_max_size, fd_results);

	fclose(fd_results);

	return 0;
}
