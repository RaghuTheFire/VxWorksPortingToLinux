/**
 * @file mboxLibDemo.cpp
 * @brief Demonstration application for the mailbox library
 * 
 * This demo shows how to use the mailbox library for inter-thread communication
 * with both blocking and non-blocking operations.
 */

#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include "mboxLib.h"

// Structure for our demo messages
struct DemoMessage {
    int id;
    char text[64];
    uint32_t timestamp;
};

// Global mailbox handle
MBOX_ID g_mbox = nullptr;

// Producer thread function
void producerThread(int threadId, int numMessages) {
    std::cout << "Producer " << threadId << " started, sending " << numMessages << " messages" << std::endl;
    
    for (int i = 0; i < numMessages; i++) {
        // Allocate and prepare a message
        DemoMessage* msg = new DemoMessage();
        msg->id = threadId * 1000 + i;
        msg->timestamp = static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count());
        snprintf(msg->text, sizeof(msg->text), "Message %d from producer %d", i, threadId);
        
        // Send with a 1-second timeout (1000 ticks assuming 1000 ticks/sec)
        int result = mboxSend(g_mbox, msg, 1000);
        
        if (result == 0) {
            std::cout << "Producer " << threadId << " sent message " << i << std::endl;
        } else {
            std::cerr << "Producer " << threadId << " failed to send message " << i << std::endl;
            delete msg;
        }
        
        // Small delay between messages
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "Producer " << threadId << " finished" << std::endl;
}

// Consumer thread function
void consumerThread(int threadId, int numMessages) {
    std::cout << "Consumer " << threadId << " started, expecting " << numMessages << " messages" << std::endl;
    
    int received = 0;
    while (received < numMessages) {
        void* msgPtr = nullptr;
        
        // Receive with a 2-second timeout
        int result = mboxReceive(g_mbox, &msgPtr, 2000);
        
        if (result == 0 && msgPtr != nullptr) {
            DemoMessage* msg = static_cast<DemoMessage*>(msgPtr);
            std::cout << "Consumer " << threadId << " received: [" << msg->id << "] " 
                      << msg->text << " (timestamp: " << msg->timestamp << ")" << std::endl;
            delete msg;
            received++;
        } else {
            std::cerr << "Consumer " << threadId << " timeout or error receiving message" << std::endl;
            // In a real application, you might want to break after several timeouts
        }
    }
    
    std::cout << "Consumer " << threadId << " finished" << std::endl;
}

// Demo for non-blocking operations
void nonBlockingDemo() {
    std::cout << "\n=== Non-blocking operation demo ===" << std::endl;
    
    // Try to receive with no wait (timeout = 0)
    void* msgPtr = nullptr;
    int result = mboxReceive(g_mbox, &msgPtr, 0);
    
    if (result == 0) {
        std::cout << "Unexpectedly received a message in non-blocking demo" << std::endl;
        delete static_cast<DemoMessage*>(msgPtr);
    } else {
        std::cout << "No message available (as expected) in non-blocking receive" << std::endl;
    }
    
    // Try to send with no wait
    DemoMessage* msg = new DemoMessage();
    msg->id = 999;
    strcpy(msg->text, "Non-blocking test message");
    msg->timestamp = 0;
    
    result = mboxSend(g_mbox, msg, 0);
    
    if (result == 0) {
        std::cout << "Non-blocking send succeeded" << std::endl;
    } else {
        std::cout << "Non-blocking send failed (mailbox full)" << std::endl;
        delete msg;
    }
}

int main() {
    std::cout << "Mailbox Library Demo Application" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Create a mailbox with capacity for 10 messages
    g_mbox = mboxCreate(10);
    if (!g_mbox) {
        std::cerr << "Failed to create mailbox!" << std::endl;
        return 1;
    }
    
    std::cout << "Created mailbox with capacity 10" << std::endl;
    
    // Create producer and consumer threads
    const int numProducers = 2;
    const int numConsumers = 2;
    const int messagesPerProducer = 5;
    
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    
    // Start producers
    for (int i = 0; i < numProducers; i++) {
        producers.emplace_back(producerThread, i, messagesPerProducer);
    }
    
    // Start consumers
    for (int i = 0; i < numConsumers; i++) {
        consumers.emplace_back(consumerThread, i, messagesPerProducer);
    }
    
    // Wait for all producers to finish
    for (auto& producer : producers) {
        producer.join();
    }
    
    // Wait for all consumers to finish
    for (auto& consumer : consumers) {
        consumer.join();
    }
    
    // Demonstrate non-blocking operations
    nonBlockingDemo();
    
    // Clean up
    mboxDelete(g_mbox);
    std::cout << "Mailbox deleted, demo completed" << std::endl;
    
    return 0;
}