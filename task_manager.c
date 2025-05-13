#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Include pthread.h before time.h to avoid redefinition issues
#include <pthread.h>
#include <time.h>

#include <stdbool.h>

// Cross-platform sleep function
#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep((x)*1000)
#else
    #include <unistd.h>
#endif

#define MAX_TASKS 100
#define MAX_DESCRIPTION 256
#define FILENAME "tasks.dat"
#define MAX_SIMULTANEOUS_TASKS 10

typedef enum {
    LOW = 5,
    MEDIUM = 3,
    HIGH = 1
} Priority;

typedef struct {
    int id;
    char description[MAX_DESCRIPTION];
    Priority priority;
    int duration; // in seconds
    time_t created;
    bool completed;
} Task;

typedef struct {
    Task *task;
    int *isRunning;
    int taskIndex;
} ThreadArgs;

Task tasks[MAX_TASKS];
int taskCount = 0;
int nextTaskId = 1;

// Function prototypes
void clearInputBuffer();
void saveTasksToFile();
void loadTasksFromFile();
void addTask();
void viewTasks();
void searchTasks();
void deleteTask();
void modifyTask();
void sortTasks();
void executeTasks();
void executeSpecificTask();
void executeMultipleTasks();
void showMenu();
void displayTaskDetails(Task task);
const char* priorityToString(Priority p);
bool isDuplicateTask(const char* description, Priority priority, int duration);
void* executeTaskThread(void* arg);

// Helper function to clear input buffer
void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Save tasks to file
void saveTasksToFile() {
    FILE *file = fopen(FILENAME, "wb");
    
    if (file == NULL) {
        printf("Error: Cannot open file for writing.\n");
        return;
    }
    
    // Write the next task ID first
    fwrite(&nextTaskId, sizeof(int), 1, file);
    
    // Write the number of tasks
    fwrite(&taskCount, sizeof(int), 1, file);
    
    // Write each task
    fwrite(tasks, sizeof(Task), taskCount, file);
    
    fclose(file);
    printf("Tasks saved to %s\n", FILENAME);
}

// Load tasks from file
void loadTasksFromFile() {
    FILE *file = fopen(FILENAME, "rb");
    
    if (file == NULL) {
        printf("No saved tasks found. Starting with empty task list.\n");
        return;
    }
    
    // Read the next task ID
    fread(&nextTaskId, sizeof(int), 1, file);
    
    // Read the number of tasks
    fread(&taskCount, sizeof(int), 1, file);
    
    if (taskCount > MAX_TASKS) {
        printf("Warning: File contains more tasks than maximum allowed. Loading only %d tasks.\n", MAX_TASKS);
        taskCount = MAX_TASKS;
    }
    
    // Read each task
    fread(tasks, sizeof(Task), taskCount, file);
    
    fclose(file);
    printf("Loaded %d tasks from %s\n", taskCount, FILENAME);
}

// Convert priority enum to string
const char* priorityToString(Priority p) {
    switch (p) {
        case HIGH: return "High";
        case MEDIUM: return "Medium";
        case LOW: return "Low";
        default: return "Unknown";
    }
}

// Function to check if a task with similar description and properties already exists
bool isDuplicateTask(const char* description, Priority priority, int duration) {
    for (int i = 0; i < taskCount; i++) {
        // Check for exact description match
        if (strcmp(tasks[i].description, description) == 0) {
            printf("\n⚠️ Similar task already exists! ⚠️\n");
            displayTaskDetails(tasks[i]);
            printf("Do you still want to add this task? (1=Yes, 0=No): ");
            int confirm;
            scanf("%d", &confirm);
            return confirm != 1;
        }
    }
    return false;
}

