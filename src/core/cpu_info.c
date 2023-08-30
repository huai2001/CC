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
#include <cc/core.h>
#include <cc/string.h>
#if defined(__WIN32__)
#include <cc/core/windows/windows.h>
#endif

#ifdef _CC_HAVE_SYSCONF_
#include <unistd.h>
#endif

#ifdef _CC_HAVE_SYSCTLBYNAME_
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef __CC_MACOSX__
#include <mach/task_info.h>
#include <mach/mach_types.h>
#include <mach/mach.h>
#elif defined(__CC_MACOSX__) && (defined(__ppc__) || defined(__ppc64__))
#include <sys/sysctl.h> /* For AltiVec check */
#elif defined(__CC_OPENBSD__) && defined(__powerpc__)
#include <sys/param.h>
#include <sys/sysctl.h> /* For AltiVec check */
#include <machine/cpu.h>
#endif

/**/
int _cc_cpu_number_processors = 0;

static int CPU_haveCPUID(void) {
    int has_CPUID = 0;
/* *INDENT-OFF* */
#if defined(__GNUC__) && defined(i386)
    __asm__ (
"        pushfl                      # Get original EFLAGS             \n"
"        popl    %%eax                                                 \n"
"        movl    %%eax,%%ecx                                           \n"
"        xorl    $0x200000,%%eax     # Flip ID bit in EFLAGS           \n"
"        pushl   %%eax               # Save new EFLAGS value on stack  \n"
"        popfl                       # Replace current EFLAGS value    \n"
"        pushfl                      # Get new EFLAGS                  \n"
"        popl    %%eax               # Store new EFLAGS in EAX         \n"
"        xorl    %%ecx,%%eax         # Can not toggle ID bit,          \n"
"        jz      1f                  # Processor=80486                 \n"
"        movl    $1,%0               # We have CPUID support           \n"
"1:                                                                    \n"
    : "=m" (has_CPUID)
    :
    : "%eax", "%ecx"
    );
#elif defined(__GNUC__) && defined(__x86_64__)
/* Technically, if this is being compiled under __x86_64__ then it has 
   CPUid by definition.  But it's nice to be able to prove it.  :)      */
    __asm__ (
"        pushfq                      # Get original EFLAGS             \n"
"        popq    %%rax                                                 \n"
"        movq    %%rax,%%rcx                                           \n"
"        xorl    $0x200000,%%eax     # Flip ID bit in EFLAGS           \n"
"        pushq   %%rax               # Save new EFLAGS value on stack  \n"
"        popfq                       # Replace current EFLAGS value    \n"
"        pushfq                      # Get new EFLAGS                  \n"
"        popq    %%rax               # Store new EFLAGS in EAX         \n"
"        xorl    %%ecx,%%eax         # Can not toggle ID bit,          \n"
"        jz      1f                  # Processor=80486                 \n"
"        movl    $1,%0               # We have CPUID support           \n"
"1:                                                                    \n"
    : "=m" (has_CPUID)
    :
    : "%rax", "%rcx"
    );
#elif (defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)
    __asm {
        pushfd                      ; Get original EFLAGS
        pop     eax
        mov     ecx, eax
        xor     eax, 200000h        ; Flip ID bit in EFLAGS
        push    eax                 ; Save new EFLAGS value on stack
        popfd                       ; Replace current EFLAGS value
        pushfd                      ; Get new EFLAGS
        pop     eax                 ; Store new EFLAGS in EAX
        xor     eax, ecx            ; Can not toggle ID bit,
        jz      done                ; Processor=80486
        mov     has_CPUID,1         ; We have CPUID support
done:
    }
#elif defined(_MSC_VER) && defined(_M_X64)
    has_CPUID = 1;
#elif defined(__sun) && defined(__i386)
    __asm (
"       pushfl                 \n"
"       popl    %eax           \n"
"       movl    %eax,%ecx      \n"
"       xorl    $0x200000,%eax \n"
"       pushl   %eax           \n"
"       popfl                  \n"
"       pushfl                 \n"
"       popl    %eax           \n"
"       xorl    %ecx,%eax      \n"
"       jz      1f             \n"
"       movl    $1,-8(%ebp)    \n"
"1:                            \n"
    );
#elif defined(__sun) && defined(__amd64)
    __asm (
"       pushfq                 \n"
"       popq    %rax           \n"
"       movq    %rax,%rcx      \n"
"       xorl    $0x200000,%eax \n"
"       pushq   %rax           \n"
"       popfq                  \n"
"       pushfq                 \n"
"       popq    %rax           \n"
"       xorl    %ecx,%eax      \n"
"       jz      1f             \n"
"       movl    $1,-8(%rbp)    \n"
"1:                            \n"
    );
#endif
/* *INDENT-ON* */
    return has_CPUID;
}
#if defined(__GNUC__) && defined(i386)
#define cpuid(func, a, b, c, d) \
    __asm__ __volatile__ ( \
"        pushl %%ebx        \n" \
"        xorl %%ecx,%%ecx   \n" \
"        cpuid              \n" \
"        movl %%ebx, %%esi  \n" \
"        popl %%ebx         \n" : \
            "=a" (a), "=S" (b), "=c" (c), "=d" (d) : "a" (func))
