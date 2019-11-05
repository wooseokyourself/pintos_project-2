#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/user/syscall.h"

/* This comment is from POSTECH project2.pdf

System call: internal interrupts or software exceptions
Implement the system call handler
    implement your code on syscall.c and syscall.h in userprog folder.
    System call numbers for each system call are defined in lib/syscall-nr.h
    (BELOW!)
        enum 
        {
            // Projects 2 and later.
            SYS_HALT,                   // Halt the operating system.
            SYS_EXIT,                   // Terminate this process. 
            SYS_EXEC,                   // Start another process. 
            SYS_WAIT,                   // Wait for a child process to die. 
            SYS_CREATE,                 // Create a file. 
            SYS_REMOVE,                 // Delete a file. 
            SYS_OPEN,                   // Open a file. 
            SYS_FILESIZE,               // Obtain a file's size. 
            SYS_READ,                   // Read from a file. 
            SYS_WRITE,                  // Write to a file. 
            SYS_SEEK,                   // Change position in a file. 
            SYS_TELL,                   // Report current position in a file. 
            SYS_CLOSE,                  // Close a file. 

            // Project 3 and optionally project 4. 
            SYS_MMAP,                   // Map a file into memory. 
            SYS_MUNMAP,                 // Remove a memory mapping. 

            // Project 4 only.
            SYS_CHDIR,                  // Change the current directory. 
            SYS_MKDIR,                  // Create a directory. 
            SYS_READDIR,                // Reads a directory entry. 
            SYS_ISDIR,                  // Tests if a fd represents a directory. 
            SYS_INUMBER                 // Returns the inode number for a fd. 
        };

*/

struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };

void syscall_init (void);
void halt (void);
void exit (int status);
pid_t exec (const char *cmd_lime);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

#endif /* userprog/syscall.h */
