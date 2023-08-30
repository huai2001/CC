# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
CMAKE_MINIMUM_REQUIRED(VERSION 3.6)

SET(CMAKE_WARN_VS10, OFF)
SET(CMAKE_SUPPRESS_REGENERATION TRUE)

# Set the project name
PROJECT(libcc)

SET(PROJECT_NAME, "libcc")
SET(PROJECT_C_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

SET_PROPERTY(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
# using Clang
SET(CMAKE_C_COMPILER, "clang")
ELSEIF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
# using GCC
SET(CMAKE_C_COMPILER, "gcc")
ELSEIF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
# using Intel C++
SET(CMAKE_C_COMPILER, "intel")
ELSEIF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
# using Visual Studio C++
SET(CMAKE_C_COMPILER, "gcc")
ENDIF()

MESSAGE("PROJECT_NAME:${PROJECT_NAME}")
MESSAGE("SOURCE:${PROJECT_C_SOURCE_DIR}")

# Generate the static library from the library sources
SET(SRC_LIST
    ${PROJECT_C_SOURCE_DIR}/aes.c
    ${PROJECT_C_SOURCE_DIR}/base16.c
    ${PROJECT_C_SOURCE_DIR}/base64.c
    ${PROJECT_C_SOURCE_DIR}/crc.c
    ${PROJECT_C_SOURCE_DIR}/md2.c
    ${PROJECT_C_SOURCE_DIR}/md4.c
    ${PROJECT_C_SOURCE_DIR}/md5.c
    ${PROJECT_C_SOURCE_DIR}/sha1.c
    ${PROJECT_C_SOURCE_DIR}/sha256.c
    ${PROJECT_C_SOURCE_DIR}/sha512.c
    ${PROJECT_C_SOURCE_DIR}/sha3.c
    ${PROJECT_C_SOURCE_DIR}/des.c
	${PROJECT_C_SOURCE_DIR}/xxtea.c
    ${PROJECT_C_SOURCE_DIR}/UTF.c
    ${PROJECT_C_SOURCE_DIR}/GBK.c
    ${PROJECT_C_SOURCE_DIR}/list.c
    ${PROJECT_C_SOURCE_DIR}/queue.c
    ${PROJECT_C_SOURCE_DIR}/hashtable.c
    ${PROJECT_C_SOURCE_DIR}/rbtree.c
    ${PROJECT_C_SOURCE_DIR}/array.c
    ${PROJECT_C_SOURCE_DIR}/string.c
    ${PROJECT_C_SOURCE_DIR}/buf.c
    ${PROJECT_C_SOURCE_DIR}/url.c
    ${PROJECT_C_SOURCE_DIR}/uuid.c
    ${PROJECT_C_SOURCE_DIR}/rand.c
    ${PROJECT_C_SOURCE_DIR}/logger.c
    ${PROJECT_C_SOURCE_DIR}/alloc.c
    ${PROJECT_C_SOURCE_DIR}/socket/inet.c
    ${PROJECT_C_SOURCE_DIR}/socket/socket.c
    ${PROJECT_C_SOURCE_DIR}/thread/thread.c
    ${PROJECT_C_SOURCE_DIR}/time/strptime.c
    ${PROJECT_C_SOURCE_DIR}/time/time.c
    ${PROJECT_C_SOURCE_DIR}/atomic/atomic.c
    ${PROJECT_C_SOURCE_DIR}/atomic/spinlock.c
    ${PROJECT_C_SOURCE_DIR}/atomic/rwlock.c
    ${PROJECT_C_SOURCE_DIR}/core/cpu_info.c
    ${PROJECT_C_SOURCE_DIR}/core/generic.c
    ${PROJECT_C_SOURCE_DIR}/core/file.c
    ${PROJECT_C_SOURCE_DIR}/power/power.c
)
IF (SRC_EVENT_LIST)
    LIST(APPEND SRC_LIST
        ${PROJECT_C_SOURCE_DIR}/event/select.c
        ${PROJECT_C_SOURCE_DIR}/event/event.c
        ${PROJECT_C_SOURCE_DIR}/event/loop.c
        ${PROJECT_C_SOURCE_DIR}/event/timeout.c
        ${PROJECT_C_SOURCE_DIR}/event/tcp.c
        ${PROJECT_C_SOURCE_DIR}/event/buffer.c
    )
ENDIF ()

IF (UNIX)
    ##freebse,unix
    IF (SRC_EVENT_LIST)
        LIST(APPEND SRC_LIST
            ${PROJECT_C_SOURCE_DIR}/event/unix/sys_kqueue.c
        )
    ENDIF ()
    LIST(APPEND SRC_LIST
        ${PROJECT_C_SOURCE_DIR}/time/unix/sys_time.c
        ${PROJECT_C_SOURCE_DIR}/core/file.c
        ${PROJECT_C_SOURCE_DIR}/core/unix.c
        ${PROJECT_C_SOURCE_DIR}/core/unix/sys_core.c
        ${PROJECT_C_SOURCE_DIR}/core/unix/sys_locale.c
        ${PROJECT_C_SOURCE_DIR}/socket/unix/sys_socket.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_thread.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_cond.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_mutex.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_sem.c
        ${PROJECT_C_SOURCE_DIR}/loadso/dlopen/sys_loadso.c
    )
ELSEIF (APPLE)
    #macosx
    IF (SRC_EVENT_LIST)
        LIST(APPEND SRC_LIST
            ${PROJECT_C_SOURCE_DIR}/event/unix/sys_kqueue.c
        )
    ENDIF ()

    LIST(APPEND SRC_LIST
        ${PROJECT_C_SOURCE_DIR}/time/unix/sys_time.c
        ${PROJECT_C_SOURCE_DIR}/core/OSX/sys_file.m
        ${PROJECT_C_SOURCE_DIR}/core/OSX/sys_core.m
        ${PROJECT_C_SOURCE_DIR}/core/OSX/sys_locale.m
        ${PROJECT_C_SOURCE_DIR}/core/unix.c
        ${PROJECT_C_SOURCE_DIR}/power/macosx/sys_power.c
        ${PROJECT_C_SOURCE_DIR}/socket/unix/sys_socket.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_thread.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_cond.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_mutex.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_sem.c
        ${PROJECT_C_SOURCE_DIR}/loadso/dlopen/sys_loadso.c
    )
ELSEIF (IPHONE)
    #ios
    IF (SRC_EVENT_LIST)
        LIST(APPEND SRC_LIST
            ${PROJECT_C_SOURCE_DIR}/event/unix/sys_kqueue.c
        )
    ENDIF ()

    LIST(APPEND SRC_LIST
        ${PROJECT_C_SOURCE_DIR}/time/unix/sys_time.c
        ${PROJECT_C_SOURCE_DIR}/core/IOS/sys_file.c
        ${PROJECT_C_SOURCE_DIR}/core/IOS/sys_core.c
        ${PROJECT_C_SOURCE_DIR}/core/unix.c
        ${PROJECT_C_SOURCE_DIR}/socket/unix/sys_socket.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_thread.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_cond.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_mutex.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_sem.c
    )
ELSEIF (LINUX)
    #linux
    IF (SRC_EVENT_LIST)
        LIST(APPEND SRC_LIST
            ${PROJECT_C_SOURCE_DIR}/event/linux/sys_epoll.c
        )
    ENDIF ()
    LIST(APPEND SRC_LIST
        ${PROJECT_C_SOURCE_DIR}/time/linux/sys_time.c
        ${PROJECT_C_SOURCE_DIR}/core/file.c
        ${PROJECT_C_SOURCE_DIR}/core/unix.c
        ${PROJECT_C_SOURCE_DIR}/core/unix/sys_core.c
        ${PROJECT_C_SOURCE_DIR}/core/unix/sys_locale.c
        ${PROJECT_C_SOURCE_DIR}/power/linux/sys_power.c
        ${PROJECT_C_SOURCE_DIR}/socket/linux/sys_socket.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_thread.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_cond.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_mutex.c
        ${PROJECT_C_SOURCE_DIR}/thread/pthread/sys_sem.c
        ${PROJECT_C_SOURCE_DIR}/loadso/dlopen/sys_loadso
    )
ELSEIF (WIN32)
    #windows
    IF (SRC_EVENT_LIST)
        LIST(APPEND SRC_LIST
        ${PROJECT_C_SOURCE_DIR}/socket/windows/sys_iocp.c
        ${PROJECT_C_SOURCE_DIR}/socket/windows/sys_WSA.c
        ${PROJECT_C_SOURCE_DIR}/socket/windows/sys_iocp_overlapped.c
        )
    ENDIF ()

    LIST(APPEND SRC_LIST
        ${PROJECT_C_SOURCE_DIR}/time/windows/sys_time.c
        ${PROJECT_C_SOURCE_DIR}/core/file.c
        ${PROJECT_C_SOURCE_DIR}/core/windows.c
        ${PROJECT_C_SOURCE_DIR}/core/windows/sys_core.c
        ${PROJECT_C_SOURCE_DIR}/core/windows/sys_locale.c
        ${PROJECT_C_SOURCE_DIR}/core/windows/sys_dirent.c
        ${PROJECT_C_SOURCE_DIR}/core/windows/sys_file.c
        ${PROJECT_C_SOURCE_DIR}/core/windows/sys_pipe.c
        ${PROJECT_C_SOURCE_DIR}/power/windows/sys_power.c
        ${PROJECT_C_SOURCE_DIR}/socket/windows/sys_socket.c
        ${PROJECT_C_SOURCE_DIR}/thread/windows/sys_thread.c
        ${PROJECT_C_SOURCE_DIR}/thread/windows/sys_cond.c
        ${PROJECT_C_SOURCE_DIR}/thread/windows/sys_mutex.c
        ${PROJECT_C_SOURCE_DIR}/thread/windows/sys_sem.c
        ${PROJECT_C_SOURCE_DIR}/loadso/windows/sys_loadso.c
    )
ENDIF ()

IF (SRC_JSON_LIST)
    LIST(APPEND SRC_LIST 
        ${PROJECT_C_SOURCE_DIR}/json/json.c
        ${PROJECT_C_SOURCE_DIR}/json/json.parser.c
    )
ENDIF ()

IF(SRC_XML_LIST)
    LIST(APPEND SRC_LIST 
        ${PROJECT_C_SOURCE_DIR}/xml/xml.c
        ${PROJECT_C_SOURCE_DIR}/xml/xml.parser.c
    )
ENDIF ()

IF(SRC_INI_LIST)
    LIST(APPEND SRC_LIST 
        ${PROJECT_C_SOURCE_DIR}/ini/ini.c
        ${PROJECT_C_SOURCE_DIR}/ini/ini.parser.c
    )
ENDIF ()

IF(SRC_DB_LIST)
    ADD_DEFINITIONS(
        -D_CC_USE_SYSTEM_SQLITE3_LIB_
        -D_CC_ENABLE_UNIXODBC_
    )
    
    LIST(APPEND SRC_LIST 
        ${PROJECT_C_SOURCE_DIR}/db/mysql.c
        ${PROJECT_C_SOURCE_DIR}/db/sqlite.c
        ${PROJECT_C_SOURCE_DIR}/db/sqlsvr.c
        ${PROJECT_C_SOURCE_DIR}/db/oci8.c
    )
ENDIF ()

INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/include)

LINK_DIRECTORIES (${PROJECT_SOURCE_DIR}/lib)

ADD_LIBRARY(
        # Sets the name of the library.
        ${PROJECT_NAME}
        # Sets the library as a shared library.
        #SHARED

        # Provides a relative path to your source file(s).
        # src/.c
        ${SRC_LIST}
    )