// Function to display a single task with details
void displayTaskDetails(Task task) {
    char timeStr[30];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&task.created));
    
    printf("┌──────────────────────────────────────────────────────────────┐\n");
    printf("│ Task ID: %-52d │\n", task.id);
    printf("├──────────────────────────────────────────────────────────────┤\n");
    printf("│ Description: %-48s │\n", task.description);
    printf("│ Priority:    %-48s │\n", priorityToString(task.priority));
    printf("│ Duration:    %-2d seconds                                      │\n", task.duration);
    printf("│ Created:     %-48s │\n", timeStr);
    printf("│ Status:      %-48s │\n", task.completed ? "Completed" : "Pending");
    printf("└──────────────────────────────────────────────────────────────┘\n");
}

// Function to add a new task
void addTask() {
    if (taskCount >= MAX_TASKS) {
        printf("Task limit reached.\n");
        return;
    }

    Task t;
    t.id = nextTaskId++;
    t.created = time(NULL);
    t.completed = false;

    printf("\n=== Adding New Task ===\n");
    printf("Task ID: %d (auto-assigned)\n", t.id);
    
    clearInputBuffer();
    printf("Enter description: ");
    fgets(t.description, MAX_DESCRIPTION, stdin);
    t.description[strcspn(t.description, "\n")] = 0;
    
    int priorityChoice;
    do {
        printf("Select priority:\n");
        printf("1. High\n");
        printf("2. Medium\n");
        printf("3. Low\n");
        printf("Choice: ");
        scanf("%d", &priorityChoice);
        
        switch (priorityChoice) {
            case 1: t.priority = HIGH; break;
            case 2: t.priority = MEDIUM; break;
            case 3: t.priority = LOW; break;
            default: printf("Invalid choice. Try again.\n");
        }
    } while (priorityChoice < 1 || priorityChoice > 3);

    do {
        printf("Enter duration (in seconds, 1-3600): ");
        scanf("%d", &t.duration);
        
        if (t.duration < 1 || t.duration > 3600) {
            printf("Invalid duration. Please enter a value between 1 and 3600 seconds.\n");
        }
    } while (t.duration < 1 || t.duration > 3600);

    // Check for duplicate tasks
    if (isDuplicateTask(t.description, t.priority, t.duration)) {
        return;
    }

    tasks[taskCount++] = t;
    printf("\nTask added successfully!\n");
    
    saveTasksToFile();
}

// Function to display all tasks
void viewTasks() {
    if (taskCount == 0) {
        printf("\nNo tasks available.\n");
        return;
    }

    printf("\n=== Task List (%d tasks) ===\n", taskCount);
    printf("┌─────┬───────────────────────────────┬──────────┬──────────┬──────────┐\n");
    printf("│ ID  │ Description                   │ Priority │ Duration │ Status   │\n");
    printf("├─────┼───────────────────────────────┼──────────┼──────────┼──────────┤\n");
    
    for (int i = 0; i < taskCount; i++) {
        // Truncate description if too long for display
        char shortDesc[30];
        strncpy(shortDesc, tasks[i].description, 25);
        shortDesc[25] = '\0';
        if (strlen(tasks[i].description) > 25) {
            strcat(shortDesc, "...");
        }
        
        printf("│ %-3d │ %-27s │ %-8s │ %-8d │ %-8s │\n", 
               tasks[i].id, 
               shortDesc, 
               priorityToString(tasks[i].priority), 
               tasks[i].duration,
               tasks[i].completed ? "Done" : "Pending");
    }
    printf("└─────┴───────────────────────────────┴──────────┴──────────┴──────────┘\n");
    
    printf("\nEnter task ID for details or 0 to return: ");
    int id;
    scanf("%d", &id);
    
    if (id != 0) {
        for (int i = 0; i < taskCount; i++) {
            if (tasks[i].id == id) {
                displayTaskDetails(tasks[i]);
                return;
            }
        }
        printf("Task not found.\n");
    }
}

