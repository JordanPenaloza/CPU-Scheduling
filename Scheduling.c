#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QUEUE_SIZE 1000

typedef struct {
    int processNumber;
    int arrivalTime;
    int cpuBurst;
    int priority;
    int remainingBurst;
    int finishTime;
    int waitingTime;
    int completed;   // used for SJF
} Process;

// Function prototypes
void simulateRoundRobin(Process processes[], int numProcesses, int timeQuantum, FILE *out);
void simulateSJF(Process processes[], int numProcesses, FILE *out);

int main() {
    // Open the input file.
    FILE *fp = fopen("input.txt", "r");
    if (fp == NULL) {
        printf("Error opening input.txt\n");
        return 1;
    }
    
    // Read the first line (algorithm and possibly time quantum).
    char firstLine[100];
    if (fgets(firstLine, sizeof(firstLine), fp) == NULL) {
        printf("Error reading algorithm line\n");
        fclose(fp);
        return 1;
    }
    firstLine[strcspn(firstLine, "\r\n")] = 0;
    
    char algorithm[50];
    int timeQuantum = 0;
    
    // Check if the algorithm is RR (Round Robin).
    if (strncmp(firstLine, "RR", 2) == 0) {
        char *token = strtok(firstLine, " ");
        strcpy(algorithm, token);  // algorithm now holds "RR"
        token = strtok(NULL, " ");
        if (token != NULL) {
            timeQuantum = atoi(token);
        }
    } else {
        strcpy(algorithm, firstLine);
    }
    
    // Read the number of processes.
    int numProcesses;
    if (fscanf(fp, "%d", &numProcesses) != 1) {
        printf("Error reading number of processes\n");
        fclose(fp);
        return 1;
    }
    
    // Allocate memory for the processes.
    Process *processes = (Process *)malloc(numProcesses * sizeof(Process));
    if (processes == NULL) {
        printf("Memory allocation failed\n");
        fclose(fp);
        return 1;
    }
    
    // Read each process's details and initialize simulation-specific fields.
    for (int i = 0; i < numProcesses; i++) {
        if (fscanf(fp, "%d %d %d %d", 
                   &processes[i].processNumber,
                   &processes[i].arrivalTime,
                   &processes[i].cpuBurst,
                   &processes[i].priority) != 4) {
            printf("Error reading process data for process %d\n", i + 1);
            free(processes);
            fclose(fp);
            return 1;
        }
        processes[i].remainingBurst = processes[i].cpuBurst;
        processes[i].finishTime = 0;
        processes[i].waitingTime = 0;
        processes[i].completed = 0;   // IMPORTANT: Initialize completed flag for SJF.
    }
    fclose(fp);
    
    // Open output file for writing.
    FILE *out = fopen("output.txt", "w");
    if (out == NULL) {
        printf("Error opening output.txt for writing\n");
        free(processes);
        return 1;
    }
    
    // Check which algorithm to simulate.
    if (strcmp(algorithm, "RR") == 0) {
        fprintf(out, "RR %d\n", timeQuantum);
        simulateRoundRobin(processes, numProcesses, timeQuantum, out);
    } else if (strcmp(algorithm, "SJF") == 0) {
        fprintf(out, "SJF\n");
        simulateSJF(processes, numProcesses, out);
    } else {
        fprintf(out, "%s\n", algorithm);
        // Other algorithms can be added similarly.
    }
    
    // Compute and write the average waiting time.
    double totalWaitingTime = 0;
    for (int i = 0; i < numProcesses; i++) {
        // Waiting time = Finish time - Arrival time - CPU burst.
        int waitingTime = processes[i].finishTime - processes[i].arrivalTime - processes[i].cpuBurst;
        totalWaitingTime += waitingTime;
    }
    double avgWaitingTime = totalWaitingTime / numProcesses;
    fprintf(out, "AVG Waiting Time: %.2f\n", avgWaitingTime);
    
    fclose(out);
    free(processes);
    return 0;
}

