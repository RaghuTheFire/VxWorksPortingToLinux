/**
 * @file msgQLibDemo.cpp
 * @brief Demo application for the message queue library
 * @details This application demonstrates the usage of the message queue library
 * with both FIFO and priority modes, showing how to create, send, receive, and
 * delete message queues.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "msgQLib.h"

/* Custom tick provider for the demo */
int vxTicksPerSecondGet(void) {
    return 100; // 100 ticks per second
}

/* Message structure for the demo */
typedef struct {
    int id;
    char text[32];
    int priority;
} demo_message_t;

/* Global queue handles for the demo */
MSG_Q_ID fifo_queue = NULL;
MSG_Q_ID priority_queue = NULL;

/**
 * @brief Producer thread function for FIFO queue
 * @param arg Thread argument (unused)
 * @return NULL
 * @details Produces messages for the FIFO queue demo
 */
void* fifo_producer(void* arg) {
    (void)arg;
    demo_message_t messages[] = {
        {1, "First message", 0},
        {2, "Second message", 0},
        {3, "Third message", 0},
        {4, "Fourth message", 0},
        {5, "Fifth message", 0}
    };
    
    printf("FIFO Producer: Starting to send messages\n");
    
    for (int i = 0; i < 5; i++) {
        STATUS status = msgQSend(fifo_queue, &messages[i], sizeof(demo_message_t), 100, 0);
        if (status == OK) {
            printf("FIFO Producer: Sent message %d - '%s'\n", messages[i].id, messages[i].text);
        } else {
            printf("FIFO Producer: Failed to send message %d\n", messages[i].id);
        }
        usleep(100000); // 100ms delay between messages
    }
    
    printf("FIFO Producer: Finished sending messages\n");
    return NULL;
}

/**
 * @brief Consumer thread function for FIFO queue
 * @param arg Thread argument (unused)
 * @return NULL
 * @details Consumes messages from the FIFO queue demo
 */
void* fifo_consumer(void* arg) {
    (void)arg;
    demo_message_t received;
    int count = 0;
    
    printf("FIFO Consumer: Starting to receive messages\n");
    
    while (count < 5) {
        int bytes = msgQReceive(fifo_queue, &received, sizeof(demo_message_t), 100);
        if (bytes > 0) {
            printf("FIFO Consumer: Received message %d - '%s'\n", received.id, received.text);
            count++;
        } else if (bytes == 0) {
            printf("FIFO Consumer: Received empty message\n");
            count++;
        } else {
            printf("FIFO Consumer: Timeout waiting for message\n");
        }
    }
    
    printf("FIFO Consumer: Finished receiving messages\n");
    return NULL;
}

/**
 * @brief Producer thread function for priority queue
 * @param arg Thread argument (unused)
 * @return NULL
 * @details Produces messages for the priority queue demo with different priorities
 */
void* priority_producer(void* arg) {
    (void)arg;
    demo_message_t messages[] = {
        {1, "Low priority message", 10},    // Lower priority
        {2, "High priority message", 100},  // Higher priority
        {3, "Medium priority message", 50}, // Medium priority
        {4, "Urgent message", 200},         // Very high priority
        {5, "Normal message", 30}           // Normal priority
    };
    
    printf("Priority Producer: Starting to send messages\n");
    
    for (int i = 0; i < 5; i++) {
        STATUS status = msgQSend(priority_queue, &messages[i], sizeof(demo_message_t), 100, messages[i].priority);
        if (status == OK) {
            printf("Priority Producer: Sent message %d - '%s' (priority: %d)\n", 
                   messages[i].id, messages[i].text, messages[i].priority);
        } else {
            printf("Priority Producer: Failed to send message %d\n", messages[i].id);
        }
        usleep(100000); // 100ms delay between messages
    }
    
    printf("Priority Producer: Finished sending messages\n");
    return NULL;
}

/**
 * @brief Consumer thread function for priority queue
 * @param arg Thread argument (unused)
 * @return NULL
 * @details Consumes messages from the priority queue demo, showing priority ordering
 */
void* priority_consumer(void* arg) {
    (void)arg;
    demo_message_t received;
    int count = 0;
    
    printf("Priority Consumer: Starting to receive messages\n");
    
    while (count < 5) {
        int bytes = msgQReceive(priority_queue, &received, sizeof(demo_message_t), 100);
        if (bytes > 0) {
            printf("Priority Consumer: Received message %d - '%s' (priority: %d)\n", 
                   received.id, received.text, received.priority);
            count++;
        } else if (bytes == 0) {
            printf("Priority Consumer: Received empty message\n");
            count++;
        } else {
            printf("Priority Consumer: Timeout waiting for message\n");
        }
    }
    
    printf("Priority Consumer: Finished receiving messages\n");
    return NULL;
}

/**
 * @brief Demo function for FIFO queue operations
 * @return 0 on success, -1 on failure
 * @details Demonstrates FIFO message queue operations
 */