// Search for tasks
void searchTasks() {
    if (taskCount == 0) {
        printf("\nNo tasks available to search.\n");
        return;
    }
    
    printf("\n=== Search Tasks ===\n");
    printf("1. Search by keyword\n");
    printf("2. Search by priority\n");
    printf("3. Return to main menu\n");
    printf("Choice: ");
    
    int choice;
    scanf("%d", &choice);
    
    switch (choice) {
        case 1: {
            char keyword[50];
            clearInputBuffer();
            printf("Enter keyword: ");
            fgets(keyword, sizeof(keyword), stdin);
            keyword[strcspn(keyword, "\n")] = 0;
            
            printf("\n=== Search Results ===\n");
            int found = 0;
            
            for (int i = 0; i < taskCount; i++) {
                if (strstr(tasks[i].description, keyword) != NULL) {
                    displayTaskDetails(tasks[i]);
                    found++;
                }
            }
            
            if (found == 0) {
                printf("No tasks found matching '%s'\n", keyword);
            } else {
                printf("%d task(s) found.\n", found);
            }
            break;
        }
        case 2: {
            int priorityChoice;
            printf("Select priority to search for:\n");
            printf("1. High\n");
            printf("2. Medium\n");
            printf("3. Low\n");
            printf("Choice: ");
            scanf("%d", &priorityChoice);
            
            Priority searchPriority;
            switch (priorityChoice) {
                case 1: searchPriority = HIGH; break;
                case 2: searchPriority = MEDIUM; break;
                case 3: searchPriority = LOW; break;
                default: printf("Invalid choice.\n"); return;
            }
            
            printf("\n=== Search Results ===\n");
            int found = 0;
            
            for (int i = 0; i < taskCount; i++) {
                if (tasks[i].priority == searchPriority) {
                    displayTaskDetails(tasks[i]);
                    found++;
                }
            }
            
            if (found == 0) {
                printf("No tasks found with %s priority\n", priorityToString(searchPriority));
            } else {
                printf("%d task(s) found.\n", found);
            }
            break;
        }
        case 3:
            return;
        default:
            printf("Invalid choice.\n");
    }
}

// Delete a task
void deleteTask() {
    if (taskCount == 0) {
        printf("\nNo tasks available to delete.\n");
        return;
    }
    
    printf("\n=== Delete Task ===\n");
    
    // Display a compact list of tasks
    printf("Current tasks:\n");
    for (int i = 0; i < taskCount; i++) {
        printf("%d: %s (%s)\n", 
               tasks[i].id, 
               tasks[i].description, 
               priorityToString(tasks[i].priority));
    }
    
    printf("\nEnter task ID to delete (or 0 to cancel): ");
    int id;
    scanf("%d", &id);
    
    if (id == 0) return;
    
    // Find and delete the task
    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].id == id) {
            printf("Deleting task: %s\n", tasks[i].description);
            printf("Are you sure? (1=Yes, 0=No): ");
            int confirm;
            scanf("%d", &confirm);
            
            if (confirm == 1) {
                // Shift all tasks down to fill the gap
                for (int j = i; j < taskCount - 1; j++) {
                    tasks[j] = tasks[j + 1];
                }
                taskCount--;
                printf("Task deleted successfully.\n");
                saveTasksToFile();
            } else {
                printf("Deletion cancelled.\n");
            }
            return;
        }
    }
    
    printf("Task with ID %d not found.\n", id);
}