// Round Robin Simulation Function.
void simulateRoundRobin(Process processes[], int numProcesses, int timeQuantum, FILE *out) {
    int currentTime = 0;
    int finishedCount = 0;
    int queue[QUEUE_SIZE];
    int front = 0, rear = 0;
    int inQueue[1000] = {0};  // Flags: 0 = not in queue, 1 = in queue
    
    // Enqueue processes that have arrived at time 0.
    for (int i = 0; i < numProcesses; i++) {
        if (processes[i].arrivalTime <= currentTime && processes[i].remainingBurst > 0) {
            queue[rear] = i;
            rear = (rear + 1) % QUEUE_SIZE;
            inQueue[i] = 1;
        }
    }
    
    // Simulation loop.
    while (finishedCount < numProcesses) {
        // If the queue is empty, increment time until a process arrives.
        if (front == rear) {
            currentTime++;
            for (int i = 0; i < numProcesses; i++) {
                if (processes[i].arrivalTime == currentTime && processes[i].remainingBurst > 0 && !inQueue[i]) {
                    queue[rear] = i;
                    rear = (rear + 1) % QUEUE_SIZE;
                    inQueue[i] = 1;
                }
            }
            continue;
        }
        
        // Dequeue the next process.
        int i = queue[front];
        front = (front + 1) % QUEUE_SIZE;
        inQueue[i] = 0;
        
        // Log the scheduling event (Gantt chart format).
        fprintf(out, "%d\t%d\n", currentTime, processes[i].processNumber);
        
        // Determine execution time.
        int execTime = (processes[i].remainingBurst > timeQuantum) ? timeQuantum : processes[i].remainingBurst;
        processes[i].remainingBurst -= execTime;
        currentTime += execTime;
        
        // Check for any new arrivals during this time slice.
        for (int j = 0; j < numProcesses; j++) {
            if (processes[j].arrivalTime > (currentTime - execTime) &&
                processes[j].arrivalTime <= currentTime &&
                processes[j].remainingBurst > 0 &&
                !inQueue[j]) {
                queue[rear] = j;
                rear = (rear + 1) % QUEUE_SIZE;
                inQueue[j] = 1;
            }
        }
        
        // If the process is not finished, re-enqueue it.
        if (processes[i].remainingBurst > 0) {
            queue[rear] = i;
            rear = (rear + 1) % QUEUE_SIZE;
            inQueue[i] = 1;
        } else {
            // Process finished; record its finish time.
            processes[i].finishTime = currentTime;
            finishedCount++;
        }
    }
}

// Shortest Job First (SJF) Simulation Function (Non-preemptive)
void simulateSJF(Process processes[], int numProcesses, FILE *out) {
    int currentTime = 0;
    int completedCount = 0;
    
    // Loop until all processes are completed.
    while (completedCount < numProcesses) {
        int idx = -1;
        int minBurst = 1e9;  // a large number
        
        // Find the process with the smallest burst among those that have arrived and are not completed.
        for (int i = 0; i < numProcesses; i++) {
            if (!processes[i].completed && processes[i].arrivalTime <= currentTime) {
                if (processes[i].cpuBurst < minBurst) {
                    minBurst = processes[i].cpuBurst;
                    idx = i;
                } else if (processes[i].cpuBurst == minBurst && idx != -1) {
                    // Tie-breaker: earlier arrival time.
                    if (processes[i].arrivalTime < processes[idx].arrivalTime) {
                        idx = i;
                    } else if (processes[i].arrivalTime == processes[idx].arrivalTime) {
                        // Tie-breaker: lower process number.
                        if (processes[i].processNumber < processes[idx].processNumber) {
                            idx = i;
                        }
                    }
                }
            }
        }
        
        // If no process is available, increment time.
        if (idx == -1) {
            currentTime++;
            continue;
        }
        
        // Log the scheduling event.
        fprintf(out, "%d\t%d\n", currentTime, processes[idx].processNumber);
        
        // Run the selected process to completion.
        currentTime += processes[idx].cpuBurst;
        processes[idx].finishTime = currentTime;
        processes[idx].completed = 1;
        completedCount++;
        
        // Calculate waiting time for this process.
        processes[idx].waitingTime = processes[idx].finishTime - processes[idx].arrivalTime - processes[idx].cpuBurst;
    }
}
