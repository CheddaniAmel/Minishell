/** @file builtins.c
 * @brief Implementation of built-in commands
 * @author Nom1
 * @author Nom2
 * @date 2025-26
 * @details Implémentation des fonctions des commandes intégrées.
 */
#define _GNU_SOURCE
#include <stdio.h>


#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "builtins.h"
#include "processus.h"

/** @brief Fonction de vérification si une commande est une commande "built-in".
 * @param cmd Nom de la commande à vérifier.
 * @return int 1 si la commande est intégrée, 0 sinon.
 * @details Les commandes intégrées sont a minima: cd, exit, export, unset, pwd.
 */
int is_builtin(const processus_t* cmd) {
if (!cmd || !cmd->path) return 0;

    return (
        strcmp(cmd->path, "cd")    == 0 ||
        strcmp(cmd->path, "exit")  == 0 ||
        strcmp(cmd->path, "export")== 0 ||
        strcmp(cmd->path, "unset") == 0 ||
        strcmp(cmd->path, "pwd")   == 0
    );
}

/** @brief Fonction d'exécution d'une commande intégrée.
 * @param cmd Pointeur vers la structure de commande à exécuter.
 * @return int 0 en cas de succès, -1 en cas d'erreur.
 */
int exec_builtin(processus_t* cmd) {
    if (!cmd || !cmd->path) return -1;

    if (strcmp(cmd->path, "cd") == 0)
        return builtin_cd(cmd);

    if (strcmp(cmd->path, "exit") == 0)
        return builtin_exit(cmd);

    if (strcmp(cmd->path, "export") == 0)
        return builtin_export(cmd);

    if (strcmp(cmd->path, "unset") == 0)
        return builtin_unset(cmd);

    if (strcmp(cmd->path, "pwd") == 0)
        return builtin_pwd(cmd);

    return -1;

}

/** Fonctions spécifiques aux commandes intégrées. */
/** @brief Fonction d'exécution de la commande "cd".
 * @param cmd Pointeur vers la structure de commande à exécuter.
 * @return int 0 en cas de succès, -1 en cas d'erreur.
 * @details Déplace le CWD du processus vers le répertoire spécifié dans le premier argument de la commande.
 *  Si aucun argument n'est fourni, le CWD est déplacé vers le répertoire HOME de l'utilisateur.
 *  En cas d'erreur (répertoire inexistant, permission refusée, etc.), un message d'erreur est affiché sur *cmd->stderr* et la fonction retourne un code d'erreur.
 */
int builtin_cd(processus_t* cmd) {
    char *target = NULL;

    if (cmd->argv[1])
        target = cmd->argv[1];
    else
        target = getenv("HOME");

    if (!target)
        target = "/";

    if (chdir(target) != 0) {
         
        dprintf(cmd->stderr_fd, "cd: %s: No such file or directory\n", target);
        return -1;
    }

    return 0;


}

/** @brief Fonction d'exécution de la commande "exit".
 * @param cmd Pointeur vers la structure de commande à exécuter.
 * @return int 0 en cas de succès, -1 en cas d'erreur.
 * @details Termine le shell avec le code de sortie spécifié dans le premier argument de la commande.
 *  Si aucun argument n'est fourni, le shell se termine avec le code de sortie 0.
 *  En cas d'erreur (argument non numérique, etc.), un message d'erreur est affiché sur *cmd->stderr* et la fonction retourne un code d'erreur
 */
int builtin_exit(processus_t* cmd) {
    int code = 0;

    if (cmd->argv[1]) {
        for (int i = 0; cmd->argv[1][i]; i++) {
            if (cmd->argv[1][i] < '0' || cmd->argv[1][i] > '9') {
                dprintf(cmd->stderr_fd, "exit: numeric argument required\n");
                return -1;
            }
        }
        code = atoi(cmd->argv[1]);
    }

    exit(code);
}

/** @brief Fonction d'exécution de la commande "export".
 * @param cmd Pointeur vers la structure de commande à exécuter.
 * @return int 0 en cas de succès, -1 en cas d'erreur.
 * @details Ajoute ou modifie une variable d'environnement dans l'environnement du shell. En cas d'erreur (format invalide, etc.), un message d'erreur est affiché sur *cmd->stderr* et la fonction retourne un code d'erreur.
 */
int builtin_export(processus_t* cmd) {
    if (!cmd->argv[1]) {
        dprintf(cmd->stderr_fd, "export: expected VAR=VALUE\n");
        return -1;
    }

    char *arg = cmd->argv[1];
    char *eq = strchr(arg, '=');

    if (!eq) {
        dprintf(cmd->stderr_fd, "export: expected VAR=VALUE\n");
        return -1;
    }

    *eq = '\0';
    char *var = arg;
    char *val = eq + 1;

    if (setenv(var, val, 1) != 0) {
        dprintf(cmd->stderr_fd, "export: failed to set variable\n");
        return -1;
    }

    return 0;


}

/** @brief Fonction d'exécution de la commande "unset".
 * @param cmd Pointeur vers la structure de commande à exécuter.
 * @return int 0 en cas de succès, -1 en cas d'erreur.
 * @details Supprime une variable d'environnement de l'environnement du shell. En cas d'erreur (variable inexistante, etc.), un message d'erreur est affiché sur *cmd->stderr* et la fonction retourne un code d'erreur.
 */
int builtin_unset(processus_t* cmd) {
    if (!cmd->argv[1]) {
        dprintf(cmd->stderr_fd, "unset: missing variable name\n");
        return -1;
    }

    if (unsetenv(cmd->argv[1]) != 0) {
        dprintf(cmd->stderr_fd, "unset: error removing variable\n");
        return -1;
    }

    return 0;

}

/** @brief Fonction d'exécution de la commande "pwd".
 * @param cmd Pointeur vers la structure de commande à exécuter.
 * @return int 0 en cas de succès, -1 en cas d'erreur.
 * @details Affiche le répertoire de travail actuel (CWD) du processus sur la sortie standard *cmd->stdout*. En cas d'erreur, un message d'erreur est affiché sur *cmd->stderr* et la fonction retourne un code d'erreur.
 */
int builtin_pwd(processus_t* cmd) {
 char buffer[4096];

    if (!getcwd(buffer, sizeof(buffer))) {
        dprintf(cmd->stderr_fd, "pwd: error retrieving path\n");
        return -1;
    }

    dprintf(cmd->stdout_fd, "%s\n", buffer);
    return 0;

}