// Modify a task
void modifyTask() {
    if (taskCount == 0) {
        printf("\nNo tasks available to modify.\n");
        return;
    }
    
    printf("\n=== Modify Task ===\n");
    
    // Display a compact list of tasks
    printf("Current tasks:\n");
    for (int i = 0; i < taskCount; i++) {
        printf("%d: %s (%s)\n", 
               tasks[i].id, 
               tasks[i].description, 
               priorityToString(tasks[i].priority));
    }
    
    printf("\nEnter task ID to modify (or 0 to cancel): ");
    int id;
    scanf("%d", &id);
    
    if (id == 0) return;
    
    // Find the task
    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].id == id) {
            Task *t = &tasks[i];
            
            printf("\n=== Modifying Task ID: %d ===\n", t->id);
            printf("1. Description: %s\n", t->description);
            printf("2. Priority: %s\n", priorityToString(t->priority));
            printf("3. Duration: %d seconds\n", t->duration);
            printf("4. Status: %s\n", t->completed ? "Completed" : "Pending");
            printf("5. Save and return\n");
            
            int choice;
            do {
                printf("\nSelect what to modify (1-5): ");
                scanf("%d", &choice);
                
                switch (choice) {
                    case 1:
                        clearInputBuffer();
                        printf("New description: ");
                        fgets(t->description, MAX_DESCRIPTION, stdin);
                        t->description[strcspn(t->description, "\n")] = 0;
                        break;
                    case 2: {
                        int priorityChoice;
                        printf("Select new priority:\n");
                        printf("1. High\n");
                        printf("2. Medium\n");
                        printf("3. Low\n");
                        printf("Choice: ");
                        scanf("%d", &priorityChoice);
                        
                        switch (priorityChoice) {
                            case 1: t->priority = HIGH; break;
                            case 2: t->priority = MEDIUM; break;
                            case 3: t->priority = LOW; break;
                            default: printf("Invalid choice.\n");
                        }
                        break;
                    }
                    case 3:
                        do {
                            printf("New duration (1-3600 seconds): ");
                            scanf("%d", &t->duration);
                            
                            if (t->duration < 1 || t->duration > 3600) {
                                printf("Invalid duration.\n");
                            }
                        } while (t->duration < 1 || t->duration > 3600);
                        break;
                    case 4:
                        t->completed = !t->completed;
                        printf("Status changed to: %s\n", t->completed ? "Completed" : "Pending");
                        break;
                    case 5:
                        printf("Changes saved.\n");
                        saveTasksToFile();
                        return;
                    default:
                        printf("Invalid choice.\n");
                }
                
            } while (choice != 5);
            
            return;
        }
    }
    
    printf("Task with ID %d not found.\n", id);
}

// Sort tasks by priority and then duration
void sortTasks() {
    if (taskCount <= 1) {
        printf("\nNothing to sort.\n");
        return;
    }
    
    printf("\n=== Sort Tasks ===\n");
    printf("1. Sort by priority (highest first)\n");
    printf("2. Sort by duration (shortest first)\n");
    printf("3. Sort by creation time (newest first)\n");
    printf("Choice: ");
    
    int choice;
    scanf("%d", &choice);
    
    for (int i = 0; i < taskCount - 1; i++) {
        for (int j = i + 1; j < taskCount; j++) {
            bool shouldSwap = false;
            
            switch (choice) {
                case 1:
                    // By priority (then duration for ties)
                    shouldSwap = tasks[i].priority > tasks[j].priority || 
                                (tasks[i].priority == tasks[j].priority && 
                                 tasks[i].duration > tasks[j].duration);
                    break;
                case 2:
                    // By duration (then priority for ties)
                    shouldSwap = tasks[i].duration > tasks[j].duration || 
                                (tasks[i].duration == tasks[j].duration && 
                                 tasks[i].priority > tasks[j].priority);
                    break;
                case 3:
                    // By creation time (newer first)
                    shouldSwap = tasks[i].created < tasks[j].created;
                    break;
                default:
                    printf("Invalid choice. Nothing sorted.\n");
                    return;
            }
            
            if (shouldSwap) {
                Task temp = tasks[i];
                tasks[i] = tasks[j];
                tasks[j] = temp;
            }
        }
    }
    
    printf("Tasks sorted successfully.\n");
    viewTasks();
}

// Thread function to execute a single task
void* executeTaskThread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    Task* task = args->task;
    int* isRunning = args->isRunning;
    int taskIndex = args->taskIndex;
    
    printf("\n[Thread %d] Executing: %s (ID: %d) | Priority: %s | Duration: %d sec\n",
           taskIndex + 1, task->description, task->id, 
           priorityToString(task->priority), task->duration);
    
    // Countdown timer
    for (int j = task->duration; j > 0; j--) {
        if (!(*isRunning)) {
            printf("\n[Thread %d] Task execution cancelled.\n", taskIndex + 1);
            pthread_exit(NULL);
        }
        
        printf("\r[Thread %d] Time remaining: %d seconds...   ", taskIndex + 1, j);
        fflush(stdout);
        sleep(1);  // Simulate execution
    }
    
    task->completed = true;
    printf("\r[Thread %d] Task %d completed!                  \n", taskIndex + 1, task->id);
    
    free(args);
    pthread_exit(NULL);
}

