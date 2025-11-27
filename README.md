
# 🐚 Minishell – Projet PSI 2025/2026

Ce projet est une implémentation simplifiée d’un shell UNIX, réalisée dans le cadre de l’unité *PSI* (Programmation des Systèmes d'Information
) à l’upjv pour l’année universitaire 2025/2026.

Il s’agit d’un mini-interpréteur de commandes capable d’exécuter des commandes internes (builtins), des commandes externes, de gérer les redirections, les pipes et les variables d’environnement.

---

## 🚀 Fonctionnalités implémentées

### ✔ **1. Gestion des commandes internes (builtins)**  
- `cd`  
- `exit`  
- `export`  
- `unset`  

### ✔ **2. Exécution de commandes externes**
Exemples :
```bash
ls -la
grep "txt"
echo hello
````

### ✔ **3. Redirections**

* Sortie standard :
  `cmd > fichier.txt`
  `cmd >> fichier.txt`
* Entrée standard :
  `cmd < fichier.txt`

### ✔ **4. Pipes**

Exécution en pipeline :

```bash
ls | grep txt
```

### ✔ **5. Opérateurs logiques**

* `cmd1 && cmd2`
* `cmd1 || cmd2`

### ✔ **6. Exécution en arrière-plan**

```
sleep 5 &
```

### ✔ **7. Gestion des variables d’environnement**

* Substitution : `$HOME`
* Exportation : `export VAR=value`
* Suppression : `unset VAR`

---

## 🧠 Architecture du projet

```
PSI_projet_2026/
│
├── src/
│   ├── main.c           → boucle principale du shell
│   ├── parser.c         → découpe et analyse de la ligne de commande
│   ├── processus.c      → gestion de l’exécution et des redirections
│   ├── builtins.c       → commandes internes
│
├── include/
│   ├── parser.h
│   ├── processus.h
│   ├── builtins.h
│
├── Makefile             → compilation complète
└── README.md
```

---

## 🔧 Compilation

Depuis la racine du projet :

```bash
make
```

Un exécutable `minishell` est généré.

---

## ▶️ Exécution

```bash
./minishell
```

Exemples :

```bash
$ ls
$ echo "test" > file.txt
$ cat < file.txt
$ ls | grep src
$ export VAR=hello
$ echo $VAR
```

---

## 📌 Remarque importante

Ce projet est une version académique simplifiée d’un shell Linux :
il n’a pas vocation à remplacer un shell complet (bash, zsh…).

---

## ✨ Auteur(e)

**Amel Cheddani**
Projet PSI 2025/2026

