#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>  // For PATH_MAX

#define MAX_USERNAME 32
#define MAX_CLUE 128
#define RECORD_SIZE sizeof(struct Treasure)
#define MAX_HUNTS 50
#define MAX_USERS 100

struct Treasure {
    int id;
    char username[MAX_USERNAME];
    float latitude;
    float longitude;
    char clue[MAX_CLUE];
    int value;
};

struct UserScore {
    char username[MAX_USERNAME];
    int total_value;
};

void calculate_scores_for_hunt(const char *hunt_name) {
    char path[PATH_MAX];  // Use system-defined maximum path length
    int needed = snprintf(path, sizeof(path), "%s/treasures.dat", hunt_name);
    
    if (needed >= sizeof(path)) {
        fprintf(stderr, "Path too long for hunt: %s\n", hunt_name);
        return;
    }

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open treasures file for hunt %s\n", hunt_name);
        return;
    }

    struct UserScore scores[MAX_USERS];
    int num_users = 0;

    struct Treasure t;
    while (fread(&t, RECORD_SIZE, 1, fp) == 1) {
        int found = 0;
        for (int i = 0; i < num_users; i++) {
            if (strcmp(scores[i].username, t.username) == 0) {
                scores[i].total_value += t.value;
                found = 1;
                break;
            }
        }
        if (!found && num_users < MAX_USERS) {
            strncpy(scores[num_users].username, t.username, MAX_USERNAME);
            scores[num_users].username[MAX_USERNAME - 1] = '\0';  // Ensure null-termination
            scores[num_users].total_value = t.value;
            num_users++;
        }
    }
    fclose(fp);

    // Sort scores in descending order
    for (int i = 0; i < num_users - 1; i++) {
        for (int j = 0; j < num_users - i - 1; j++) {
            if (scores[j].total_value < scores[j+1].total_value) {
                struct UserScore temp = scores[j];
                scores[j] = scores[j+1];
                scores[j+1] = temp;
            }
        }
    }

    printf("\n\033[1;36m=== Scores for hunt '%s' ===\033[0m\n", hunt_name);
    for (int i = 0; i < num_users; i++) {
        printf("%-20s: %5d points\n", scores[i].username, scores[i].total_value);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // Calculate scores for all hunts
        DIR *d = opendir(".");
        if (!d) {
            perror("Failed to open directory");
            return 1;
        }

        struct dirent *entry;
        while ((entry = readdir(d)) != NULL) {
            if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
                char path[PATH_MAX];
                int needed = snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name);
                
                if (needed < sizeof(path) && access(path, F_OK) == 0) {
                    calculate_scores_for_hunt(entry->d_name);
                }
            }
        }
        closedir(d);
    } else if (argc == 2) {
        // Calculate scores for specific hunt
        calculate_scores_for_hunt(argv[1]);
    } else {
        fprintf(stderr, "Usage: %s [hunt_name]\n", argv[0]);
        return 1;
    }

    return 0;
}