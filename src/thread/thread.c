/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
 * All rights reserved.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#include <cc/alloc.h>
#include <cc/array.h>
#include <cc/string.h>
#include <cc/thread.h>

/* Arguments and callback to setup and run the user thread function */
typedef struct _cc_thread_args {
    _cc_thread_callback_t callback;
    void *user_args;
    _cc_semaphore_t *wait;
    _cc_thread_t *info;
} _cc_thread_args_t;

/**/
void _cc_thread_running_function(void *args) {
    _cc_thread_args_t *thread_args;
    _cc_thread_t *self;
    _cc_thread_callback_t user_func;
    void *user_args;

    /* Get the thread id */
    thread_args = (_cc_thread_args_t *)args;
    thread_args->info->thread_id = _cc_get_current_sys_thread_id();

    /* Figure out what function to run */
    self = thread_args->info;
    user_func = thread_args->callback;
    user_args = thread_args->user_args;

    /* Perform any system-dependent setup - this function may not fail */
    _cc_setup_sys_thread(self->name);

    /* Wake up the parent thread */
    _cc_semaphore_post(thread_args->wait);

    /* Run the function */
    self->status = user_func(self, user_args);

    /* Mark us as ready to be joined (or detached) */
    if (!_cc_atomic32_cas(&self->state, _CC_THREAD_STATE_ALIVE_, _CC_THREAD_STATE_ZOMBIE_)) {
        /* Clean up if something already detached us. */
        if (_cc_atomic32_cas(&self->state, _CC_THREAD_STATE_DETACHED_, _CC_THREAD_STATE_CLEANED_)) {
            _cc_safe_free(self->name);
            _cc_free(self);
        }
    }
}
/**/
_cc_thread_t *_cc_create_thread(_cc_thread_callback_t callback, const tchar_t *name, pvoid_t args) {
    return _cc_create_thread_with_stacksize(callback, name, 0, args);
}
/**/
_cc_thread_t *_cc_create_thread_with_stacksize(_cc_thread_callback_t callback, const tchar_t *name, size_t stacksize,
                                               pvoid_t args) {
    _cc_thread_t *self;
    _cc_thread_args_t thread_args;

    /* Allocate memory for the thread info structure */
    self = _CC_MALLOC(_cc_thread_t);
    bzero(self, sizeof(_cc_thread_t));
    self->status = -1;
    self->stacksize = stacksize > 0 ? stacksize : 0;
    _cc_atomic32_set(&self->state, _CC_THREAD_STATE_ALIVE_);

    if (name != NULL) {
        self->name = _cc_tcsdup(name);
    }

    /* Set up the arguments for the thread */
    thread_args.callback = callback;
    thread_args.user_args = args;
    thread_args.info = self;
    thread_args.wait = _cc_create_semaphore(0);

    if (_cc_unlikely(thread_args.wait == NULL)) {
        _cc_free(self);
        return (NULL);
    }

    /* Create the thread and go! */
    if (_cc_likely(_cc_create_sys_thread(self, &thread_args))) {
        /* Wait for the thread function to use arguments */
        _cc_semaphore_wait(thread_args.wait);
    } else {
        /* Oops, failed.  Gotta free everything */
        _cc_free(self);
        self = NULL;
    }

    _cc_destroy_semaphore(&thread_args.wait);

    /* Everything is running now */
    return self;
}

/**/
bool_t _cc_thread_start(_cc_thread_callback_t callback, const tchar_t *name, pvoid_t args) {
    _cc_thread_t *self = _cc_create_thread_with_stacksize(callback, name, 0, args);
    if (_cc_likely(self)) {
        _cc_detach_thread(self);
        return true;
    }
    return false;
}

/**/
int32_t _cc_get_thread_id(_cc_thread_t *self) {
    return _cc_get_sys_thread_id(self);
}

/**/
void _cc_wait_thread(_cc_thread_t *self, int32_t *status) {
    _cc_assert(self != NULL);

    _cc_wait_sys_thread(self);
    if (status) {
        *status = self->status;
    }
    _cc_safe_free(self->name);
    _cc_free(self);
}

/**/
void _cc_detach_thread(_cc_thread_t *self) {
    _cc_assert(self != NULL);

    /* Grab dibs if the state is alive+joinable. */
    if (_cc_atomic32_cas(&self->state, _CC_THREAD_STATE_ALIVE_, _CC_THREAD_STATE_DETACHED_)) {
        _cc_detach_sys_thread(self);
    } else {
        /* all other states are pretty final, see where we landed. */
        const _cc_atomic32_t thread_state = _cc_atomic32_get(&self->state);
        if ((thread_state == _CC_THREAD_STATE_DETACHED_) || (thread_state == _CC_THREAD_STATE_CLEANED_)) {
            return; /* already detached (you shouldn't call this twice!) */
        } else if (thread_state == _CC_THREAD_STATE_ZOMBIE_) {
            _cc_wait_thread(self, NULL); /* already done, clean it up. */
        } else {
            _cc_assert(0 && "Unexpected thread state");
        }
    }
}
