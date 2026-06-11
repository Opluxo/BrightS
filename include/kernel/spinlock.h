#ifndef BRIGHTS_SPINLOCK_H
#define BRIGHTS_SPINLOCK_H

#include <stdint.h>

/* Simple spinlock for SMP synchronization */
typedef struct {
  volatile uint32_t locked;  /* 0 = unlocked, 1 = locked */
} brights_spinlock_t;

/* Initialize a spinlock */
static inline void brights_spinlock_init(brights_spinlock_t *lock)
{
  lock->locked = 0;
}

/* Acquire spinlock with interrupt disable */
static inline void brights_spinlock_acquire(brights_spinlock_t *lock)
{
  /* Disable interrupts while spinning */
  __asm__ __volatile__("cli");

  /* Test-and-set loop */
  while (__sync_lock_test_and_set(&lock->locked, 1)) {
    /* Spin with pause hint */
    while (lock->locked) {
      __asm__ __volatile__("pause");
    }
  }
}

/* Release spinlock and restore interrupt state */
static inline void brights_spinlock_release(brights_spinlock_t *lock)
{
  __sync_lock_release(&lock->locked);
  /* Re-enable interrupts */
  __asm__ __volatile__("sti");
}

/* Acquire without disabling interrupts (for short critical sections) */
static inline void brights_spinlock_acquire_noirq(brights_spinlock_t *lock)
{
  while (__sync_lock_test_and_set(&lock->locked, 1)) {
    while (lock->locked) {
      __asm__ __volatile__("pause");
    }
  }
}

/* Release without restoring interrupts */
static inline void brights_spinlock_release_noirq(brights_spinlock_t *lock)
{
  __sync_lock_release(&lock->locked);
}

#endif
