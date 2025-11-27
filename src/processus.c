/** @file processus.c
 * @brief Implementation of process management functions
 * @author Nom1
 * @author Nom2
 * @date 2025-26
 * @details Implémentation des fonctions de gestion des processus.
 */
#define _GNU_SOURCE
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>     // ← AJOUTER CETTE LIGNE

#include "processus.h"
#include "builtins.h"



#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>


/**
 * @brief Fonction d'initialisation d'une structure de processus.
 * @param proc Pointeur vers la structure de processus à initialiser.
 * @return int 0 en cas de succès, -1 en cas d'erreur.
 * @details Cette fonction initialise les champs de la structure avec les valeurs suivantes:
 * - *pid*: 0
 * - *argv*: {NULL}
 * - *envp*: {NULL}
 * - *path*: NULL
 * - *stdin_fd*: 0
 * - *stdout_fd*: 1
 * - *stderr_fd*: 2
 * - *status*: 0
 * - *is_background*: 0
 * - *start_time*: {0}
 * - *end_time*: {0}
 * - *cf*: NULL
 */
 
#define MAX_FDS 32
#define MAX_TOKENS 128

int init_processus(processus_t* proc) {
    if (!proc) return -1;

    proc->pid = 0;

    for (int i = 0; i < MAX_ARGS; i++)
        proc->argv[i] = NULL;

    for (int i = 0; i < MAX_ENV; i++) {
        proc->envp[i] = NULL;
     } 

    proc->path = NULL;

    proc->stdin_fd = 0;
    proc->stdout_fd = 1;
    proc->stderr_fd = 2;

    proc->status = 0;
    proc->is_background = 0;

    memset(&proc->start_time, 0, sizeof(struct timespec));
    memset(&proc->end_time, 0, sizeof(struct timespec));

    proc->cf = NULL;

    return 0;

}

/** @brief Fonction de lancement d'un processus à partir d'une structure de processus.
 * @param proc Pointeur vers la structure de processus à lancer.
 * @return int 0 en cas de succès, un code d'erreur sinon.
 * @details Cette fonction utilise *fork()* et *execve()* pour lancer le processus décrit par la structure.
 *    Elle gère également les redirections des IOs standards (via *dup2()*).
 *    En cas de succès, le champ *pid* de la structure est mis à jour avec le PID du processus fils.
 *    Le flag *is_background* détermine si on attend la fin du processus ou non.
 *    La valeur de *status* est mise à jour à l'issue de l'exécution avec le code de retour du processus fils lorsque le flag *is_background* est désactivé.
 *    Les temps de démarrage et d'arrêt sont enregistrés dans *start_time* et *end_time* respectivement. *end_time* est mis à jour uniquement si *is_background* est désactivé.
 *    Les descripteurs de fichiers ouverts sont gérés dans *cf->cmdl->opened_descriptors* : le processus "fils" ferme tous les descripteurs listés dans ce tableau avant d'exécuter la commande.
 */
 