#elif defined(__GNUC__) && defined(__x86_64__)
#define cpuid(func, a, b, c, d) \
    __asm__ __volatile__ ( \
"        pushq %%rbx        \n" \
"        xorq %%rcx,%%rcx   \n" \
"        cpuid              \n" \
"        movq %%rbx, %%rsi  \n" \
"        popq %%rbx         \n" : \
            "=a" (a), "=S" (b), "=c" (c), "=d" (d) : "a" (func))
#elif (defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)
#define cpuid(func, a, b, c, d) \
    __asm { \
        __asm mov eax, func \
        __asm xor ecx, ecx \
        __asm cpuid \
        __asm mov a, eax \
        __asm mov b, ebx \
        __asm mov c, ecx \
        __asm mov d, edx \
}
#elif defined(_MSC_VER) && defined(_M_X64)
#define cpuid(func, a, b, c, d) do { \
    int CPUInfo[4]; \
    __cpuid(CPUInfo, func); \
    a = CPUInfo[0]; \
    b = CPUInfo[1]; \
    c = CPUInfo[2]; \
    d = CPUInfo[3]; \
} while (0)
#else
#define cpuid(func, a, b, c, d) \
    do { a = b = c = d = 0; (void) a; (void) b; (void) c; (void) d; } while (0)
#endif

/**/
int _cc_cpu_count(void) {
#if defined(_SC_NPROCESSORS_ONLN)
    if (_cc_cpu_number_processors <= 0) {
        _cc_cpu_number_processors = (int)sysconf(_SC_NPROCESSORS_ONLN);
    }
#endif

#ifdef __CC_WINDOWS__
    if (_cc_cpu_number_processors <= 0) {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        _cc_cpu_number_processors = info.dwNumberOfProcessors;
    }
#endif

#ifdef _CC_HAVE_SYSCTLBYNAME_
    if (_cc_cpu_number_processors <= 0) {
        size_t size = sizeof(_cc_cpu_number_processors);
        sysctlbyname("hw.ncpu", &_cc_cpu_number_processors, &size, NULL, 0);
    }
#endif
#ifdef __CC_OS2__
    if (_cc_cpu_number_processors <= 0) {
        DosQuerySysInfo(QSV_NUMPROCESSORS, QSV_NUMPROCESSORS,
                        &_cc_cpu_number_processors, sizeof(_cc_cpu_number_processors) );
    }
#endif
    /* There has to be at least 1, right? :) */
    if (_cc_cpu_number_processors <= 0) {
        _cc_cpu_number_processors = 1;
    }
    return _cc_cpu_number_processors;
}

const tchar_t* _cc_cpu_sn(void) {
    static tchar_t _cc_cpu_sn_value[32] = {0};
    int a, b, c, d;

    if (_cc_cpu_sn_value[0]) {
        return _cc_cpu_sn_value;
    }

    if (CPU_haveCPUID()) {
        cpuid(0x00000000, a, b, c, d);
        _sntprintf(_cc_cpu_sn_value, _cc_countof(_cc_cpu_sn_value), _T("%04X-%04X-%04X-%04X-%04X-%04X"),
            a>> 16, a & 0xffff,
            d>> 16, d & 0xffff,
            c>> 16, c & 0xffff);
    }

    return _cc_cpu_sn_value;
}
