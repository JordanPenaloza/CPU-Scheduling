#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QUEUE_SIZE 1000

typedef struct {
    int processNumber;
    int arrivalTime;
    int cpuBurst;
    int priority;
    int remainingBurst; // Used for RR and preemptive scheduling.
    int finishTime;
    int waitingTime;
    int completed;    // 0: not completed, 1: completed.
} Process;

// Function prototypes.
void resetProcesses(Process processes[], int numProcesses);
void simulateRoundRobin(Process processes[], int numProcesses, int timeQuantum, FILE *out);
void simulateSJF(Process processes[], int numProcesses, FILE *out);
void simulatePriorityNoPreemptive(Process processes[], int numProcesses, FILE *out);
void simulatePriorityPreemptive(Process processes[], int numProcesses, FILE *out);
int compareArrivalTime(const void *a, const void *b);

int main() {
    FILE *fp = fopen("input.txt", "r");
    if (fp == NULL) {
        printf("Error opening input.txt\n");
        return 1;
    }
    
    // Read the first line and remove any newline characters.
    char firstLine[100];
    if (fgets(firstLine, sizeof(firstLine), fp) == NULL) {
        printf("Error reading algorithm line\n");
        fclose(fp);
        return 1;
    }
    firstLine[strcspn(firstLine, "\r\n")] = '\0';
    
    // Use sscanf to extract the algorithm name and (if present) the time quantum.
    char algorithm[50];
    int timeQuantum = 0;
    if (sscanf(firstLine, "%s %d", algorithm, &timeQuantum) < 1) {
        printf("Error parsing the algorithm line\n");
        fclose(fp);
        return 1;
    }
    
    int numProcesses;
    if (fscanf(fp, "%d", &numProcesses) != 1) {
        printf("Error reading number of processes\n");
        fclose(fp);
        return 1;
    }
    
    Process *processes = (Process *)malloc(numProcesses * sizeof(Process));
    if (processes == NULL) {
        printf("Memory allocation failed\n");
        fclose(fp);
        return 1;
    }
    
    for (int i = 0; i < numProcesses; i++) {
        if (fscanf(fp, "%d %d %d %d", 
                   &processes[i].processNumber,
                   &processes[i].arrivalTime,
                   &processes[i].cpuBurst,
                   &processes[i].priority) != 4) {
            printf("Error reading process data for process %d\n", i+1);
            free(processes);
            fclose(fp);
            return 1;
        }
        processes[i].remainingBurst = processes[i].cpuBurst;
        processes[i].finishTime = 0;
        processes[i].waitingTime = 0;
        processes[i].completed = 0;
    }
    fclose(fp);
    
    // Sort processes by arrival time.
    qsort(processes, numProcesses, sizeof(Process), compareArrivalTime);
    
    FILE *out = fopen("output.txt", "w");
    if (out == NULL) {
        printf("Error opening output.txt for writing\n");
        free(processes);
        return 1;
    }
    
    // Reset simulation fields.
    resetProcesses(processes, numProcesses);
    
    // Run the chosen scheduling algorithm.
    if (strcmp(algorithm, "RR") == 0) {
        fprintf(out, "RR %d\n", timeQuantum);
        simulateRoundRobin(processes, numProcesses, timeQuantum, out);
    } else if (strcmp(algorithm, "SJF") == 0) {
        fprintf(out, "SJF\n");
        simulateSJF(processes, numProcesses, out);
    } else if (strcmp(algorithm, "PR_noPREMP") == 0) {
        fprintf(out, "PR_noPREMP\n");
        simulatePriorityNoPreemptive(processes, numProcesses, out);
    } else if (strcmp(algorithm, "PR_withPREMP") == 0) {
        fprintf(out, "PR_withPREMP\n");
        simulatePriorityPreemptive(processes, numProcesses, out);
    } else {
        fprintf(out, "%s\n", algorithm);
    }
    
    double totalWaitingTime = 0;
    for (int i = 0; i < numProcesses; i++) {
        int wt = processes[i].finishTime - processes[i].arrivalTime - processes[i].cpuBurst;
        totalWaitingTime += wt;
    }
    double avgWaitingTime = totalWaitingTime / numProcesses;
    fprintf(out, "AVG Waiting Time: %.2f\n", avgWaitingTime);
    
    fclose(out);
    free(processes);
    return 0;
}

int compareArrivalTime(const void *a, const void *b) {
    Process *p1 = (Process *)a;
    Process *p2 = (Process *)b;
    return p1->arrivalTime - p2->arrivalTime;
}

void resetProcesses(Process processes[], int numProcesses) {
    for (int i = 0; i < numProcesses; i++) {
        processes[i].remainingBurst = processes[i].cpuBurst;
        processes[i].finishTime = 0;
        processes[i].waitingTime = 0;
        processes[i].completed = 0;
    }
}

// -------------------- Simulation Functions --------------------

