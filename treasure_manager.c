#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <errno.h> 

//weekeND frumos, hai la vot

#define MAX_USERNAME 32
#define MAX_CLUE 128
#define RECORD_SIZE sizeof(struct Treasure)

// Structure to hold treasure information
struct Treasure 
{
    int id;
    char username[MAX_USERNAME];
    float latitude;
    float longitude;
    char clue[MAX_CLUE];
    int value;
};

// This function generates a unique ID for a new treasure
int get_next_id(const char *hunt_id) 
{
    char id_path[128];
    snprintf(id_path, sizeof(id_path), "%s/id_counter", hunt_id);

    int fd = open(id_path, O_RDWR | O_CREAT, 0644);
    if (fd < 0) return -1;

    int id = 0;
    read(fd, &id, sizeof(int));
    lseek(fd, 0, SEEK_SET);
    id++;
    write(fd, &id, sizeof(int));
    close(fd);
    return id;
}

// This function updates the IDs of treasures after one is removed
void update_ids_after_removal(const char *hunt_id) 
{
    char file_path[128], temp_path[128];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
    snprintf(temp_path, sizeof(temp_path), "%s/tmp.dat", hunt_id);

    // Open the original file and a temporary file
    int fd = open(file_path, O_RDONLY);
    int temp_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0 || temp_fd < 0) 
    {
        perror("Error opening files for ID update");
        return;
    }

    // Read treasures and reassign IDs
    struct Treasure t;
    int new_id = 1;
    while (read(fd, &t, RECORD_SIZE) == RECORD_SIZE) 
    {
        t.id = new_id++;
        write(temp_fd, &t, RECORD_SIZE);
    }

    close(fd);
    close(temp_fd);
    rename(temp_path, file_path);

    // Update id_counter
    snprintf(file_path, sizeof(file_path), "%s/id_counter", hunt_id);
    fd = open(file_path, O_WRONLY | O_TRUNC);
    if (fd >= 0) 
    {
        write(fd, &new_id, sizeof(int));
        close(fd);
    }
}

// This function writes the actions performed in the hunt in a log file
void log_action(const char *hunt_id, const char *action) 
{
    char log_path[128];
    snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_id);
    int fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) 
    {
        perror("Error opening log file");
        return;
    }
    dprintf(fd, "%s\n", action);
    close(fd);

    // Create a symlink to the log file for easy access
    char link_name[128];
    snprintf(link_name, sizeof(link_name), "logged_hunt-%s", hunt_id);
    unlink(link_name);
    symlink(log_path, link_name);
}

// This function creates a new treasure in the hunt directory and initializes the ID counter
void add_treasure(const char *hunt_id) 
{
    char dir[64];
    snprintf(dir, sizeof(dir), "%s", hunt_id);

    if (mkdir(dir, 0755) == -1 && errno != EEXIST) // Check if directory already exists and if it can be created
    {
        perror("Failed to create hunt directory");
        return;
    }

    int new_id = get_next_id(hunt_id);
    if (new_id < 0) 
    {
        fprintf(stderr, "Failed to generate unique ID\n");
        return;
    }

    char file_path[128];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", dir);

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) 
    {
        perror("Cannot open treasure file");
        return;
    }

    struct Treasure t;
    t.id = new_id;
    printf("Enter Username: "); scanf("%31s", t.username);
    printf("Enter Latitude: "); scanf("%f", &t.latitude);
    printf("Enter Longitude: "); scanf("%f", &t.longitude);
    printf("Enter Clue: "); getchar(); fgets(t.clue, MAX_CLUE, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0;
    printf("Enter Value: "); scanf("%d", &t.value);

    write(fd, &t, RECORD_SIZE);
    close(fd);

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Added treasure ID %d", t.id);
    log_action(hunt_id, log_msg); //writes the action in the log file
}

// This function lists all treasures in a hunt directory
void list_treasures(const char *hunt_id) 
{
    char file_path[128];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) 
    {
        perror("Cannot open file");
        return;
    }

    struct stat st;
    stat(file_path, &st);

    // Print hunt information
    printf("Hunt: %s\nFile size: %ld bytes\nLast modified: %s\n",
           hunt_id, st.st_size, ctime(&st.st_mtime));

    struct Treasure t;
    while (read(fd, &t, RECORD_SIZE) == RECORD_SIZE) 
    {
        printf("\033[1;31m ID:\033[0m%d, \033[1;31m User:\033[0m%s, \033[1;31m Lat:\033[0m%.2f, \033[1;31m Lon:\033[0m%.2f, \033[1;31m Clue:\033[0m%s, \033[1;31m Value:\033[0m%d\n",
               t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
    }
    close(fd);
    log_action(hunt_id, "Listed treasures"); // Log the action of listing treasures
}

