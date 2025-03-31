#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int processNumber;
    int arrivalTime;
    int cpuBurst;
    int priority;
} Process;

int main() {
    // Open the input file.
    FILE *fp = fopen("input.txt", "r");
    if (fp == NULL) {
        printf("Error opening input.txt\n");
        return 1;
    }
    
    // Buffer to read the first line.
    char firstLine[100];
    if (fgets(firstLine, sizeof(firstLine), fp) == NULL) {
        printf("Error reading algorithm line\n");
        fclose(fp);
        return 1;
    }
    
    // Remove any newline characters.
    firstLine[strcspn(firstLine, "\r\n")] = 0;
    
    char algorithm[50];
    int timeQuantum = 0;
    
    // Check if the algorithm is Round Robin (RR) by comparing the first two characters.
    if (strncmp(firstLine, "RR", 2) == 0) {
        // Tokenize the string to extract "RR" and the time quantum.
        char *token = strtok(firstLine, " ");
        strcpy(algorithm, token);  // algorithm now holds "RR"
        token = strtok(NULL, " ");
        if (token != NULL) {
            timeQuantum = atoi(token);
        }
    } else {
        // For non-RR algorithms, just copy the algorithm name.
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
    
    // Read each process's details.
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
    }
    
    fclose(fp);
    
    // For testing: print the parsed input.
    printf("Algorithm: %s\n", algorithm);
    if (strcmp(algorithm, "RR") == 0) {
        printf("Time Quantum: %d\n", timeQuantum);
    }
    printf("Number of Processes: %d\n", numProcesses);
    
    for (int i = 0; i < numProcesses; i++) {
        printf("Process %d: Arrival = %d, CPU Burst = %d, Priority = %d\n",
               processes[i].processNumber,
               processes[i].arrivalTime,
               processes[i].cpuBurst,
               processes[i].priority);
    }
    
    free(processes);
    return 0;
}
