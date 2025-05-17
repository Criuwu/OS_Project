#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

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

#define RECORD_SIZE sizeof(struct Treasure)

typedef struct {
    char username[MAX_USERNAME];
    int total_score;
} UserScore;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_directory>\n", argv[0]);
        return 1;
    }

    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%s/treasures.dat", argv[1]);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Cannot open treasures.dat");
        return 1;
    }

    struct Treasure t;
    UserScore scores[100];
    int user_count = 0;

    while (read(fd, &t, RECORD_SIZE) == RECORD_SIZE) {
        int found = 0;
        for (int i = 0; i < user_count; i++) {
            if (strcmp(scores[i].username, t.username) == 0) {
                scores[i].total_score += t.value;
                found = 1;
                break;
            }
        }
        if (!found) {
            strncpy(scores[user_count].username, t.username, MAX_USERNAME);
            scores[user_count].total_score = t.value;
            user_count++;
        }
    }
    close(fd);

    for (int i = 0; i < user_count; i++) {
        printf("%s: %d\n", scores[i].username, scores[i].total_score);
    }

    return 0;
}
