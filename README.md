# TP de Systèmes d'Exploitation — Gestion d’un tunnel avec des bus

## Présentation
Ce projet consiste à résoudre un problème de synchronisation à l’aide des sémaphores POSIX (`sem_t`). L’objectif est de permettre à des bus venant de deux directions opposées d’utiliser un tunnel à voie unique, en respectant des règles d’équité et de sécurité. Chaque bus est simulé par un thread.

## Compilation et exécution

Pour compiler le programme, utiliser la commande suivante dans le terminal :

```bash
gcc -o tunnel tunnel.c

Pour executer le fichier, utiliser la commande suivante dans le terminal :

```bash
./tunnel

