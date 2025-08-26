/**
 * @file semDemo.c
 * @brief Demonstration of Semaphore Library Usage
 * @ingroup semLib
 */

#include "semLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_THREADS 5
#define NUM_ITERATIONS 3

// Global variables for demonstration
static int sharedCounter = 0;
static SEM_ID binarySem;
static SEM_ID countingSem;
static SEM_ID mutexSem;

/**
 * @brief Thread function demonstrating binary semaphore usage
 */
void* binarySemThread(void* arg) {
    int threadId = *(int*)arg;
    
    printf("Thread %d: Waiting for binary semaphore...\n", threadId);
    semTake(binarySem, -1); // Wait indefinitely
    
    printf("Thread %d: Acquired binary semaphore. Doing work...\n", threadId);
    sleep(1); // Simulate work
    printf("Thread %d: Work completed. Releasing binary semaphore.\n", threadId);
    
    semGive(binarySem);
    return NULL;
}

/**
 * @brief Thread function demonstrating counting semaphore usage
 */
void* countingSemThread(void* arg) {
    int threadId = *(int*)arg;
    
    printf("Thread %d: Waiting for counting semaphore...\n", threadId);
    semTake(countingSem, -1); // Wait indefinitely
    
    printf("Thread %d: Acquired counting semaphore. Doing work...\n", threadId);
    sleep(1); // Simulate work
    printf("Thread %d: Work completed. Releasing counting semaphore.\n", threadId);
    
    semGive(countingSem);
    return NULL;
}

/**
 * @brief Thread function demonstrating mutex semaphore usage
 */
void* mutexSemThread(void* arg) {
    int threadId = *(int*)arg;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        printf("Thread %d: Waiting for mutex (iteration %d)...\n", threadId, i+1);
        semTake(mutexSem, -1); // Wait indefinitely
        
        // Critical section
        int localCopy = sharedCounter;
        printf("Thread %d: Read shared counter = %d\n", threadId, localCopy);
        usleep(100000); // Sleep for 100ms to increase chance of race condition
        localCopy++;
        sharedCounter = localCopy;
        printf("Thread %d: Updated shared counter to %d\n", threadId, sharedCounter);
        
        semGive(mutexSem);
        printf("Thread %d: Released mutex\n", threadId);
        
        // Sleep a bit to let other threads run
        usleep(50000);
    }
    
    return NULL;
}

/**
 * @brief Thread function demonstrating timeout behavior
 */
void* timeoutThread(void* arg) {
    int threadId = *(int*)arg;
    
    printf("Thread %d: Attempting to take binary semaphore with 200ms timeout...\n", threadId);
    int result = semTake(binarySem, 20); // 20 ticks = 200ms (assuming 10ms/tick)
    
    if (result == OK) {
        printf("Thread %d: Acquired binary semaphore within timeout\n", threadId);
        sleep(1); // Simulate work
        semGive(binarySem);
    } else {
        printf("Thread %d: Failed to acquire binary semaphore within timeout\n", threadId);
    }
    
    return NULL;
}

/**
 * @brief Main function - demonstrates all semaphore types
 */
int main() 
{
    printf("Semaphore Library Demonstration\n");
    printf("===============================\n\n");
    
    pthread_t threads[NUM_THREADS];
    int threadIds[NUM_THREADS];
    
    // Create semaphores
    binarySem = semBCreate(SEM_Q_FIFO, 1); // Initially available
    countingSem = semCCreate(SEM_Q_FIFO, 2); // Allow 2 concurrent accesses
    mutexSem = semMCreate(SEM_Q_FIFO);
    
    if (!binarySem || !countingSem || !mutexSem) {
        printf("Failed to create semaphores!\n");
        return EXIT_FAILURE;
    }
    
    // Demonstration 1: Binary Semaphore
    printf("1. Binary Semaphore Demonstration\n");
    printf("---------------------------------\n");
    
    // Create threads for binary semaphore demo
    for (int i = 0; i < 2; i++) {
        threadIds[i] = i;
        pthread_create(&threads[i], NULL, binarySemThread, &threadIds[i]);
    }
    
    // Wait for binary semaphore threads
    for (int i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n2. Counting Semaphore Demonstration (Limit: 2 concurrent)\n");
    printf("---------------------------------------------------------\n");
    
    // Create threads for counting semaphore demo
    for (int i = 0; i < 4; i++) {
        threadIds[i] = i;
        pthread_create(&threads[i], NULL, countingSemThread, &threadIds[i]);
    }
    
    // Wait for counting semaphore threads
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n3. Mutex Semaphore Demonstration (Shared Counter)\n");
    printf("-------------------------------------------------\n");
    printf("Initial shared counter value: %d\n", sharedCounter);
    
    // Create threads for mutex demo
    for (int i = 0; i < NUM_THREADS; i++) {
        threadIds[i] = i;
        pthread_create(&threads[i], NULL, mutexSemThread, &threadIds[i]);
    }
    
    // Wait for mutex threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Final shared counter value: %d (Expected: %d)\n", 
           sharedCounter, NUM_THREADS * NUM_ITERATIONS);
    
    printf("\n4. Timeout Demonstration\n");
    printf("------------------------\n");
    
    // First take the binary semaphore to make it unavailable
    semTake(binarySem, -1);
    printf("Main thread: Acquired binary semaphore, making it unavailable\n");
    
    // Create a thread that will timeout waiting for the semaphore
    int timeoutThreadId = 0;
    pthread_t timeoutThreadHandle;
    pthread_create(&timeoutThreadHandle, NULL, timeoutThread, &timeoutThreadId);
    
    // Wait a bit, then release the semaphore
    sleep(1);
    printf("Main thread: Releasing binary semaphore\n");
    semGive(binarySem);
    
    // Wait for the timeout thread
    pthread_join(timeoutThreadHandle, NULL);
    
    // Clean up
    semDelete(binarySem);
    semDelete(countingSem);
    semDelete(mutexSem);
    
    printf("\nDemo completed successfully!\n");
    return EXIT_SUCCESS;
}