int demo_fifo_queue(void) {
    printf("\n=== FIFO Queue Demo ===\n");
    
    // Create FIFO queue
    fifo_queue = msgQCreate(10, sizeof(demo_message_t), MSG_Q_FIFO);
    if (fifo_queue == NULL) {
        printf("Failed to create FIFO queue\n");
        return -1;
    }
    printf("Created FIFO queue with capacity 10 messages\n");
    
    pthread_t producer_thread, consumer_thread;
    
    // Create producer and consumer threads
    if (pthread_create(&producer_thread, NULL, fifo_producer, NULL) != 0) {
        printf("Failed to create producer thread\n");
        msgQDelete(fifo_queue);
        return -1;
    }
    
    if (pthread_create(&consumer_thread, NULL, fifo_consumer, NULL) != 0) {
        printf("Failed to create consumer thread\n");
        pthread_cancel(producer_thread);
        msgQDelete(fifo_queue);
        return -1;
    }
    
    // Wait for threads to complete
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    
    // Delete the queue
    if (msgQDelete(fifo_queue) == OK) {
        printf("FIFO queue deleted successfully\n");
    } else {
        printf("Failed to delete FIFO queue\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Demo function for priority queue operations
 * @return 0 on success, -1 on failure
 * @details Demonstrates priority message queue operations
 */
int demo_priority_queue(void) {
    printf("\n=== Priority Queue Demo ===\n");
    
    // Create priority queue
    priority_queue = msgQCreate(10, sizeof(demo_message_t), MSG_Q_PRIORITY);
    if (priority_queue == NULL) {
        printf("Failed to create priority queue\n");
        return -1;
    }
    printf("Created priority queue with capacity 10 messages\n");
    
    pthread_t producer_thread, consumer_thread;
    
    // Create producer and consumer threads
    if (pthread_create(&producer_thread, NULL, priority_producer, NULL) != 0) {
        printf("Failed to create producer thread\n");
        msgQDelete(priority_queue);
        return -1;
    }
    
    if (pthread_create(&consumer_thread, NULL, priority_consumer, NULL) != 0) {
        printf("Failed to create consumer thread\n");
        pthread_cancel(producer_thread);
        msgQDelete(priority_queue);
        return -1;
    }
    
    // Wait for threads to complete
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    
    // Delete the queue
    if (msgQDelete(priority_queue) == OK) {
        printf("Priority queue deleted successfully\n");
    } else {
        printf("Failed to delete priority queue\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Demo function for timeout operations
 * @return 0 on success, -1 on failure
 * @details Demonstrates timeout behavior in message queues
 */
int demo_timeout_behavior(void) {
    printf("\n=== Timeout Behavior Demo ===\n");
    
    // Create a small queue
    MSG_Q_ID timeout_queue = msgQCreate(2, sizeof(demo_message_t), MSG_Q_FIFO);
    if (timeout_queue == NULL) {
        printf("Failed to create timeout demo queue\n");
        return -1;
    }
    
    demo_message_t msg = {1, "Test message", 0};
    
    // Fill the queue
    printf("Filling the queue...\n");
    for (int i = 0; i < 2; i++) {
        if (msgQSend(timeout_queue, &msg, sizeof(demo_message_t), 100, 0) == OK) {
            printf("Sent message %d to fill the queue\n", i + 1);
        }
    }
    
    // Try to send with timeout (should fail)
    printf("Trying to send to a full queue with 500ms timeout...\n");
    STATUS status = msgQSend(timeout_queue, &msg, sizeof(demo_message_t), 50, 0);
    if (status == ERROR) {
        printf("Correctly timed out when trying to send to a full queue\n");
    } else {
        printf("Unexpectedly succeeded in sending to a full queue\n");
    }
    
    // Try to receive with timeout from an empty queue
    MSG_Q_ID empty_queue = msgQCreate(2, sizeof(demo_message_t), MSG_Q_FIFO);
    printf("Trying to receive from an empty queue with 500ms timeout...\n");
    int bytes = msgQReceive(empty_queue, &msg, sizeof(demo_message_t), 50);
    if (bytes == -1) {
        printf("Correctly timed out when trying to receive from an empty queue\n");
    } else {
        printf("Unexpectedly received from an empty queue\n");
    }
    
    // Clean up
    msgQDelete(timeout_queue);
    msgQDelete(empty_queue);
    
    return 0;
}

/**
 * @brief Main function
 * @return 0 on success, -1 on failure
 * @details Runs all demo functions
 */
int main(void) {
    printf("Message Queue Library Demo Application\n");
    printf("======================================\n");
    
    // Run FIFO queue demo
    if (demo_fifo_queue() != 0) {
        printf("FIFO queue demo failed\n");
        return -1;
    }
    
    // Run priority queue demo
    if (demo_priority_queue() != 0) {
        printf("Priority queue demo failed\n");
        return -1;
    }
    
    // Run timeout behavior demo
    if (demo_timeout_behavior() != 0) {
        printf("Timeout behavior demo failed\n");
        return -1;
    }
    
    printf("\nAll demos completed successfully!\n");
    return 0;
}