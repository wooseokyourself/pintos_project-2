#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/off_t.h"
#include "devices/block.h"

/* These are defined in threads/thread.c */
extern struct list opened_file_list;
extern int file_open_count;

struct file *getfile (int fd);
static void syscall_handler (struct intr_frame *f);
void check_user_vaddr (const void *vaddr);

struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
    
    // MYCODE_START
    int fd;                     /* fild_open_count. */
    struct list_elem elem;      /* List element. */
    // MYCODE_END
  };
struct inode_disk
  {
    block_sector_t start;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    uint32_t unused[125];               /* Not used. */
  };
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
  };

void
syscall_init (void) 
{
printf("syscall_init START!\n");
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
printf("syscall_init END!\n");
}

void
syscall_handler (struct intr_frame *f) 
{
  /*
    인자가 1개인 녀석은 esp + 4 부터 시작하고, 2개 이상은 녀석은 esp + 20 부터 시작한다.
    리턴값이 있는 함수는, 그 리턴값을 eax에 넣어주어야 한다. (docs p.36)
  */
  printf ("system call!\n");
printf("syscall: %d\n", *(uint32_t *)(f->esp));
hex_dump (f->esp, f->esp, 100, 1);
  switch (*(uint32_t *)(f->esp))
  {
    case SYS_HALT:                   // args number: 0
      halt ();
      break;

    case SYS_EXIT:                   // args number: 1
      check_user_vaddr (f->esp + 4);
      exit( (int)*(uint32_t *)(f->esp + 4) );
      break;

    case SYS_EXEC:                   // args number: 1
      check_user_vaddr (f->esp + 4);
      f->eax = exec ( (const char *)*(uint32_t *)(f->esp + 4) );
      break;

    case SYS_WAIT:                   // args number: 1
      check_user_vaddr (f->esp + 4);
      f->eax = wait ( (pid_t *)*(uint32_t *)(f->esp + 4) );
      break;

    case SYS_CREATE:                 // args number: 2
      check_user_vaddr (f->esp + 20);
      f->eax = create ( (const char *)*(uint32_t *)(f->esp + 20), (unsigned *)*(uint32_t *)(f->esp + 24) );
      break;

    case SYS_REMOVE:                 // args number: 1
      check_user_vaddr (f->esp + 4);
      f->eax = remove ( (const char *)*(uint32_t *)(f->esp + 4) );
      break;

    case SYS_OPEN:                   // args number: 1
      check_user_vaddr (f->esp + 4);
      f->eax = open ( (const char *)*(uint32_t *)(f->esp + 4) );
      break;

    case SYS_FILESIZE:               // args number: 1
      check_user_vaddr (f->esp + 4);
      f->eax = filesize ( (int)*(uint32_t *)(f->esp + 4) );
      break;

    case SYS_READ:                   // args number: 3
      check_user_vaddr (f->esp + 20);
      f->eax = read ( (int)*(uint32_t *)(f->esp+20), (void *)*(uint32_t *)(f->esp + 24), (unsigned)*((uint32_t *)(f->esp + 28)) );
      break;

    case SYS_WRITE:                  // args number: 3
      check_user_vaddr (f->esp + 20);
      f->eax = write( (int)*(uint32_t *)(f->esp+20), (void *)*(uint32_t *)(f->esp + 24), (unsigned)*((uint32_t *)(f->esp + 28)) );
      break;

    case SYS_SEEK:                   // args number: 2
      check_user_vaddr (f->esp + 20);
      seek ( (int)*(uint32_t *)(f->esp+20), (unsigned)*((uint32_t *)(f->esp + 24)) );
      break;

    case SYS_TELL:                   // args number: 1
      check_user_vaddr (f->esp + 4);
      f->eax = tell ( (int)*(uint32_t *)(f->esp + 4) );
      break;

    case SYS_CLOSE:                  // args number: 1
      check_user_vaddr (f->esp + 4);
      close ( (int)*(uint32_t *)(f->esp + 4) );
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
  thread_current() -> isRun = false;
  thread_current() -> exit_code = status;
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
  struct file *return_file = filesys_open (file);
  if (return_file == NULL)
  {
printf("  >> filesys_open(file) failed, return -1\n");
    return -1;
  }
  else
  {
printf("  >> filesys_open(file) success, return fd(%d)", file_open_count);
    return_file->fd = file_open_count;
    file_open_count++;
    list_push_back (&opened_file_list, &(return_file->elem));
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
    return f->inode->data.length;
}

int
read (int fd, void *buffer, unsigned size)
{
  if (fd == 0)
  {
    /* input_getc() 를 이용해 키보드 입력을 버퍼에 넣는다. 그리고 입력된 사이즈(bytes)를 리턴한다. */
    int i;
    for (i=0; i<size; i++)
    {
      if ( ( (char *)buffer)[i] == '\0')
        break;
    }
    return i;
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

void
check_user_vaddr (const void *vaddr)
{
  ASSERT(is_user_vaddr(vaddr));
}
