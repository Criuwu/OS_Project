#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define RECORD_SIZE sizeof(struct Treasure)
#define MAX_USERNAME 32
#define MAX_CLUE 128

struct Treasure {
    int id;
    char username[MAX_USERNAME];
    float latitude;
    float longitude;
    char clue[MAX_CLUE];
    int value;
};

volatile sig_atomic_t sigusr1_received = 0;
int out_fd = STDOUT_FILENO;

void handle_sigusr1(int sig) {
    sigusr1_received = 1;
}

void print_to_pipe(const char *format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    write(out_fd, buffer, strlen(buffer));
}

void list_hunts() {
    DIR *d = opendir(".");
    if (!d) return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            char path[128];
            snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name);

            int count = 0;
            int fd = open(path, O_RDONLY);
            if (fd >= 0) {
                struct Treasure t;
                while (read(fd, &t, RECORD_SIZE) == RECORD_SIZE) count++;
                close(fd);
            }

            print_to_pipe("Hunt: %s, Treasures: %d\n", entry->d_name, count);
        }
    }
    closedir(d);
}

void list_treasures(const char *hunt) {
    char path[128];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        print_to_pipe("Failed to open treasures.dat :'(\n");
        return;
    }

    struct Treasure t;
    while (read(fd, &t, RECORD_SIZE) == RECORD_SIZE) {
        print_to_pipe("ID: %d, User: %s, Lat: %.2f, Lon: %.2f, Clue: %s, Value: %d\n",
               t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
    }
    close(fd);
}

void view_treasure(const char *hunt, int id) {
    char path[128];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        print_to_pipe("Failed to open treasures.dat\n");
        return;
    }

    struct Treasure t;
    while (read(fd, &t, RECORD_SIZE) == RECORD_SIZE) {
        if (t.id == id) {
            print_to_pipe("Treasure: ID: %d, User: %s, Lat: %.2f, Lon: %.2f, Clue: %s, Value: %d\n",
                   t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
            break;
        }
    }
    close(fd);
}

void process_command() {
    FILE *fp = fopen("hub_command.txt", "r");
    if (!fp) return;

    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return;
    }
    fclose(fp);

    line[strcspn(line, "\n")] = 0;

    if (strcmp(line, "list_hunts") == 0) {
        list_hunts();
    } else if (strncmp(line, "list_treasures", 14) == 0) {
        char *hunt = strchr(line, ' ');
        if (hunt) list_treasures(hunt + 1);
    } else if (strncmp(line, "view_treasure", 13) == 0) {
        char *args = strchr(line, ' ');
        if (args) {
            char hunt[64];
            int id;
            sscanf(args + 1, "%s %d", hunt, &id);
            view_treasure(hunt, id);
        }
    } else if (strcmp(line, "stop_monitor") == 0) {
        print_to_pipe("Stopping monitor...\n");
        usleep(3000000); // Simulate delay (3 seconds)
        exit(0);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    char *pipe_env = getenv("MONITOR_PIPE");
    if (pipe_env) {
        out_fd = atoi(pipe_env);
    }

    print_to_pipe("Monitor running. PID: %d\n", getpid());

    while (1) {
        pause();
        if (sigusr1_received) {
            sigusr1_received = 0;
            process_command();
        }
    }
    return 0;
}