int launch_processus(processus_t* proc) {
    if (!proc) return -1;

    /* Si c'est un builtin et qu'on est en foreground : exécution dans le parent */
    if (is_builtin(proc) && !proc->is_background) {
        int r = exec_builtin(proc);
        proc->status = (r == 0) ? 0 : 1;
        return (r == 0) ? 0 : -1;
    }

    /* Enregistrer le temps de démarrage si le champ existe */
    #if defined(CLOCK_REALTIME)
    clock_gettime(CLOCK_REALTIME, &proc->start_time);
    #endif

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        /* ---------- enfant ---------- */
        signal(SIGINT, SIG_DFL);  // ajoute pour restaurer le comportement par defaut de sigint

        /* Appliquer redirections (si différents des standards) */
        if (proc->stdin_fd >= 0 && proc->stdin_fd != STDIN_FILENO) {
            if (dup2(proc->stdin_fd, STDIN_FILENO) < 0) {
                perror("dup2 stdin");
                _exit(127);
            }
        }
        if (proc->stdout_fd >= 0 && proc->stdout_fd != STDOUT_FILENO) {
            if (dup2(proc->stdout_fd, STDOUT_FILENO) < 0) {
                perror("dup2 stdout");
                _exit(127);
            }
        }
        if (proc->stderr_fd >= 0 && proc->stderr_fd != STDERR_FILENO) {
            if (dup2(proc->stderr_fd, STDERR_FILENO) < 0) {
                perror("dup2 stderr");
                _exit(127);
            }
        }

        /* Fermer les descripteurs listés dans cmdl->opened_descriptors s'ils existent */
        if (proc->cf && proc->cf->cmdl) {
            for (int i = 0; i < MAX_FDS; ++i) {
                int fd = proc->cf->cmdl->opened_descriptors[i];
                if (fd >= 0) {
                    /* ne pas fermer les std fds préalablement dupliqués (ils valent 0/1/2) */
                    if (fd != STDIN_FILENO && fd != STDOUT_FILENO && fd != STDERR_FILENO)
                        close(fd);
                }
            }
        }

        /* Si builtin et background -> exécution dans l'enfant (ne changera pas le parent) */
        if (is_builtin(proc)) {
            int r = exec_builtin(proc);
            _exit((r == 0) ? 0 : 1);
        }

        /* Exécuter le binaire : utiliser execve si envp connu sinon execvp */
        //Exécuter la commande
        
        // Préparer l'environnement
    extern char **environ;  // Environnement global
    char **env = (proc->envp[0] != NULL) ? proc->envp : environ;
    
    // Utiliser execvp qui cherche automatiquement dans le PATH
execvp(proc->path, proc->argv);

        /* Si exec échoue */
        fprintf(stderr, "%s: %s\n", proc->path ? proc->path : "unknown", strerror(errno));
        _exit(127);
    } else {
        /* ---------- parent ---------- */
        proc->pid = pid;

        if (proc->is_background) {
            /* processus en arrière-plan : ne pas attendre */
            printf("[bg] pid %d\n", (int)pid);
            proc->status = 0;
            return 0;
        } else {
            int wstatus = 0;
            if (waitpid(pid, &wstatus, 0) < 0) {
                perror("waitpid");
                return -1;
            }

            /* enregistrer status */
            proc->status = wstatus;

            /* enregistrer end_time si champ présent */
            #if defined(CLOCK_REALTIME)
            clock_gettime(CLOCK_REALTIME, &proc->end_time);
            #endif

            /* retourner 0 si exit code 0 sinon code d'erreur non nul */
            if (WIFEXITED(wstatus)) {
                int exitcode = WEXITSTATUS(wstatus);
                return (exitcode == 0) ? 0 : exitcode;
            } else if (WIFSIGNALED(wstatus)) {
                return 128 + WTERMSIG(wstatus);
            } else {
                return -1;
            }
        }
    }
}




/** @brief Fonction d'initialisation d'une structure de contrôle de flux.
 * @param cf Pointeur vers la structure de contrôle de flux à initialiser.
 * @return int 0 en cas de succès, -1 en cas d'erreur.
 * @details Cette fonction initialise les champs de la structure avec les valeurs suivantes:
 * - *proc*: NULL
 * - *unconditionnal_next*: NULL
 * - *on_success_next*: NULL
 * - *on_failure_next*: NULL
 * - *cmdl*: NULL
 */
 
int init_control_flow(control_flow_t* cf) {
    // Ici, un appel à bzero permettrais sûrement de faire le travail en une seule ligne
    
     if (!cf) return -1;

    cf->proc = NULL;
    cf->unconditionnal_next = NULL;
    cf->on_success_next = NULL;
    cf->on_failure_next = NULL;
    cf->cmdl = NULL;

    return 0;
    
}

