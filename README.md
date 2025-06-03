
# OS PROJECT

- **`treasure_manager.c`** — program to manage treasures
- **`treasure_hub.c`** — Interactive shell that manages a background `monitor` process.
- **`monitor.c`** — Signal-based background process, reads commands from a file and responds via pipes
- **`scozre_calculator.c`** — Extrenal program that computes the total score


# Commands for running the program

## `treasure_manager.c`

  - ***--rules*** : Displays the rules and commands that can be used

  - ***--add <hunt_id>*** : Add a new treasure to the specified hunt (game session).     Each hunt is stored in a separate directory.

  - ***--list <hunt_id>*** : List all treasures in the specified hunt. First print the hunt name, the (total) file size and last modification time of its treasure file(s), then list the treasures.

  - ***--view <hunt_id> <id>*** : View details of a specific treasure
  
  - ***--remove_treasure <hunt_id> <id>*** : Remove a treasure 

  - ***--remove_hunt <hunt_id>*** : Remove an entire hunt


## `treasure_hub.c`

  - ***start_monitor*** : starts a separate background process that monitors the hunts and prints to the standard output information about them when asked to
  
  - ***list_hunts*** : asks the monitor to list the hunts and the total number of treasures in each

  - ***list_treasures <hunt_id>***: tells the monitor to show the information about all treasures in a hunt, the same way as the command line at the previous stage did
  
  - ***view_treasure <hunt_id> <id>***: tells the monitor to show the information about a treasure in hunt, the same way as the command line at the previous stage did
  
  - ***stop_monitor***: asks the monitor to end then returns to the prompt. Prints monitor's  termination state when it ends.

   - ***calculate_score*** : launches a separate score_calculator process for each existing hunt. (can run without starting the monitor)
  
  - ***exit*** : if the monitor still runs, prints an error message, otherwise ends the program


## `score_calculator.c`

This external program is invoked by treasure_hub when the calculate_score command is issued.

  - ***./score_calculator*** : calculates the scores for all the hunts