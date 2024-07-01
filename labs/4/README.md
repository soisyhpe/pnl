# LAB-4

---

## Task 1 : Connecting the kgdb debugger

---

> **Question 1**

> **Question 2**

Le serial port pour kgdb se configure en utilisant un paramètre lors de l'exécution de qemu : `-serial mon:stdio -serial tcp::1234,server,notwait`.

> **Question 3**

Étant donné qu'il y a un test `-z ${KDB}`, il faut définir la variable `KDB` : `KDB=1`.

On démarre gdb en spécifiant le binaire `vmlinux` : `gdb /tmp/linux-6.5.7/vmlinux`.

Puis, on se connecte sur la machine virtuelle : `target remote :1234`.

Ensuite, on démarre la machine virtuelle : `./qemu-run-externKernel.sh`.

> **Question 4**

`info thread` : Affiche les threads lancés sur la machine virtuelle.

`monitor ps` : Affiche la liste des tâches actives dans ta machine virtuelle. Du coup, `monitor <command>` permet d'exécuter des commandes depuis gdb sur la machine virtuelle.

> **Question 5**

La commande `continue` permet de résumer (débloquer) la machine virtuelle.

> **Question 6**

Pour redonner la main à gdb sur la machine hôte de la machine virtuelle, on va écrire dans le fichier `echo 'g' > /proc/sysrq-trigger`.

> [!info] Pour simplifier le debugging, [Magic SysRq Key](https://en.wikipedia.org/wiki/Magic_SysRq_key) offre des fonctionnalités.

## Task 2 : Using kgdb

---

> **Question 1**

Pour récupérer où les symboles sont référencés et déclarés : On utilise le site [elixir](https://elixir.bootlin.com/linux/latest/A/ident/init_uts_ns).

D'après *elixir*, la structure `init_uts_ns` est définie et initialisée dans le fichier `include/linux/utsname.h` du code source du noyau.

> **Question 2**

On affiche le contenu de la variable `init_uts_ns` : `print init_uts_ns`.

Pour modifier un champs de la structure `init_uts_ns` : `set variable init_uts_ns.name.release = "eros"`.

Pour voir les changements :
- Sur gdb : `print init_uts_ns`
- Sur la machine virtuelle : `uname -r`

> [!danger] Il est impossible de définir des symboles qui n'existent pas : `set variable test = init_uts_ns`.

## Task 3 : My first bug

---

> **Question 1**

Le module `hanging` :
- Lance un thread : `kthread_run(my_hanging_fn, NULL, "my_hanging_fn")`
- La fonction `my_hanging_fn` : Rend l'exécution de la tâche interruptible, Attend 60 * HZ et Indéfinit la structure hanging_thread.
- Lors du déchargement du module, stoppe le thread si la structure existe toujours i.e. dans le cas où le thread n'est pas allé au bout de son `schedule_timeout(50*HZ);`.

> **Question 2**

On modifie le Makefile de sorte à ce que `KERNELDIR_LKP` pointe vers le chemin du code source du kernel : `KERNELDIR_LKP ?= /tmp/linux-6.5.7/`.

Ensuite, on exécute la commande `make` pour créer le binaire.

Une fois que les fichiers objets et binaires sont crées, on va déplacement le module dans le dossier partagé `/tmp/share/`.

Dans la machine virtuelle, on exécute la commande suivante pour charger le module : `insmod hanging.ko`.

Après avoir attendu environ 30 secondes, on remarque que l'exécution du module se termine.

Cela n'est pas un comportement normal. En effet, le kernel est censé lever un **kernel panic** lorsqu'un processus se idle plus de 30 secondes.

> **Question 3**

Pour régler "ce problème", il faut redéfinir les configurations de notre kernel (`make menuconfig`) où on va déclencher *kgdb* lorsqu'un **kernel panic** a lieu.

Solution : > Kernel hacking > Debug Oops, Lockups and Hangs > Panic on Oops [=y]

Une fois le changement réalisé, on recompile le noyau : `make -j 24`.

Après que la compilation soit terminée, on lance la machine virtuelle avec le script : `./qemu-run-externKernel.sh`.

Enfin, on charge à nouveau le module : `insmod hanging.ko`.

> **Question 4**

Non, le code ne correspond pas au code de notre module. En effet, on remarque que notre **kernel panic** se produit dans la fonction `check_hung_uninterruptible_tasks()` qui renvoie un **kernel panic** au bout

> **Question 5**

La commande `monitor ps` permet de récupérer la liste des processus actifs sur la machine virtuelle.

On remarque que le processus `my_hanging_fn` a pour PID `139`. On utilise donc la commande `monitor btp 139` pour dump le backtrace du module.

> **Question 6**

En exécutant la commande `monitor lsmod` depuis gdb, on a bien notre module qui est chargé.

```bash
(gdb) monitor lsmod
Module                  Size  modstruct     Used by
hanging                 4096/    4096/       0/    4096  0xffffffffa0002040    0  (Live) 0xffffffffa0000000/0xffffffffa0004000/0x0000000000000000/0xffffffffa0002000 [ ]
```

> [!info] La première adresse affichée est l'adresse de la `struct module`.

La seconde adresse correspond à *???*.

> **Question 7**

Pour résoudre ce problème, on a deux solutions possibles :
- Première solution : **Rendre le code interruptible** avec `set_current_state(TASK_INTERRUPTIBLE);`. 
- Deuxième solution : Dans la configuration du kernel, on peut **désactiver la randomisation des adresses**.

## Task 4 : Dynamic debug printing

---


