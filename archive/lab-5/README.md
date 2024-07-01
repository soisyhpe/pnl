# PNL : LAB-5 : User/Kernel Communication Mechanisms

Autheur : Eros CHAN (28720888)

---

## Tâche 1 : Le pseudo système de fichier *sysfs*

---

On build le module depuis la machine hôte (i.e. machine de la PPTI) et on copie le fichier .ko vers le répertoire partagé.

Ensuite, on charge le module : `insmod hello.ko`
Et l'exécute : `dmesg`

On peut décharger le module : `rmmod hello`

> [!danger] L'implantation proposé créé un répertoire. On peut éviter la création d'un répertoire en retirant la ligne qui créé un groupe (i.e. répertoire). Il faut faire un `sysfs_create_file(kobj, attr)`.

> **Question 1**

> **Question 2**

sysfs_create_file kobj attr

sysfs_remove_file

## Tâche 2 : Introduction to ioctl

---

> [!info] *ioctl* est un moyen de fournir des appels systèmes personnalisés pour les drivers.

On va faire un driver qui implante un ioctl :
  - mknod command pour créer et ajouter un nouveau device

> [!info] Il y a deux types de device : **character device** et **block device**.

> [!info] Liste des devices disponibles sur le kernel : `cat /proc/devices`

La fonction `register_chrdev` permet d'enregister un device, `unregister_chrdev` permet de retirer l'enregistrement d'un device.

> [!info] Spécifier 0 en tant que major indique que le kernel doit choisir un nombre aléatoire. Pour récupérer la valeur de ce nombre, on récupère la valeur de retour de la fonction.

> **Question 1**

Après avoir compilé le fichier, on insère le module (qui se situe dans le dossier partagé /share): `insmod helloioctl.c`

Pour vérifier que notre device a bien été rajouté dans les devices characters, on exécute la commande suivante : `cat /proc/devices`.

> **Question 2**

On cherche à implanter un ioctl qui renvoie "Hello ioctl!" à l'utilisateur. Pour ce faire, on a besoin de définir un nouveau nombre en read-only qu'on nommera `HELLO`.

On définit le charactère `'N'` en tant que type dans un fichier header qu'on nomme `helloioctl.h`.

> [!info] D'après la [spécification kernel](https://docs.kernel.org/userspace-api/ioctl/ioctl-number.html), le charactère `'N'` ne créé pas de conflit.

On implante la fonction `unlocked_ioctl()` qui affiche "Hello world!" quand `HELLO` est utiilisé et renvoie `-ENOTTY` sinon.

> [!danger] Attention : lorsqu'on souhaite déplacer des données entre l'espace user et l'espace kernel, on utilise les fonctions `copy_from_user()` and `copy_to_suer()`.

## Tâche 3 : Représentation d'un prrocessus dans le kernel

---

> **Question 1**

La `struct pid` a pour rôle d'identifier un processus dans le noyau. Le pid vit dans un table de hachage ce qui permet d'identifier rapidement le processus attaché.

> **Question 2**

