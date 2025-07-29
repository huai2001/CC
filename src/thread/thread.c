/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
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
#include <libcc/alloc.h>
#include <libcc/array.h>
#include <libcc/string.h>
#include <libcc/thread.h>
/**/
_CC_API_PUBLIC(void) _cc_thread_running_function(void *args) {
    _cc_thread_t *self;
    _cc_thread_callback_t user_func;
    void *user_args;

    /* Get the thread id */
    self = (_cc_thread_t *)args;
    self->thread_id = _cc_get_current_sys_thread_id();

    /* Figure out what function to run */
    user_func = self->callback;
    user_args = self->user_args;

    /* Perform any system-dependent setup - this function may not fail */
    _cc_setup_sys_thread(self->name);

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
_CC_API_PUBLIC(_cc_thread_t*) _cc_thread(_cc_thread_callback_t callback, const tchar_t *name, pvoid_t args) {
    return _cc_thread_with_stacksize(callback, name, 0, args);
}
/**/
_CC_API_PUBLIC(_cc_thread_t*) _cc_thread_with_stacksize(_cc_thread_callback_t callback, const tchar_t *name, size_t stacksize,
                                               pvoid_t args) {
    _cc_thread_t *self;

    /* Allocate memory for the thread info structure */
    self = _CC_MALLOC(_cc_thread_t);
    bzero(self, sizeof(_cc_thread_t));
    self->status = -1;
    self->stacksize = stacksize > 0 ? stacksize : 0;
    _cc_atomic32_set(&self->state, _CC_THREAD_STATE_ALIVE_);

    if (name != nullptr) {
        self->name = _cc_tcsdup(name);
    }

    /* Set up the arguments for the thread */
    self->callback = callback;
    self->user_args = args;

    /* Create the thread and go! */
    if (_cc_unlikely(!_cc_create_sys_thread(self))) {
        /* Oops, failed.  Gotta free everything */
        _cc_free(self);
        self = nullptr;
    }

    /* Everything is running now */
    return self;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_thread_start(_cc_thread_callback_t callback, const tchar_t *name, pvoid_t args) {
    _cc_thread_t *self = _cc_thread_with_stacksize(callback, name, 0, args);
    if (_cc_likely(self)) {
        _cc_detach_thread(self);
        return true;
    }
    return false;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_thread_id(_cc_thread_t *self) {
    return _cc_get_sys_thread_id(self);
}

/**/
_CC_API_PUBLIC(void) _cc_wait_thread(_cc_thread_t *self, int32_t *status) {
    _cc_assert(self != nullptr);

    _cc_wait_sys_thread(self);
    if (status) {
        *status = self->status;
    }
    _cc_safe_free(self->name);
    _cc_free(self);
}

/**/
_CC_API_PUBLIC(void) _cc_detach_thread(_cc_thread_t *self) {
    _cc_assert(self != nullptr);

    /* Grab dibs if the state is alive+joinable. */
    if (_cc_atomic32_cas(&self->state, _CC_THREAD_STATE_ALIVE_, _CC_THREAD_STATE_DETACHED_)) {
        _cc_detach_sys_thread(self);
    } else {
        /* all other states are pretty final, see where we landed. */
        const _cc_atomic32_t thread_state = _cc_atomic32_get(&self->state);
        if ((thread_state == _CC_THREAD_STATE_DETACHED_) || (thread_state == _CC_THREAD_STATE_CLEANED_)) {
            return; /* already detached (you shouldn't call this twice!) */
        } else if (thread_state == _CC_THREAD_STATE_ZOMBIE_) {
            _cc_wait_thread(self, nullptr); /* already done, clean it up. */
        } else {
            _cc_assert(0 && "Unexpected thread state");
        }
    }
}