// Execute all pending tasks simultaneously (or up to MAX_SIMULTANEOUS_TASKS)
void executeMultipleTasks() {
    if (taskCount == 0) {
        printf("\nNo tasks to execute.\n");
        return;
    }
    
    // Count pending tasks
    int pendingCount = 0;
    int pendingTaskIds[MAX_TASKS];
    
    printf("\n=== Pending Tasks ===\n");
    
    for (int i = 0; i < taskCount; i++) {
        if (!tasks[i].completed) {
            pendingTaskIds[pendingCount++] = tasks[i].id;
            printf("%d: %s (%s, %d sec)\n", 
                   tasks[i].id, 
                   tasks[i].description, 
                   priorityToString(tasks[i].priority),
                   tasks[i].duration);
        }
    }
    
    if (pendingCount == 0) {
        printf("No pending tasks to execute.\n");
        return;
    }
    
    // Task selection
    char taskSelection[MAX_TASKS * 4]; // Allow for IDs up to 999 with commas
    bool selectedTasks[MAX_TASKS] = {false};
    int numSelected = 0;
    
    printf("\nSelect tasks to execute (enter IDs separated by commas, or 'all' for all tasks): ");
    clearInputBuffer();
    fgets(taskSelection, sizeof(taskSelection), stdin);
    
    // Remove newline
    taskSelection[strcspn(taskSelection, "\n")] = 0;
    
    // Check if user wants all tasks
    if (strcmp(taskSelection, "all") == 0) {
        for (int i = 0; i < pendingCount; i++) {
            for (int j = 0; j < taskCount; j++) {
                if (tasks[j].id == pendingTaskIds[i] && !tasks[j].completed) {
                    selectedTasks[j] = true;
                    numSelected++;
                }
            }
        }
    } else {
        // Parse comma-separated IDs
        char *token = strtok(taskSelection, " ,");
        while (token != NULL) {
            int id = atoi(token);
            if (id > 0) {
                for (int i = 0; i < taskCount; i++) {
                    if (tasks[i].id == id && !tasks[i].completed) {
                        selectedTasks[i] = true;
                        numSelected++;
                        break;
                    }
                }
            }
            token = strtok(NULL, " ,");
        }
    }
    
    if (numSelected == 0) {
        printf("No valid tasks selected.\n");
        return;
    }
    
    // Create threads (up to MAX_SIMULTANEOUS_TASKS)
    int maxThreads = (numSelected < MAX_SIMULTANEOUS_TASKS) ? numSelected : MAX_SIMULTANEOUS_TASKS;
    pthread_t threads[MAX_SIMULTANEOUS_TASKS];
    int runningThreads = 0;
    int isRunning = 1; // Flag to control thread execution
    int tasksDone = 0;
    
    printf("\nExecuting %d tasks with up to %d running simultaneously.\n", numSelected, maxThreads);
    printf("Press Enter to start execution or Ctrl+C to cancel...");
    getchar();
    
    time_t startTime = time(NULL);
    
    // Initialize and create threads
    for (int i = 0; i < taskCount && runningThreads < maxThreads; i++) {
        if (selectedTasks[i]) {
            ThreadArgs *args = (ThreadArgs*)malloc(sizeof(ThreadArgs));
            if (args == NULL) {
                printf("Memory allocation error\n");
                return;
            }
            
            args->task = &tasks[i];
            args->isRunning = &isRunning;
            args->taskIndex = runningThreads;
            
            pthread_create(&threads[runningThreads], NULL, executeTaskThread, (void*)args);
            runningThreads++;
            tasksDone++;
        }
    }
    
    // Wait for threads to finish and start new ones as needed
    int nextTaskToRun = 0;
    while (tasksDone < numSelected) {
        // Find the next task to run
        for (int i = nextTaskToRun; i < taskCount; i++) {
            if (selectedTasks[i] && !tasks[i].completed) {
                nextTaskToRun = i + 1;
                break;
            }
        }
        
        // Wait for any thread to finish
        for (int i = 0; i < runningThreads; i++) {
            pthread_join(threads[i], NULL);
        }
        
        // Start new threads for remaining tasks
        runningThreads = 0;
        for (int i = nextTaskToRun; i < taskCount && runningThreads < maxThreads; i++) {
            if (selectedTasks[i] && !tasks[i].completed) {
                ThreadArgs *args = (ThreadArgs*)malloc(sizeof(ThreadArgs));
                if (args == NULL) {
                    printf("Memory allocation error\n");
                    return;
                }
                
                args->task = &tasks[i];
                args->isRunning = &isRunning;
                args->taskIndex = runningThreads;
                
                pthread_create(&threads[runningThreads], NULL, executeTaskThread, (void*)args);
                runningThreads++;
                tasksDone++;
                
                if (tasksDone >= numSelected) {
                    break;
                }
            }
        }
    }
    
    // Wait for any remaining threads
    for (int i = 0; i < runningThreads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    time_t endTime = time(NULL);
    printf("\n=== Execution Summary ===\n");
    printf("Tasks completed: %d\n", numSelected);
    printf("Total wall clock time: %ld seconds\n", (endTime - startTime));
    
    saveTasksToFile();
}

// Execute a specific task
void executeSpecificTask() {
    if (taskCount == 0) {
        printf("\nNo tasks available to execute.\n");
        return;
    }
    
    printf("\n=== Execute Specific Task ===\n");
    
    // Display only pending tasks
    printf("Pending tasks:\n");
    int pendingCount = 0;
    for (int i = 0; i < taskCount; i++) {
        if (!tasks[i].completed) {
            printf("%d: %s (%s, %d sec)\n", 
                   tasks[i].id, 
                   tasks[i].description, 
                   priorityToString(tasks[i].priority),
                   tasks[i].duration);
            pendingCount++;
        }
    }
    
    if (pendingCount == 0) {
        printf("No pending tasks to execute.\n");
        return;
    }
    
    printf("\nEnter task ID to execute (or 0 to cancel): ");
    int id;
    scanf("%d", &id);
    
    if (id == 0) return;
    
    // Find and execute the task
    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].id == id && !tasks[i].completed) {
            printf("\nExecuting: %s (ID: %d) | Priority: %s | Duration: %d sec\n",
                   tasks[i].description, tasks[i].id, 
                   priorityToString(tasks[i].priority), 
                   tasks[i].duration);
            
            printf("Press Enter to start execution...");
            clearInputBuffer();
            getchar();
            
            // Countdown timer
            for (int j = tasks[i].duration; j > 0; j--) {
                printf("\rTime remaining: %d seconds...   ", j);
                fflush(stdout);
                sleep(1);  // Simulate execution
            }
            
            tasks[i].completed = true;
            printf("\rTask %d completed!                  \n", tasks[i].id);
            saveTasksToFile();
            return;
        } else if (tasks[i].id == id && tasks[i].completed) {
            printf("Task %d is already marked as completed.\n", id);
            return;
        }
    }
    
    printf("Task with ID %d not found.\n", id);
}

