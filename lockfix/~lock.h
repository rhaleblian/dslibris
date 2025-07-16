#ifndef __SYS_LOCK_H__
#define __SYS_LOCK_H__

#include <_ansi.h>
#include <stdint.h>

typedef int32_t _LOCK_T;

struct __lock_t {
	_LOCK_T lock;
	uint32_t thread_tag;
	uint32_t counter;
};

typedef struct __lock_t _LOCK_RECURSIVE_T;

typedef uint32_t _COND_T;

#define __LOCK_INITIALIZER ((_LOCK_T)0)
#define __LOCK_INITIALIZER_RECURSIVE ((_LOCK_RECURSIVE_T){__LOCK_INITIALIZER,0,0})
#define __COND_INITIALIZER ((_COND_T)0)

#ifdef __cplusplus
extern "C" {
#endif

static inline void __libc_lock_init(_LOCK_T *lock) {
        *lock = __LOCK_INITIALIZER;
}

static inline void __libc_lock_close(_LOCK_T *lock ) {}

static inline void __libc_lock_init_recursive(_LOCK_RECURSIVE_T *lock) {
        *lock = __LOCK_INITIALIZER_RECURSIVE;
}

static inline void __libc_lock_close_recursive(_LOCK_RECURSIVE_T *lock ) {}

extern void __libc_lock_acquire(_LOCK_T *lock);
extern void __libc_lock_acquire_recursive(_LOCK_RECURSIVE_T *lock);
extern void __libc_lock_release(_LOCK_T *lock);
extern void __libc_lock_release_recursive(_LOCK_RECURSIVE_T *lock);

/* Returns 0 for success and non-zero for failure */
extern int __libc_lock_try_acquire(_LOCK_T *lock);
extern int __libc_lock_try_acquire_recursive(_LOCK_RECURSIVE_T *lock);

/* Returns errno */
static inline int __libc_cond_init(_COND_T *cond) {
        *cond = __COND_INITIALIZER;
}

extern int __libc_cond_signal(_COND_T *cond);
extern int __libc_cond_broadcast(_COND_T *cond);
extern int __libc_cond_wait(_COND_T *cond, _LOCK_T *lock, uint64_t timeout_ns);
extern int __libc_cond_wait_recursive(_COND_T *cond, _LOCK_RECURSIVE_T *lock, uint64_t timeout_ns);

#ifdef __cplusplus
}
#endif


#define __LOCK_INIT(CLASS,NAME) \
CLASS _LOCK_T NAME = __LOCK_INITIALIZER;

#define __LOCK_INIT_RECURSIVE(CLASS,NAME) \
CLASS _LOCK_RECURSIVE_T NAME = __LOCK_INITIALIZER_RECURSIVE;

#define __COND_INIT(CLASS,NAME) \
CLASS _COND_T NAME = __COND_INITIALIZER;

#define __lock_init(NAME) \
	__libc_lock_init(&(NAME))

#define __lock_init_recursive(NAME) \
	__libc_lock_init_recursive(&(NAME))

#define __lock_close(NAME) \
	__libc_lock_close(&(NAME))

#define __lock_close_recursive(NAME) \
	__libc_lock_close_recursive(&(NAME))

#define __lock_acquire(NAME) \
	__libc_lock_acquire(&(NAME))

#define __lock_acquire_recursive(NAME) \
	__libc_lock_acquire_recursive(&(NAME))

#define __lock_try_acquire(NAME) \
	__libc_lock_try_acquire(&(NAME))

#define __lock_try_acquire_recursive(NAME) \
	__libc_lock_try_acquire_recursive(&(NAME))

#define __lock_release(NAME) \
	__libc_lock_release(&(NAME))

#define __lock_release_recursive(NAME) \
	__libc_lock_release_recursive(&(NAME))

#define __cond_init(NAME) \
        __libc_cond_init(&(NAME))

#define __cond_signal(NAME) \
        __libc_cond_signal(&(NAME))

#define __cond_broadcast(NAME) \
        __libc_cond_broadcast(&(NAME))

#define __cond_wait(NAME, LOCK, TIMEOUT) \
        __libc_cond_wait(&(NAME), &(LOCK), (TIMEOUT))

#define __cond_wait_recursive(NAME, LOCK, TIMEOUT) \
        __libc_cond_wait_recursive(&(NAME), &(LOCK), (TIMEOUT))


#endif // __SYS_LOCK_H__
