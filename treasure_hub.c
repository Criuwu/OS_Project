#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>

#define MAX_CMD 256

pid_t monitor_pid = -1;
int monitor_running = 0;
int pipefd[2];

void sigchld_handler(int sig) {
    int status;
    waitpid(monitor_pid, &status, 0);
    monitor_running = 0;
    printf("\033[1;31m Monitor process terminated. Exit status: %d\n \033[0m ", WEXITSTATUS(status));
}

void send_command_to_monitor(const char *command) {
    FILE *fp = fopen("hub_command.txt", "w");
    if (!fp) {
        perror("Failed to write command");
        return;
    }
    fprintf(fp, "%s\n", command);
    fclose(fp);
    kill(monitor_pid, SIGUSR1);

    // Read output from the monitor via the pipe
    char buffer[1024];
    ssize_t n;
    while ((n = read(pipefd[0], buffer, sizeof(buffer)-1)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
        break;
    }
}

void run_score_calculator(const char *hunt_name) {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("Pipe failed");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        execl("./score_calculator", "score_calculator", hunt_name, NULL);
        perror("Failed to exec score_calculator");
        exit(1);
    } else if (pid > 0) {
        // Parent
        close(fd[1]);
        char buffer[1024];
        ssize_t n;
        while ((n = read(fd[0], buffer, sizeof(buffer)-1)) > 0) {
            buffer[n] = '\0';
            printf("%s", buffer);
        }
        close(fd[0]);
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
}

void calculate_scores() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            char path[128];
            snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name);
            if (access(path, F_OK) == 0) {
                printf("\033[1;33mScores for hunt: %s\033[0m\n", entry->d_name);
                run_score_calculator(entry->d_name);
                printf("\n");
            }
        }
    }
    closedir(dir);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);

    char input[256];

    while (1) {
        printf("\033[0;36m >> \033[0m");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitor already running.\n");
                continue;
            }

            if (pipe(pipefd) == -1) {
                perror("pipe");
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == 0) {
                // Child: set pipe for writing
                close(pipefd[0]);
                char fd_str[10];
                snprintf(fd_str, sizeof(fd_str), "%d", pipefd[1]);
                setenv("MONITOR_PIPE", fd_str, 1);
                execl("./monitor", "monitor", NULL);
                perror("Failed to exec monitor");
                exit(1);
            } else {
                // Parent
                close(pipefd[1]);
                monitor_running = 1;
                printf("Monitor started with PID %d\n", monitor_pid);
            }

        } else if (strncmp(input, "list_hunts", 10) == 0 ||
                   strncmp(input, "list_treasures", 14) == 0 ||
                   strncmp(input, "view_treasure", 13) == 0) {
            if (!monitor_running) {
                printf("Monitor is not running.\n");
                continue;
            }
            send_command_to_monitor(input);

        } else if (strcmp(input, "stop_monitor") == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            send_command_to_monitor("stop_monitor");

        } else if (strcmp(input, "calculate_score") == 0) {
            calculate_scores();
        } else if (strcmp(input, "exit") == 0) {
            if (monitor_running) {
                printf("\033[1;31m Monitor is still running. Stop it before exiting. \033[0m  \n");
            } else {
                break;
            }
        } else {
            printf("Unknown command\n");
        }
    }

    return 0;
}
