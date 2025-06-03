#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>

pid_t monitor_pid = -1;
int monitor_running = 0;
int pipe_fds[2] = {-1, -1};

// Function to handle signal SIGCHLD
// This function is called when the monitor process terminates
void sigchld_handler(int sig) 
{
    int status;
    waitpid(monitor_pid, &status, 0);
    monitor_running = 0;
    printf("\033[1;31m Monitor process terminated. Exit status: %d\n \033[0m ", WEXITSTATUS(status));
}

// Function to send commands to the monitor process
// This function writes the command to a file and signals the monitor to read it
void send_command_to_monitor(const char *command) 
{
    FILE *fp = fopen("hub_command.txt", "w");
    if (!fp) 
    {
        perror("Failed to write command");
        return;
    }
    fprintf(fp, "%s\n", command);
    fclose(fp);
    kill(monitor_pid, SIGUSR1);
}

void read_monitor_output()
{
    char buffer[1024];
    ssize_t bytes_read;

    fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK); // Set the read end of the pipe to non-blocking mode, so that it does not wait if there is no data to read
    
    while((bytes_read = read(pipe_fds[0], buffer, sizeof(buffer) - 1)) > 0) 
    {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
}

void calculate_scores() 
{
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - run score calculator
        execl("./score_calculator", "score_calculator", NULL);
        perror("Failed to exec score_calculator");
        exit(1);
    } else if (pid > 0) {
        // Parent process - wait for completion
        waitpid(pid, NULL, 0);
    } else {
        perror("fork failed");
    }
}

int main() 
{

    struct sigaction sa;
    sa.sa_handler = sigchld_handler; // Pointer to your signal handler function
    sigemptyset(&sa.sa_mask);  // Block no other signals while in handler
    sa.sa_flags = 0; // Flags for extra behavior

    sigaction(SIGCHLD, &sa, NULL);

    if (pipe(pipe_fds) == -1) 
    {
        perror("pipe failed");
        return 1;
    }

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
            if (monitor_pid == 0) 
            {
                // Child process - monitor
                close(pipe_fds[0]);
                char fd_str[16];
                snprintf(fd_str, sizeof(fd_str), "%d", pipe_fds[1]);
                execl("./monitor", "monitor", fd_str, NULL);
                perror("Failed to exec monitor");
                exit(1);
            } else {
                // Parent process
                close(pipe_fds[1]);
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
            usleep(100000); // Small delay to allow monitor to process
            read_monitor_output();

        } else if (strcmp(input, "stop_monitor") == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            send_command_to_monitor("stop_monitor");
            usleep(100000);
            read_monitor_output();

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

    close(pipe_fds[0]);
    if (pipe_fds[1] != -1) close(pipe_fds[1]);
    return 0;
}