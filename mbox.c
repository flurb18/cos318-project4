/* mailbox.c
 * COS 318, Fall 2019: Project 4 IPC and Process Management
 * Mailbox implementation
 */

#include "common.h"
#include "interrupt.h"
#include "mbox.h"
#include "sync.h"
#include "util.h"

typedef struct {
  // TODO: Fill this in
  char mbytes[MAX_MESSAGE_LENGTH];
} Message;

typedef struct {
  // The name of this message box
  char name[MBOX_NAME_LENGTH];
  // How many processes have this mbox open
  int usage_count;
  // The bounded buffer of messages
  Message buffer[MAX_MBOX_LENGTH];
  // The first and last elements of the queue in the buffer (might wrap around)
  int first;
  int last;
  /* Semaphores to block onto - s_msgfree counts the number of
     available (unused) message slots in the buffer for sending
     messages, while s_msginuse counts the number of available
     (used) slots in the buffer for recieving messages */
  semaphore_t s_buffree;
  semaphore_t s_bufinuse;
  /* Lock for critical sections */
  lock_t l;
} MessageBox;

static MessageBox MessageBoxen[MAX_MBOXEN];

// Perform any system-startup initialization for the message boxes
void init_mbox(void) {
  (void) MessageBoxen;
  // TODO: Fill this in
  int i;
  for (i = 0; i < MAX_MBOXEN; i++) {
    semaphore_init(&MessageBoxen[i].s_buffree, MAX_MBOX_LENGTH);
    semaphore_init(&MessageBoxen[i].s_bufinuse, 0);
    lock_init(&MessageBoxen[i].l);
  }
}

// Opens the mailbox named 'name', or creates a new message box if it doesn't
// already exist. A message box is a bounded buffer which holds up to
// MAX_MBOX_LENGTH items. If it fails because the message box table is full, it
// will return -1. Otherwise, it returns a message box id
mbox_t do_mbox_open(const char *name) {
  enter_critical();
  mbox_t result = do_mbox_open_helper(name);
  leave_critical();
  return result;
}

mbox_t do_mbox_open_helper(const char *name) {
  ASSERT(disable_count);
  int i, len;
  for (i = 0; i < MAX_MBOXEN; i++) {
    if (same_string(name, MessageBoxen[i].name) && \
        MessageBoxen[i].usage_count > 0) {
      MessageBoxen[i].usage_count++;
      return i;
    }
  }
  for (i = 0; i < MAX_MBOXEN; i++) {
    if (MessageBoxen[i].usage_count == 0) {
      MessageBoxen[i].first = 0;
      MessageBoxen[i].last = 0;
      MessageBoxen[i].usage_count = 1;
      len = strlen((char *)name);
      if (len > MBOX_NAME_LENGTH)
        len = MBOX_NAME_LENGTH;
      bcopy((char *)name, MessageBoxen[i].name, len);
      return i;
    }
  }
  return -1;
}

// Closes a message box
void do_mbox_close(mbox_t mbox) {
  (void) mbox;
  // TODO: Fill this in
  enter_critical();
  MessageBoxen[mbox].usage_count--;
  leave_critical();
}

// Determine if the given message box is full. Equivalently, determine if sending
// to this message box would cause a process to block
int do_mbox_is_full(mbox_t mbox) {
  (void) mbox;
  // TODO: Fill this in
  enter_critical();
  int f = MessageBoxen[mbox].first;
  int l = MessageBoxen[mbox].last;
  int result = ((f - 1 == l) || ((f == 0) && (l == MAX_MBOX_LENGTH - 1)));
  leave_critical();
  return result;
}

// Enqueues a message onto a message box. If the message box is full, the process
// will block until it can add the item. You may assume that the message box ID
// has been properly opened before this call. The message is 'nbytes' bytes
// starting at 'msg'
void do_mbox_send(mbox_t mbox, void *msg, int nbytes) {
  (void) mbox;
  (void) msg;
  (void) nbytes;
  // TODO: Fill this in
  ASSERT(!disable_count);
  MessageBox *m = &MessageBoxen[mbox];
  semaphore_down(&m->s_buffree);
  lock_acquire(&m->l);
  bcopy((char *)msg, (char *)&m->buffer[m->last].mbytes, nbytes);
  m->last = (m->last + 1) % MAX_MBOX_LENGTH;
  lock_release(&m->l);
  semaphore_up(&m->s_bufinuse);
}

// Receives a message from the specified message box. If empty, the process will
// block until it can remove an item. You may assume that the message box has
// been properly opened before this call. The message is copied into 'msg'. No
// more than 'nbytes' bytes will be copied into this buffer; longer messages
// will be truncated
void do_mbox_recv(mbox_t mbox, void *msg, int nbytes) {
  (void) mbox;
  (void) msg;
  (void) nbytes;
  // TODO: Fill this in
  MessageBox *m = &MessageBoxen[mbox];
  semaphore_down(&m->s_bufinuse);
  lock_acquire(&m->l);
  bcopy((char *)&m->buffer[m->first].mbytes, (char *)msg, nbytes);
  m->first = (m->first + 1) % MAX_MBOX_LENGTH;
  lock_release(&m->l);
  semaphore_up(&m->s_buffree);
}

// Returns the number of processes that have opened but not closed this mailbox
unsigned int do_mbox_usage_count(mbox_t mbox) {
  (void) mbox;
  return MessageBoxen[mbox].usage_count;
  // TODO: Fill this in
}
