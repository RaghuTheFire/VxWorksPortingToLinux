/**
 * @file msgQLib.h
 * @brief Message queue with FIFO or priority ordering, tick-based timeouts.
 */
#ifndef __INCmsgQLibh
#define __INCmsgQLibh

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* MSG_Q_ID;

enum {
    MSG_Q_FIFO = 0,
    MSG_Q_PRIORITY = 1
};

MSG_Q_ID msgQCreate(size_t maxMsgs, size_t maxMsgLen, int options);
int      msgQDelete(MSG_Q_ID id);
int      msgQSend(MSG_Q_ID id, const void* buf, size_t nbytes, int timeoutTicks, int priority /*0=high..N*/);
int      msgQReceive(MSG_Q_ID id, void* buf, size_t maxNBytes, int timeoutTicks, size_t* outLen /* may be null */);

#ifdef __cplusplus
}
#endif

#endif /* __INCmsgQLibh */
