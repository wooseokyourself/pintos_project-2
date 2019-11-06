#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/off_t.h"
#include "devices/block.h"

/* These are defined in threads/thread.c */
/*
extern struct list opened_file_list;
extern int file_open_count;
*/

// struct lock file_lock; // original is in thread.c

struct file *getfile (int fd);
static void syscall_handler (struct intr_frame *f);
void check_user_vaddr (const void *vaddr);

void
syscall_init (void) 
{
  // lock_init (&file_lock);
//printf("syscall_init START!\n");
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
//printf("syscall_init END!\n");
}

void
syscall_handler (struct intr_frame *f) 
{
  /*
    인자가 1개인 녀석은 esp + 4 부터 시작하고, 2개 이상은 녀석은 esp + 20 부터 시작한다.
    리턴값이 있는 함수는, 그 리턴값을 eax에 넣어주어야 한다. (docs p.36)
  */
//printf ("system call!\n");
//printf("syscall: %d\n", *(uint32_t *)(f->esp));
//hex_dump (f->esp, f->esp, 100, 1);
  void *sp = f->esp;

  switch (*(uint32_t *)sp)
  {
    case SYS_HALT:                   // args number: 0
      halt ();
      break;

    case SYS_EXIT:                   // args number: 1
      check_user_vaddr (sp + 4);
      exit( *(uint32_t *)(sp + 4) );
      break;

    case SYS_EXEC:                   // args number: 1
      check_user_vaddr (sp + 4);
      f->eax = exec ( (const char *)*(uint32_t *)(sp + 4) );
      break;

    case SYS_WAIT:                   // args number: 1
      check_user_vaddr (sp + 4);
      f->eax = wait ( (pid_t *)*(uint32_t *)(sp + 4) );
      break;

    case SYS_CREATE:                 // args number: 2
      check_user_vaddr (sp + 4);
      f->eax = create ( (const char *)*(uint32_t *)(sp + 4),  (const char *)*(uint32_t *)(sp + 8) );
      break;

    case SYS_REMOVE:                 // args number: 1
      check_user_vaddr (sp + 4);
      f->eax = remove ( (const char *)*(uint32_t *)(sp + 4) );
      break;

    case SYS_OPEN:                   // args number: 1
      check_user_vaddr (sp + 4);
      f->eax = open ( (const char *)*(uint32_t *)(sp + 4) );
      break;

    case SYS_FILESIZE:               // args number: 1
      check_user_vaddr (sp + 4);
      f->eax = filesize ( (int)*(uint32_t *)(sp + 4) );
      break;

    case SYS_READ:                   // args number: 3
      check_user_vaddr (sp + 4);
      f->eax = read ( (int)*(uint32_t *)(sp + 4), (void *)*(uint32_t *)(sp + 8), (unsigned)*((uint32_t *)(sp + 12)) );
      break;

    case SYS_WRITE:                  // args number: 3
      check_user_vaddr (sp + 4);
      f->eax = write( (int)*(uint32_t *)(sp + 4), (void *)*(uint32_t *)(sp + 8), (unsigned)*((uint32_t *)(sp + 12)) );
      break;

    case SYS_SEEK:                   // args number: 2
      check_user_vaddr (sp + 4);
      seek ( (int)*(uint32_t *)(sp + 4), (unsigned)*((uint32_t *)(sp + 8)) );
      break;

    case SYS_TELL:                   // args number: 1
      check_user_vaddr (sp + 4);
      f->eax = tell ( (int)*(uint32_t *)(sp + 4) );
      break;

    case SYS_CLOSE:                  // args number: 1
      check_user_vaddr (sp + 4);
      close ( (int)*(uint32_t *)(sp + 4) );
      break;
  }
  // thread_exit ();
}

void
halt (void)
{
  shutdown_power_off ();
}