// Round Robin Simulation Function.
void simulateRoundRobin(Process processes[], int numProcesses, int timeQuantum, FILE *out) {
    int currentTime = 0, finishedCount = 0;
    int queue[QUEUE_SIZE], front = 0, rear = 0;
    int inQueue[1000] = {0};

    for (int i = 0; i < numProcesses; i++) {
        if (processes[i].arrivalTime <= currentTime && processes[i].remainingBurst > 0) {
            queue[rear] = i;
            rear = (rear + 1) % QUEUE_SIZE;
            inQueue[i] = 1;
        }
    }
    
    while (finishedCount < numProcesses) {
        if (front == rear) {
            int minArrival = 99999999;
            for (int i = 0; i < numProcesses; i++) {
                if (processes[i].remainingBurst > 0 && processes[i].arrivalTime < minArrival)
                    minArrival = processes[i].arrivalTime;
            }
            currentTime = (minArrival > currentTime) ? minArrival : currentTime+1;
            continue;
        }
        int i = queue[front];
        front = (front + 1) % QUEUE_SIZE;
        inQueue[i] = 0;
        fprintf(out, "%d\t%d\n", currentTime, processes[i].processNumber);
        int execTime = (processes[i].remainingBurst > timeQuantum) ? timeQuantum : processes[i].remainingBurst;
        processes[i].remainingBurst -= execTime;
        currentTime += execTime;
        for (int j = 0; j < numProcesses; j++) {
            if (processes[j].arrivalTime > (currentTime - execTime) &&
                processes[j].arrivalTime <= currentTime &&
                processes[j].remainingBurst > 0 && !inQueue[j]) {
                queue[rear] = j;
                rear = (rear + 1) % QUEUE_SIZE;
                inQueue[j] = 1;
            }
        }
        if (processes[i].remainingBurst > 0) {
            queue[rear] = i;
            rear = (rear + 1) % QUEUE_SIZE;
            inQueue[i] = 1;
        } else {
            processes[i].finishTime = currentTime;
            finishedCount++;
        }
    }
}

// Shortest Job First (Non-preemptive) Simulation Function.
void simulateSJF(Process processes[], int numProcesses, FILE *out) {
    int currentTime = 0, completedCount = 0;
    while (completedCount < numProcesses) {
        int idx = -1, minBurst = 99999999;
        for (int i = 0; i < numProcesses; i++) {
            if (!processes[i].completed && processes[i].arrivalTime <= currentTime) {
                if (processes[i].cpuBurst < minBurst) {
                    minBurst = processes[i].cpuBurst;
                    idx = i;
                } else if (processes[i].cpuBurst == minBurst && idx != -1) {
                    if (processes[i].arrivalTime < processes[idx].arrivalTime ||
                        (processes[i].arrivalTime == processes[idx].arrivalTime &&
                         processes[i].processNumber < processes[idx].processNumber))
                    {
                        idx = i;
                    }
                }
            }
        }
        if (idx == -1) {
            int minArrival = 99999999;
            for (int i = 0; i < numProcesses; i++) {
                if (!processes[i].completed && processes[i].arrivalTime < minArrival)
                    minArrival = processes[i].arrivalTime;
            }
            currentTime = (minArrival > currentTime) ? minArrival : currentTime+1;
            continue;
        }
        fprintf(out, "%d\t%d\n", currentTime, processes[idx].processNumber);
        currentTime += processes[idx].cpuBurst;
        processes[idx].finishTime = currentTime;
        processes[idx].completed = 1;
        completedCount++;
        processes[idx].waitingTime = processes[idx].finishTime - processes[idx].arrivalTime - processes[idx].cpuBurst;
    }
}

// Priority Scheduling without Preemption Simulation Function.
void simulatePriorityNoPreemptive(Process processes[], int numProcesses, FILE *out) {
    int currentTime = 0, completedCount = 0;
    while (completedCount < numProcesses) {
        int idx = -1, bestPriority = 99999999;
        for (int i = 0; i < numProcesses; i++) {
            if (processes[i].remainingBurst > 0 && processes[i].arrivalTime <= currentTime) {
                if (idx == -1 || processes[i].priority < bestPriority ||
                    (processes[i].priority == bestPriority && processes[i].processNumber < processes[idx].processNumber))
                {
                    bestPriority = processes[i].priority;
                    idx = i;
                }
            }
        }
        if (idx == -1) {
            int minArrival = 99999999;
            for (int i = 0; i < numProcesses; i++) {
                if (processes[i].remainingBurst > 0 && processes[i].arrivalTime < minArrival)
                    minArrival = processes[i].arrivalTime;
            }
            currentTime = (minArrival > currentTime) ? minArrival : currentTime+1;
            continue;
        }
        fprintf(out, "%d\t%d\n", currentTime, processes[idx].processNumber);
        currentTime += processes[idx].cpuBurst;
        processes[idx].finishTime = currentTime;
        processes[idx].remainingBurst = 0;
        processes[idx].completed = 1;
        completedCount++;
        processes[idx].waitingTime = processes[idx].finishTime - processes[idx].arrivalTime - processes[idx].cpuBurst;
    }
}

// Priority Scheduling with Preemption (PR_withPREMP) Simulation Function.
void simulatePriorityPreemptive(Process processes[], int numProcesses, FILE *out) {
    int currentTime = 0, completedCount = 0;
    int lastSelectedIndex = -1;
    while (completedCount < numProcesses) {
        int idx = -1, bestPriority = 99999999;
        for (int i = 0; i < numProcesses; i++) {
            if (!processes[i].completed && processes[i].arrivalTime <= currentTime && processes[i].remainingBurst > 0) {
                if (idx == -1 || processes[i].priority < bestPriority ||
                    (processes[i].priority == bestPriority && processes[i].processNumber < processes[idx].processNumber)) {
                    idx = i;
                    bestPriority = processes[i].priority;
                }
            }
        }
        if (idx == -1) {
            currentTime++; // Increment time if no process is ready.
            continue;
        }
        if (lastSelectedIndex != idx) {
            fprintf(out, "%d\t%d\n", currentTime, processes[idx].processNumber);
            lastSelectedIndex = idx;
        }
        processes[idx].remainingBurst--;
        currentTime++;
        if (processes[idx].remainingBurst == 0) {
            processes[idx].finishTime = currentTime;
            processes[idx].completed = 1;
            completedCount++;
            lastSelectedIndex = -1;
        }
    }
}
