# PNL

---

# Introduction

Ce rapport a été rédigé dans le cadre du projet PNL pour l'année 2023-2024. Il a été réalisé conjointement par Eros CHAN, Mélissa LACOUR et Nicolas MILIVOJEVIC.

## Fonctionnalités implémentées et fonctionnelles

---

### `Read` / `Write` écrasant

Cas pris en compte :

- Lorsque l'offset se place au début, au milieu, à la fin et au-delà de la fin du fichier
- Lecture / Écriture à cheval sur deux blocs
- Lecture / Écriture d'un nombre d'octets correspondant à plusieurs blocs en un seul appel
- Écriture limitée à la taille maximale d'un fichier OuicheFS (4Mib)
- Lecture limitée au nombre d'octets disponibles dans le fichier
- Mise à jour des méta-données
- Synchronisation de l'accès à l'inode avec des verrous de lecture et d'écriture

Cas non-pris en compte :

- Cas où on écrase un fichier déjà existant.

> [!info] Le dernier cas n'est pas pris en compte puisque cela nécessite la gestion des différents flags d'ouverture e.g. `O_TRUNC`, `O_APPEND`... le sujet ne demandant pas explicitement de gérer ce cas.
>
> Pour reproduire le problème :
> ```
> echo "hello" > h
> echo "world" > h
> ```

### ioctl

Affiche les informations suivantes d'un fichier :

- Nombre de blocs utilisés
- Nombre de blocs partiellement remplis
- Liste des blocs avec leur numéro de bloc et leur taille
- Nombre d'octets perdus à cause de la fragmentation interne

### `Read` / `Write` avec insertion

Cas pris en compte :

- Taille d'un bloc limitée à $4095$ octets pour respecter l'encodage sur 12 bits
- Lorsque l'offset se place au début, au milieu, à la fin et au-delà de la fin du fichier
- Lecture / Écriture à cheval sur deux blocs
- Lecture / Écriture d'un nombre d'octets correspondant à plusieurs blocs en un seul appel
- Écriture limitée à la taille maximale d'un fichier OuicheFS (4Mib) / Au nombre de blocs maximum autorisé par fichier (troncature)
- Lecture limitée au nombre d'octets disponibles dans le fichier
- Mise à jour des méta-données
- Synchronisation de l'accès à l'inode avec des verrous de lecture et d'écriture

Cas non-pris en compte :

- Cas où on écrase un fichier déjà existant.

> [!info] Le dernier cas n'est pas pris en compte puisque cela nécessite la gestion des différents flags d'ouverture e.g. `O_TRUNC`, `O_APPEND`... le sujet ne demandant pas explicitement de gérer ce cas.
>
> Pour reproduire le problème :
> ```
> echo "hello" > h
> echo "world" > h
> ```

### Défragmentation

Caractéristiques :

- Algorithme en O(n)
- Désallocation des blocs vides
- Shift des données vers la gauche

## Fonctionnalités non-implémentées

---

- Utilisation du page cache

## Présentation des résultats (benchmark)

.---

## Lancement des programmes

---