/** @brief Fonction d'ajout d'un processus à la structure de contrôle de flux.
 * @param cmdl Pointeur vers la structure de ligne de commande dans laquelle le processus doit être ajouté.
 * @param mode Mode d'ajout (UNCONDITIONAL, ON_SUCCESS, ON_FAILURE).
 * @return processus_t* Pointeur vers le processus ajouté, ou NULL en cas d'erreur (tableau plein par exemple).
 * @details Cette fonction ajoute le processus *proc* à la structure de contrôle de flux *cf* selon le mode spécifié:
 * Le dernier élément du tableau *commands* est retourné après avoir été initialisé dans le dernier élément du tableau *flow*.
 * Cette structure control_flow_t est mise à jour pour que le champ *proc* pointe vers le processus ajouté et la liste est mise à jour de la manière suivante :
 * - Si *mode* est UNCONDITIONAL, *proc* est ajouté à la liste des processus à exécuter inconditionnellement après le processus courant.
 * - Si *mode* est ON_SUCCESS, *proc* est ajouté à la liste des processus à exécuter uniquement si le processus courant s'est terminé avec succès (code de retour 0).
 * - Si *mode* est ON_FAILURE, *proc* est ajouté à la liste des processus à exécuter uniquement si le processus courant s'est terminé avec un échec (code de retour non nul).
 */

processus_t* add_processus(command_line_t* cmdl, control_flow_mode_t mode) {
    if (!cmdl) return NULL;

    if (cmdl->num_commands >= MAX_CMDS) return NULL;

    int idx = cmdl->num_commands;
    cmdl->num_commands += 1;

    /* initialiser le processus à l'indice idx */
    processus_t* proc = &cmdl->commands[idx];
    init_processus(proc);

    /* initialiser le flow correspondant */
    control_flow_t* cf = &cmdl->flow[idx];
    init_control_flow(cf);
    cf->proc = proc;
    cf->cmdl = cmdl;

    /* relier le flow précédent vers ce nouveau selon le mode */
    if (idx > 0) {
        control_flow_t* prev = &cmdl->flow[idx - 1];
        if (mode == UNCONDITIONAL) prev->unconditionnal_next = cf;
        else if (mode == ON_SUCCESS) prev->on_success_next = cf;
        else if (mode == ON_FAILURE) prev->on_failure_next = cf;
    }

    proc->cf = cf;
    return proc;
}




/** @brief Fonction de récupération du prochain processus à exécuter selon le contrôle de flux.
 * @param cmdl Pointeur vers la structure de ligne de commande.
 * @return processus_t* Pointeur vers le prochain processus à exécuter, ou NULL si le nombre maximum est atteint.
 * @details Cette fonction retourne un pointeur vers la structure processus_t dans le tableau *commands* situé à l'indice *num_commands + 1*.
 *  Si le nombre maximum de commandes est atteint (MAX_CMDS), la fonction retourne NULL.
 *  Cela permet notamment d'initialiser les descripteurs des IOs standards qui dépendent du processus en court de traitement (dans le cas des pipes par exemple).
 */
processus_t* next_processus(command_line_t* cmdl) {
    if (!cmdl) return NULL;

    if (cmdl->num_commands >= MAX_CMDS) return NULL;

    return &cmdl->commands[cmdl->num_commands];
}


/** @brief Fonction d'ajout d'un descripteur de fichier à la structure de contrôle de flux.
 * @param cmdl Pointeur vers la structure de ligne de commande.
 * @param fd Descripteur de fichier à ajouter.
 * @return int 0 en cas de succès, -1 en cas d'erreur (tableau plein ou fd invalide).
 * @details Cette fonction ajoute le descripteur de fichier *fd* au tableau *opened_descriptors* de la structure *cf*.
 *    Si le tableau est plein ou si *fd* est invalide (négatif), la fonction retourne -1
 */
 
int add_fd(command_line_t* cmdl, int fd) {
    if (!cmdl || fd < 0) return -1;

    for (int i = 0; i < MAX_FDS; i++) {
        if (cmdl->opened_descriptors[i] == -1) {
            cmdl->opened_descriptors[i] = fd;
            return 0;
        }
    }

    return -1;
}


