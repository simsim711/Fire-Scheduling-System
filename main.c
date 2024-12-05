#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define NUM_RESOURCES 3  // Number of resource types (teams, vehicles, water)

// Define resource structure
typedef struct {
    char name[50];
    int total;      // Total resources available
    int allocated;  // Resources currently allocated
} Resource;

// Define fire event structure
typedef struct {
    int id;
    char location[50];
    int severity;  // 1 to 10 (1 = low, 10 = high severity)
    Resource required_resources[NUM_RESOURCES]; // Required resources for each fire
    int allocated[NUM_RESOURCES]; // Allocated resources for each fire
    int resolved;  // Flag to indicate if the fire event is resolved
} FireEvent;

// Global arrays for resources
Resource available[NUM_RESOURCES]; // Global available resources

FireEvent *fireEvents; // Pointer to dynamically allocated fire events

// Function to get validated input
int getValidatedInput(const char *prompt, int min, int max) {
    int value;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &value) == 1 && value >= min && value <= max) {
            break;
        } else {
            printf("Invalid input! Please enter a number between %d and %d.\n", min, max);
            while (getchar() != '\n'); // Clear invalid input
        }
    }
    return value;
}

// Function to initialize resources
void initializeResources() {
    strcpy(available[0].name, "Teams");
    available[0].total = 15;

    strcpy(available[1].name, "Vehicles");
    available[1].total = 10;

    strcpy(available[2].name, "Water");
    available[2].total = 5000;
}

// Function to check if the system is in a safe state using the Banker's Algorithm
int isSafeState(Resource available[], int allocated[][NUM_RESOURCES], int max[][NUM_RESOURCES], int numProcesses) {
    int work[NUM_RESOURCES];
    int finish[numProcesses];

    for (int i = 0; i < NUM_RESOURCES; i++) {
        work[i] = available[i].total;
    }
    for (int i = 0; i < numProcesses; i++) {
        finish[i] = 0;
    }

    int progressMade = 1;
    while (progressMade) {
        progressMade = 0;
        for (int i = 0; i < numProcesses; i++) {
            if (!finish[i]) {
                int canAllocate = 1;
                for (int j = 0; j < NUM_RESOURCES; j++) {
                    if (max[i][j] - allocated[i][j] > work[j]) {
                        canAllocate = 0;
                        break;
                    }
                }

                if (canAllocate) {
                    for (int j = 0; j < NUM_RESOURCES; j++) {
                        work[j] += allocated[i][j];
                    }
                    finish[i] = 1;
                    progressMade = 1;
                }
            }
        }
    }

    for (int i = 0; i < numProcesses; i++) {
        if (!finish[i]) {
            return 0;
        }
    }
    return 1;
}

int allocateResources(FireEvent *fire, Resource available[], int allocated[][NUM_RESOURCES], int max[][NUM_RESOURCES], int numProcesses) {
    for (int i = 0; i < NUM_RESOURCES; i++) {
        if (fire->required_resources[i].total > available[i].total) {
            printf("Not enough resources available for Fire ID %d!\n", fire->id);
            return 0;
        }
    }

    for (int i = 0; i < NUM_RESOURCES; i++) {
        allocated[fire->id][i] = fire->required_resources[i].total;
        available[i].total -= fire->required_resources[i].total;
    }

    if (!isSafeState(available, allocated, max, numProcesses)) {
        for (int i = 0; i < NUM_RESOURCES; i++) {
            allocated[fire->id][i] -= fire->required_resources[i].total;
            available[i].total += fire->required_resources[i].total;
        }
        printf("Deadlock detected, resources not allocated for Fire ID %d\n", fire->id);
        return 0;
    }

    return 1;
}

void processScheduling(FireEvent *fireEvents, int numEvents) {
    for (int i = 0; i < numEvents - 1; i++) {
        for (int j = i + 1; j < numEvents; j++) {
            if (fireEvents[i].severity < fireEvents[j].severity) {
                FireEvent temp = fireEvents[i];
                fireEvents[i] = fireEvents[j];
                fireEvents[j] = temp;
            }
        }
    }
}

// Function to log resource usage and input values
void logResourceUsage(FireEvent *fire, int max[][NUM_RESOURCES]) {
    FILE *logFile = fopen("resource_log.txt", "a");
    if (logFile == NULL) {
        printf("Error opening file for logging!\n");
        return;
    }

    fprintf(logFile, "Fire ID: %d, Location: %s, Severity: %d\n", fire->id, fire->location, fire->severity);
    fprintf(logFile, "Required Teams: %d, Required Vehicles: %d, Required Water: %d\n",
            fire->required_resources[0].total, fire->required_resources[1].total, fire->required_resources[2].total);
    fprintf(logFile, "Max Teams: %d, Max Vehicles: %d, Max Water: %d\n",
            max[fire->id][0], max[fire->id][1], max[fire->id][2]);
    fprintf(logFile, "Allocated Teams: %d, Allocated Vehicles: %d, Allocated Water: %d\n",
            fire->required_resources[0].total, fire->required_resources[1].total, fire->required_resources[2].total);
    fprintf(logFile, "---------------------------------------------\n");

    fclose(logFile);
}

void releaseResources(FireEvent *fire, Resource available[], int allocated[][NUM_RESOURCES]) {
    for (int i = 0; i < NUM_RESOURCES; i++) {
        available[i].total += fire->allocated[i];
    }

    memset(fire->allocated, 0, sizeof(fire->allocated));
}

int main() {
    int numEvents;

    printf("Enter the number of fire events: ");
    numEvents = getValidatedInput("", 1, 10);

    fireEvents = (FireEvent *)malloc(numEvents * sizeof(FireEvent));

    initializeResources();

    for (int i = 0; i < numEvents; i++) {
        fireEvents[i].id = i;
        printf("Enter details for Fire ID %d\n", i);
        printf("Location: ");
        scanf("%s", fireEvents[i].location);
        fireEvents[i].severity = getValidatedInput("Severity (1-10): ", 1, 10);

        fireEvents[i].required_resources[0].total = getValidatedInput("Enter required Teams: ", 0, 15);
        fireEvents[i].required_resources[1].total = getValidatedInput("Enter required Vehicles: ", 0, 10);
        fireEvents[i].required_resources[2].total = getValidatedInput("Enter required Water: ", 0, 5000);
    }

    processScheduling(fireEvents, numEvents);

    int max[numEvents][NUM_RESOURCES];
    int allocated[numEvents][NUM_RESOURCES];
    for (int i = 0; i < numEvents; i++) {
        memset(allocated[i], 0, sizeof(allocated[i]));
        for (int j = 0; j < NUM_RESOURCES; j++) {
            char *resourceType = (j == 0) ? "Teams" : (j == 1) ? "Vehicles" : "Water";
            char prompt[50];
            sprintf(prompt, "Enter max %s for Fire ID %d: ", resourceType, i);
            max[i][j] = getValidatedInput(prompt, fireEvents[i].required_resources[j].total, INT_MAX);
        }
    }

    for (int i = 0; i < numEvents; i++) {
        if (allocateResources(&fireEvents[i], available, allocated, max, numEvents)) {
            fireEvents[i].resolved = 1;
            printf("Resources successfully allocated for Fire ID %d at %s.\n", fireEvents[i].id, fireEvents[i].location);
            logResourceUsage(&fireEvents[i], max);
            releaseResources(&fireEvents[i], available, allocated);
        }
    }

    free(fireEvents);

    return 0;
}