// Execute all pending tasks in order
void executeTasks() {
    printf("\n=== Execute Tasks ===\n");
    printf("1. Execute all tasks in sequence\n");
    printf("2. Execute multiple tasks simultaneously\n");
    printf("3. Execute a specific task\n");
    printf("4. Return to main menu\n");
    printf("Choice: ");
    
    int choice;
    scanf("%d", &choice);
    
    switch (choice) {
        case 1: {
            if (taskCount == 0) {
                printf("\nNo tasks to execute.\n");
                return;
            }
            
            // Count pending tasks
            int pendingCount = 0;
            for (int i = 0; i < taskCount; i++) {
                if (!tasks[i].completed) {
                    pendingCount++;
                }
            }
            
            if (pendingCount == 0) {
                printf("\nNo pending tasks to execute.\n");
                return;
            }

            // Sort by priority first
            for (int i = 0; i < taskCount - 1; i++) {
                for (int j = i + 1; j < taskCount; j++) {
                    if (tasks[i].priority > tasks[j].priority || 
                       (tasks[i].priority == tasks[j].priority && tasks[i].duration > tasks[j].duration)) {
                        Task temp = tasks[i];
                        tasks[i] = tasks[j];
                        tasks[j] = temp;
                    }
                }
            }

            printf("\n=== Executing %d Pending Tasks in Sequence ===\n", pendingCount);
            printf("Tasks will be executed in priority order (highest first).\n");
            
            // Calculate total estimated time
            int totalTime = 0;
            for (int i = 0; i < taskCount; i++) {
                if (!tasks[i].completed) {
                    totalTime += tasks[i].duration;
                }
            }
            printf("Total estimated time: %d seconds\n\n", totalTime);
            
            printf("Press Enter to start execution or Ctrl+C to cancel...");
            clearInputBuffer();
            getchar();
            
            int executed = 0;
            time_t startTime = time(NULL);
            
            for (int i = 0; i < taskCount; i++) {
                if (!tasks[i].completed) {
                    printf("\n[%d/%d] Executing: %s (ID: %d) | Priority: %s | Duration: %d sec\n",
                           executed + 1, pendingCount, tasks[i].description, tasks[i].id, 
                           priorityToString(tasks[i].priority), tasks[i].duration);
                    
                    // Countdown timer
                    for (int j = tasks[i].duration; j > 0; j--) {
                        printf("\rTime remaining: %d seconds...   ", j);
                        fflush(stdout);
                        sleep(1);  // Simulate execution
                    }
                    
                    tasks[i].completed = true;
                    executed++;
                    printf("\rTask %d completed!                  \n", tasks[i].id);
                }
            }
            
            time_t endTime = time(NULL);
            printf("\n=== Execution Summary ===\n");
            printf("Tasks completed: %d\n", executed);
            printf("Total wall clock time: %ld seconds\n", (endTime - startTime));
            
            saveTasksToFile();
            break;
        }
        case 2:
            executeMultipleTasks();
            break;
        case 3:
            executeSpecificTask();
            break;
        case 4:
            return;
        default:
            printf("Invalid choice.\n");
    }
}

