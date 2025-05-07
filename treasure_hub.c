#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

pid_t monitor_pid = -1;
int monitor_running = 0;

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

            monitor_pid = fork();
            if (monitor_pid == 0) {
                execl("./monitor", "monitor", NULL);
                perror("Failed to exec monitor");
                exit(1);
            } else {
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
