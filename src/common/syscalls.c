// Standard includes
#include <stdio.h>
#include <sys/stat.h>
// Project includes
#include <hal.h>
#include "bsp_board.h"

static void syscallsInit(void) __attribute__((constructor));

static void syscallsInit(void)
{
    // Turn off buffers, so I/O occurs immediately
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          Stub for file write
*/
/*----------------------------------------------------------------------------*/
ssize_t _write(int fd, const char *buf, size_t nbyte)
{
    (void)fd;
    for (uint32_t i = 0; i < nbyte; i++)
    {
        if (buf[i] == '\n')
        {
            boardSerialSend('\r');
        }
        boardSerialSend(buf[i]);
    }

    return nbyte;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          Stub for file read
*/
/*----------------------------------------------------------------------------*/
int _read(int fd, char *ptr, int len)
{
    (void)fd;
    (void)len;
#ifdef BOOTLOADER
    return boardSerialReceive(ptr);
#else
    ptr[0] = (char)boardSerialReceive();
    return ptr[0] == EOF ? 0 : 1;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          Stub for file close
*/
/*----------------------------------------------------------------------------*/
int _close(int fd)
{
    (void)fd;
    return -1;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          Stub for file open status

*/
/*----------------------------------------------------------------------------*/
int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          Stub for set position in file

*/
/*----------------------------------------------------------------------------*/
int _lseek(int fd, int ptr, int dir)
{
    (void)fd;
    (void)ptr;
    (void)dir;
    return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 @brief          Stub for is a terminal

*/
/*----------------------------------------------------------------------------*/
int _isatty(int fd)
{
    (void)fd;
    return 1;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    return -1;
}