// Show the main menu and get user's choice
void showMenu() {
    printf("\n╔════════════════════════════════════╗\n");
    printf("║       TASK MANAGER SYSTEM          ║\n");
    printf("╠════════════════════════════════════╣\n");
    printf("║ 1. Add New Task                    ║\n");
    printf("║ 2. View All Tasks                  ║\n");
    printf("║ 3. Search Tasks                    ║\n");
    printf("║ 4. Delete Task                     ║\n");
    printf("║ 5. Modify Task                     ║\n");
    printf("║ 6. Sort Tasks                      ║\n");
    printf("║ 7. Execute Tasks                   ║\n");
    printf("║ 8. Exit                           ║\n");
    printf("╚════════════════════════════════════╝\n");
    printf("Enter your choice (1-8): ");
}

int main() {
    printf("Task Manager System\n");
    printf("===================\n");
    
    loadTasksFromFile();
    
    int choice;
    do {
        showMenu();
        scanf("%d", &choice);
        
        switch (choice) {
            case 1:
                addTask();
                break;
            case 2:
                viewTasks();
                break;
            case 3:
                searchTasks();
                break;
            case 4:
                deleteTask();
                break;
            case 5:
                modifyTask();
                break;
            case 6:
                sortTasks();
                break;
            case 7:
                executeTasks();
                break;
            case 8:
                printf("\nExiting Task Manager. Goodbye!\n");
                break;
            default:
                printf("\nInvalid choice. Please try again.\n");
        }
    } while (choice != 8);
    
    return 0;
}