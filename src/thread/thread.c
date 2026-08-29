#include <libcc/alloc.h>
#include <libcc/array.h>
#include <libcc/string.h>
#include <libcc/thread.h>

/**/
_CC_API_DYLIB_PRIVATE(bool_t) _cc_create_sys_thread(_cc_thread_t* self);
/**/
_CC_API_DYLIB_PRIVATE(size_t) _cc_get_current_sys_thread_id(void);
/**/
_CC_API_DYLIB_PRIVATE(size_t) _cc_get_sys_thread_id(_cc_thread_t* self);
/**/
_CC_API_DYLIB_PRIVATE(void) _cc_setup_sys_thread(const tchar_t* name);
/**/
_CC_API_DYLIB_PRIVATE(void) _cc_wait_sys_thread(_cc_thread_t* self);
/**/
_CC_API_DYLIB_PRIVATE(void) _cc_detach_sys_thread(_cc_thread_t* self);

/**/
_CC_API_DYLIB_PRIVATE(void) _cc_thread_running_function(void *args) {
    _cc_thread_t *self;
    _cc_thread_callback_t user_func;
    void *user_args;

    /* Get the thread id */
    self = (_cc_thread_t *)args;
    self->thread_id = _cc_get_current_sys_thread_id();

    /* Figure out what function to run */
    user_func = self->callback;
    user_args = self->args;

    /* Perform any system-dependent setup - this function may not fail */
    if (self->name) {
        _cc_setup_sys_thread(self->name);
    }

    /* Run the function */
    self->status = user_func(user_args);

    /* Mark us as ready to be joined (or detached) */
    if (!_cc_atomic32_cas(&self->state, _CC_THREAD_STATE_ALIVE_, _CC_THREAD_STATE_COMPLETE_)) {
        /* Clean up if something already detached us. */
        if (_cc_atomic32_load(&self->state) == _CC_THREAD_STATE_DETACHED_) {
            if (self->name) {
                _cc_sds_free(self->name);
            }
            _cc_free(self);
        }
    }
}

/**/
_CC_API_PUBLIC(_cc_thread_t*) _cc_thread(_cc_thread_callback_t callback, const tchar_t *name, pvoid_t args) {
    return _cc_thread_with_stacksize(callback, name, 0, args);
}

/**/
_CC_API_PUBLIC(_cc_thread_t*) _cc_thread_with_stacksize(_cc_thread_callback_t callback, const tchar_t *name, size_t stacksize, pvoid_t args) {
    _cc_thread_t *self;
    if (!callback) {
        _cc_logger(_CC_LOG_LEVEL_ERROR_,_T("Thread entry function is NULL"));
        return NULL;
    }
    /* Allocate memory for the thread info structure */
    self = (_cc_thread_t*)_cc_calloc(1,sizeof(_cc_thread_t));
    self->status = -1;
    self->stack_size = stacksize > 0 ? stacksize : 0;
    _cc_atomic32_set(&self->state, _CC_THREAD_STATE_ALIVE_);

    if (name != NULL) {
        self->name = _cc_sds_alloc(name,0);
    }

    /* Set up the arguments for the thread */
    self->callback = callback;
    self->args = args;

    /* Create the thread and go! */
    if (_cc_unlikely(!_cc_create_sys_thread(self))) {
        /* Oops, failed.  Gotta free everything */
        _cc_free(self);
        self = NULL;
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
_CC_API_PUBLIC(size_t) _cc_get_thread_id(_cc_thread_t *self) {
    return _cc_get_sys_thread_id(self);
}

/**/
_CC_API_PUBLIC(void) _cc_wait_thread(_cc_thread_t *self, int32_t *status) {
    _cc_assert(self != NULL);

    _cc_wait_sys_thread(self);
    if (status) {
        *status = self->status;
    }
    if (self->name) {
        _cc_sds_free(self->name);
    }
    _cc_free(self);
}

/**/
_CC_API_PUBLIC(void) _cc_detach_thread(_cc_thread_t *self) {
    _cc_assert(self != NULL);

    /* Grab dibs if the state is alive+joinable. */
    if (_cc_atomic32_cas(&self->state, _CC_THREAD_STATE_ALIVE_, _CC_THREAD_STATE_DETACHED_)) {
        _cc_detach_sys_thread(self);
    } else {
        /* all other states are pretty final, see where we landed. */
        const _cc_atomic32_t state = _cc_atomic32_load(&self->state);
        if (state == _CC_THREAD_STATE_DETACHED_) {
            /* already detached (you shouldn't call this twice!) */
            return;
        } else if (state == _CC_THREAD_STATE_COMPLETE_) {
            /* already done, clean it up. */
            _cc_wait_thread(self, NULL);
        } else {
            _cc_assert(0 && "Unexpected thread state");
        }
    }
}
