/**
 * @file mboxLib.h
 * @brief Simple mailbox API with tick-based timeouts.
 */
#ifndef __INCmboxLibh
#define __INCmboxLibh

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* MBOX_ID;

/* Create a mailbox with capacity maxMsgs and max message length */
MBOX_ID mboxCreate(size_t maxMsgs, size_t maxMsgLen);
/* Delete a mailbox. Safe: wakes waiters and waits for them to leave. */
int mboxDelete(MBOX_ID);
/* Send a message with timeout in ticks (0 poll, <0 forever). Returns 0 or -1 */
int mboxSend(MBOX_ID, const void* data, size_t len, int timeoutTicks);
/* Receive a message. Copies up to maxLen bytes into buf. Actual size returned via *outLen if non-null. */
int mboxReceive(MBOX_ID, void* buf, size_t maxLen, size_t* outLen, int timeoutTicks);

#ifdef __cplusplus
}
#endif

#endif /* __INCmboxLibh */
