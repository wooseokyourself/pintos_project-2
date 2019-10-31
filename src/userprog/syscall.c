#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
printf("syscall_init START!\n");
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
printf("syscall_init END!\n");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  switch (*uint32_t *)(f->esp)
  {
    case SYS_HALT:                   // args number: 0
      halt ();
      break;

    case SYS_EXIT:                   // args number: 1
      exit( *(uint32_t *)(f->esp + 4) );
      break;

    case SYS_EXEC:                   // args number: 1
      exec ();
      break;

    case SYS_WAIT:                   // args number: 1
      wait ();
      break;

    case SYS_CREATE:                 // args number: 2
      create ();
      break;

    case SYS_REMOVE:                 // args number: 1
      remove ();
      break;

    case SYS_OPEN:                   // args number: 1
      open ();
      break;

    case SYS_FILESIZE:               // args number: 1
      filesize ( (int)*(uint32_t *)(f->esp+4) );
      break;

    case SYS_READ:                   // args number: 3
      read ( (int)*(uint32_t *)(f->esp+4) );
      break;

    case SYS_WRITE:                  // args number: 3
      write( (int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)) );
      break;

    case SYS_SEEK:                   // args number: 2
      seek ( (int)*(uint32_t *)(f->esp+4) );
      break;

    case SYS_TELL:                   // args number: 1
      tell ( (int)*(uint32_t *)(f->esp+4) );
      break;

    case SYS_CLOSE:                  // args number: 1
      close ( (int)*(uint32_t *)(f->esp+4) );
      break;
  }
  thread_exit ();
}

void
halt (void)
{
  shutdown_power_off ();
}

void
exit (int status)
{
  struct thread *current = thread_current();
  if (current->status == status)
  {
printf(" SYSCALL: exit invoking process_exit()! \n");
    current->status = THREAD_DYING;
    current->isRun = false;
    process_exit ();
  }
  else
  {
printf(" SYSCALL: exit failed!\n");
  }
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
printf(" SYSCALL: wait \n");
  return process_wait (pid);
}

bool
create(const char *file, unsigned initial_size)
{
  filesys_create (file, initial_size);
}

bool
remove (const char *file)
{
  filesys_remove (file);
}

int
open (const char *file)
{
printf(" SYSCALL: open \n");
  struct file *return_file = filesys_open(file);
  if (return_file == NULL)
  {
printf("  >> filesys_open(file) failed, return -1\n");
    return -1;
  }
  else
  {
printf("  >> filesys_open(file) success, return fd(%d)", file_open_count);
    return_file->fd = file_open_count++;
    list_push_back (&opened_file_list, return_file->list_elem);
    return return_file->fd;
  }
}

int
filesize (int fd)
{
  struct file *f = getfile (fd);
  if (f == NULL)
    return -1;
  else
    return f->inode->data->length;
}

int
read (int fd, void *buffer, unsigned size)
{
  if (fd == 0)
  {
    /* input_getc() 를 이용해 키보드 입력을 버퍼에 넣는다. 그리고 입력된 사이즈(bytes)를 리턴한다. */
    return -1; // temp
  }
  else
  {
    struct file *f = getfile (fd);
    if (f == NULL)
      return -1;
    else
      return file_read (f, buffer, size);
  }
}


int
write (int fd, const void *buffer, unsigned size) // 이거 내용 부정확하니까 docs 보고 다시 짜기!!
{
  if (fd == 0)
  {
    /* putbuf() 함수를 이용하여 버퍼의 내용을 콘솔에 입력한다. 이 때에는 필요한 사이즈만큼 반복문을 돌아야 한다. */
    return -1; // temp
  }
  else
  {
    struct file *f = getfile (fd);
    if (f == NULL)
      return -1;
    else
      return file_write (f, buffer, size);
  }
}

void
seek (int fd, unsigned position)
{
  struct file *f = getfile (fd);
  if (f == NULL)
    return -1;
  else
    return file_seek (f, position);
}

unsigned
tell (int fd)
{
  struct file *f = getfile (fd);
  if (f == NULL)
    return -1;
  else
    return file_tell (f);
}

void
close (int fd)
{
  struct file *f = getfile (fd);
  if (f == NULL)
    return -1;
  else
    file_close (f);
}

struct file
*getfile (int fd)
{
  struct list_elem *e;
  struct list *files = &opened_file_list;
  bool found = false;
  for (e = list_begin (files); e != list_end (files); e = list_next (e))
  {
    struct file *f = list_entry (e, struct file, elem);
    if (f->fd == fd)
    {
      return f;
    }
  }
  return NULL;
}