# PNL : LAB-8 : Appels systèmes

---

## Tâche 1 : 

---

> **Question 1**


La fonction `syscall` permet d'invoquer des appels systèmes qui n'ont pas de wrapper en C.

Ses paramètres :
- Le numéro d'appel système
- Les arguments requis par l'appel système qu'on invoque

> **Question 2**

```c
#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(void) {
  pid_t tid;

  tid = syscall(SYS_gettid);
  syscall(SYS_tgkill, getpid(), tid, SIGHUP);
}
```

> **Question 3**

1. `hx kernel/fork.c`
  - Ajouter le corps du syscall
  - :wq

2. `hx include/linux/syscalls.h`
  - 

*Sources:*
*- [](https://medium.com/anubhav-shrimal/adding-a-hello-world-system-call-to-linux-kernel-dad32875872)*
*- [](https://dev.to/jasper/adding-a-system-call-to-the-linux-kernel-5-8-1-in-ubuntu-20-04-lts-2ga8)*