// This function views a specific treasure by its ID in a hunt directory
void view_treasure(const char *hunt_id, const char *tid_str) 
{
    int tid = atoi(tid_str);
    char file_path[128];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) 
    {
        perror("Cannot open file");
        return;
    }

    // Traverse the file and find the treasure with the given ID
    struct Treasure t;
    while(read(fd, &t, RECORD_SIZE) == RECORD_SIZE) 
    {
        // Check if the current treasure's ID matches the requested ID and print its details
        if (t.id == tid) 
        {
            printf("Found Treasure:\nID: %d\nUser: %s\nLat: %.2f\nLon: %.2f\nClue: %s\nValue: %d\n",
                   t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
            close(fd);
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Viewed treasure %d", tid);
            log_action(hunt_id, log_msg);
            return;
        }
    }
    printf("Treasure not found\n");
    close(fd);
}

// This function removes a treasure by its ID from the hunt directory
void remove_treasure(const char *hunt_id, const char *tid_str) 
{
    int tid = atoi(tid_str);
    char file_path[128], temp_path[128];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
    snprintf(temp_path, sizeof(temp_path), "%s/tmp.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    int temp_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0 || temp_fd < 0) // Check if files can be opened
    {
        perror("Error opening files");
        return;
    }

    struct Treasure t;
    int found = 0;
    // Read treasures and write to temporary file, skipping the one to be removed
    while (read(fd, &t, RECORD_SIZE) == RECORD_SIZE) 
    {
        if (t.id != tid) 
        {
            write(temp_fd, &t, RECORD_SIZE);
        } else 
        {
            found = 1;
        }
    }
    close(fd);
    close(temp_fd);
    rename(temp_path, file_path);

    if(found) // Treasure was found and removed, update IDs and log the action
    {
        printf("Treasure %d removed. IDs will be updated.\n", tid);
        update_ids_after_removal(hunt_id);
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Removed treasure %d and updated IDs", tid);
        log_action(hunt_id, log_msg);
    } else 
    {
        printf("Treasure ID not found.\n");
    }
}

// This function removes a hunt directory and all its contents
void remove_hunt(const char *hunt_id) 
{
    char file_path[128];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
    unlink(file_path);

    snprintf(file_path, sizeof(file_path), "%s/logged_hunt", hunt_id);
    unlink(file_path);

    snprintf(file_path, sizeof(file_path), "%s/id_counter", hunt_id);
    unlink(file_path);

    rmdir(hunt_id);

    char link_name[128];
    snprintf(link_name, sizeof(link_name), "logged_hunt-%s", hunt_id);
    unlink(link_name);

    printf("Hunt %s removed.\n", hunt_id);
}

// This function prints the rules and commands for using the treasure manager
void print_rules()
{
    printf("\033[1;35m");printf("\nWelcome to the treasure manager\n");printf("\033[0m");
    printf("\nTo be able to do any changes you can use the following commands: \n");
    printf("\033[1;35m"); printf("--add"); printf("\033[0m"); printf(" <hunt_name> to add a treasure in a hunt\n");
    printf("\033[1;35m"); printf("--list"); printf("\033[0m"); printf(" <hunt_name> to view the treasures in a hunt\n");
    printf("\033[1;35m"); printf("--view"); printf("\033[0m");printf(" <hunt_name> <treasure_id> to view the details of a treasure\n");
    printf("\033[1;35m"); printf("--remove_treasure"); printf("\033[0m");printf(" <hunt_name> <treasure_id> to remove a treasure from a hunt\n");
    printf("\033[1;35m"); printf("--remove_hunt"); printf("\033[0m");printf(" <hunt_name> to remove a hunt\n \n");

}

// This function calculates the scores for a hunt by calling an external score calculator
void calculate_hunt_scores(const char *hunt_id) 
{
    char command[256];
    snprintf(command, sizeof(command), "./score_calculator %s", hunt_id);
    system(command); // Execute the score calculator with the hunt ID
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s --<operation>\n", argv[0]);
        return 1;
    }


    if (strcmp(argv[1], "--rules") == 0 && argc == 2) 
    {
        print_rules();
    }
    else if (strcmp(argv[1], "--add") == 0 && argc == 3) 
    {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0 && argc == 3) 
    {
        list_treasures(argv[2]);

    } else if (strcmp(argv[1], "--view") == 0 && argc == 4) 
    {
        view_treasure(argv[2], argv[3]);

    } else if (strcmp(argv[1], "--remove_treasure") == 0 && argc == 4) 
    {
        remove_treasure(argv[2], argv[3]);

    } else if (strcmp(argv[1], "--remove_hunt") == 0 && argc == 3) 
    {
        remove_hunt(argv[2]);

    } else if (strcmp(argv[1], "--calculate_scores") == 0 && argc == 3) 
    {
        calculate_hunt_scores(argv[2]);
        
    }else 
    {
        fprintf(stderr, "Invalid command or arguments.\n");
        return 1;
    }

    return 0;
}