void
exit (int status)
{
  printf("%s: exit(%d)\n", thread_name(), status);
  // thread_current() -> status = THREAD_DYING; /* 이는 thread_exit() 내에서 처리됨 */
  thread_current() -> exit_code = status;
  for (int i=3; i<128; i++) 
  {
    if (getfile(i) != NULL)
      close(i);
  }
  thread_exit();
}

pid_t
exec (const char *cmd_lime)
{
  tid_t result = process_execute();
  if (result = TID_ERROR)
    return -1;
  else
    return result;
}

int
wait (pid_t pid)
{
//printf(" SYSCALL: wait \n");
  return process_wait (pid);
}

bool
create(const char *file, unsigned initial_size)
{
  if (file == NULL)
    exit(-1);
  return filesys_create (file, initial_size);
}

bool
remove (const char *file)
{
  if (file == NULL)
    exit(-1);
  return filesys_remove (file);
}

int
open (const char *file)
{
//printf(" SYSCALL: open \n");
  if (file == NULL)
    exit(-1);
  check_user_vaddr (file);
  // lock_acquire (&file_lock);
  struct file *return_file = filesys_open (file);
  if (return_file == NULL)
    return -1;
  else
  {
    for (int i=3; i<128; i++)
    {
      if (getfile(i) == NULL)
      {
        if (strcmp (thread_current()->name, file) == false)
          file_deny_write (return_file);

        thread_current()->fd[i] = return_file;
//printf("  >> filesys_open(file) success, return %d, idx of fd", i);
        // lock_release (&file_lock);
        return i;
      }
    }
//printf("  >> filesys_open(file) failed ; thread's fd is full, return -1\n");
  }
  // lock_release (&file_lock);
  return -1;
}

int
filesize (int fd)
{
  struct file *f = getfile (fd);
  if (f == NULL)
    exit(-1);
  else
    return file_length (f);
}

int
read (int fd, void *buffer, unsigned size)
{
  check_user_vaddr (buffer);
  // lock_acquire (&file_lock);
  if (fd == 0)
  {
    /* input_getc() 를 이용해 키보드 입력을 버퍼에 넣는다. 그리고 입력된 사이즈(bytes)를 리턴한다. */
    int i;
    for (i=0; i<size; i++)
    {
      if ( ( (char *)buffer)[i] == '\0')
        break;
    }
    // lock_release (&file_lock);
    return i;
  }
  else
  {
    struct file *f = getfile (fd);
    if (f == NULL)
      exit(-1);
    else
    {
      // lock_release (&file_lock);
      return file_read (f, buffer, size);
    }
  }
}


int
write (int fd, const void *buffer, unsigned size) // 이거 내용 부정확하니까 docs 보고 다시 짜기!!
{
  check_user_vaddr (buffer);
  // lock_acquire (&file_lock);
  if (fd == 1)
  {
    /* putbuf() 함수를 이용하여 버퍼의 내용을 콘솔에 입력한다. 이 때에는 필요한 사이즈만큼 반복문을 돌아야 한다. */
    putbuf (buffer, size);
    return size;
  }
  else
  {
    struct file *f = getfile (fd);
    if (f == NULL)
    {
      // lock_release (&file_lock);
      exit(-1);
    }
    if (f->deny_write)
    {
      file_deny_write (f);
    }
    // lock_release (&file_lock);
    return file_write (f, buffer, size);
  }
}

void
seek (int fd, unsigned position)
{
  struct file *f = getfile (fd);
  if (f == NULL)
    exit(-1);
  else
    return file_seek (f, position);
}

unsigned
tell (int fd)
{
  struct file *f = getfile (fd);
  if (f == NULL)
    exit(-1);
  else
    return file_tell (f);
}

void
close (int fd)
{
  struct file *f = getfile (fd);
  if (f == NULL)
    exit(-1);
  else
  {
    f = NULL;
    file_close (f);
  }
}

struct file
*getfile (int fd)
{
  return (thread_current()->fd[fd]);
}

void
check_user_vaddr (const void *vaddr)
{
  ASSERT(is_user_vaddr(vaddr));
}
