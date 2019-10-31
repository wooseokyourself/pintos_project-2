#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

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
      break;

    case SYS_CREATE:                 // args number: 2
      break;
    case SYS_REMOVE:                 // args number: 1
      break;
    case SYS_OPEN:                   // args number: 1
      break;
    case SYS_FILESIZE:               // args number: 1
      break;
    case SYS_READ:                   // args number: 3
      break;
    case SYS_WRITE:                  // args number: 3
      write( (int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)) );
      break;
    case SYS_SEEK:                   // args number: 2
      break;
    case SYS_TELL:                   // args number: 1
      break;
    case SYS_CLOSE:                  // args number: 1
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
  status = THREAD_DYING;
  
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


/*
  자식프로세스(pid로 식별, 즉 인자가 자식프로세스이다)를 기다리고, 자식의 exit status를 검사한다.
  이를 구현하기 위해, 현재 프로세스(스레드)는 "thread/synch.h"에 정의된
  void cond_wait (struct condition *, struct lock *)를 통해 자식프로세스의 종료를 기다리고,
  반대로 자식프로세스는 void cond_signal (struct condition *, struct lock *)
  를 통해 종료를 알리게 한다.
  pintos docs를 보면 자식프로세스가 정상적으로 종료되지 않고 커널에 의해 종료되는 상황을 구분해두었는데,
  이 때 이 함수는 -1을 리턴해야 한다.
  또한 pid가 현재 이 함수를 호출한 스레드의 자식이 아닐 경우에도 바로 -1을 리턴해야한다.
  참고로 스레드는 상속관계로 wait 할 수 없다. 즉, 자식의 자식을 인자로 wait을 호출할 수 없다.
  또한, 이미 동일한 자식을 인자로 wait을 호출했을 경우에도 즉시 -1을 리턴한다.
*/
int
wait (pid_t pid)
{
printf(" SYSCALL: wait \n");
  return process_wait (pid);
}

bool
create(const char *file, unsigned initial_size)
{

}

bool
remove (const char *file)
{

}

int
open (const char *file)
{

}

int
filesize (int fd)
{

}

int
read (int fd, void *buffer, unsigned size)
{

}


int
write (int fd, const void *buffer, unsigned size)
{

}

void
seek (int fd, unsigned position)
{

}

unsigned
tell (int fd)
{

}

void
close (int fd)
{

}