/** @brief Fonction de fermeture des descripteurs de fichiers listés dans la structure de contrôle de flux.
 * @param cmdl Pointeur vers la structure de ligne de commande.
 * @return int 0 en cas de succès, -1 en cas d'erreur.
 * @details Cette fonction ferme tous les descripteurs de fichiers listés dans le tableau *opened_descriptors* de la structure *cf*.
 *    Après fermeture, les entrées du tableau sont remises à -1.
 */
 
int close_fds(command_line_t* cmdl) {
    if (!cmdl) return -1;

    for (int i = 0; i < MAX_FDS; ++i) {
        int fd = cmdl->opened_descriptors[i];
        if (fd >= 0) {
            close(fd);
            cmdl->opened_descriptors[i] = -1;
        }
    }

    return 0;
}

/** @brief Fonction d'initialisation d'une structure de ligne de commande.
 * @param cmdl Pointeur vers la structure de ligne de commande à initialiser.
 * @return int 0 en cas de succès, -1 en cas d'erreur.
 * @details Cette fonction initialise les champs de la structure avec les valeurs suivantes:
 * - *command_line*: "\0"
 * - *tokens*: {NULL}
 * - *commands*: tableau de processus initialisés via *init_processus()*
 * - *flow*: tableau de contrôle de flux initialisé via *init_control_flow()*
 * - *num_commands*: 0
 * - *opened_descriptors*: {-1}
 */
 
 int init_command_line(command_line_t* cmdl) {
    if (!cmdl) return -1;

    /* clear command line string */
    cmdl->command_line[0] = '\0';

    /* tokens */
    for (int i = 0; i < MAX_TOKENS; ++i) cmdl->tokens[i] = NULL;

    /* commands and flow */
    cmdl->num_commands = 0;
    for (int i = 0; i < MAX_CMDS; ++i) {
        init_processus(&cmdl->commands[i]);
        init_control_flow(&cmdl->flow[i]);
        cmdl->flow[i].cmdl = cmdl;
    }

    /* opened descriptors */
    for (int i = 0; i < MAX_FDS; ++i) cmdl->opened_descriptors[i] = -1;

    return 0;
}




/** @brief Fonction de lancement d'une ligne de commande.
 * @param cmdl Pointeur vers la structure de ligne de commande à lancer.
 * @return int 0 en cas de succès, un code d'erreur sinon.
 * @details Cette fonction lance les processus selon le flux défini dans la structure *cmdl*. Les lancements sont effectués via *launch_processus()* en
 *    respectant les conditions de contrôle de flux (inconditionnel, en cas de succès, en cas d'échec).
 *    Le tableau *opened_descriptors* est utilisé pour fermer les descripteurs ouverts au moment de l'initialisation des structures processus_t.
 *    La fonction retourne 0 si tous les processus à lancer en fonction du contrôle de flux ont pu être lancés sans erreur.
 */
int launch_command_line(command_line_t* cmdl) {
    if (!cmdl) return -1;
    if (cmdl->num_commands == 0) return 0;

    /* start from the first flow */
    control_flow_t* cf = &cmdl->flow[0];

    while (cf && cf->proc) {
        processus_t* p = cf->proc;

        int ret = launch_processus(p);
        /* si erreur non nulle (et non-code de succès 0) on peut décider d'interrompre */
        if (ret < 0) {
            /* arrêter si erreur fatale */
            return ret;
        }

        /* décider du prochain noeud selon status */
        int success = 0;
        if (WIFEXITED(p->status)) {
            success = (WEXITSTATUS(p->status) == 0);
        } else {
            success = 0;
        }

        if (success && cf->on_success_next) cf = cf->on_success_next;
        else if (!success && cf->on_failure_next) cf = cf->on_failure_next;
        else cf = cf->unconditionnal_next;
    }

    /* fermer les fds ouverts pour cette ligne de commande */
    close_fds(cmdl);


    return 0;
    
}

void free_processus(processus_t* p) {
    for (int i = 0; i < MAX_ARGS; i++) {
        if (p->argv[i])
            free(p->argv[i]);
    }
}


