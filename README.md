# PintOS
#### Difference between ```tid_t``` and ```pid_t``` (docs p.41)
 > ```tid_t```는 커널 스레드에서 User process가 실행중이거나(process_execute()의 경우)
 그렇지 않은 경우(thread_create()의 경우) 커널 스레드를 식별한다.
 즉, 이는 커널에서만 쓰이는 데이터타입이다.
 
 > ```pid_t```는 User process를 식별한다. User process와 커널 안에서
 ```exec``` 혹은 ```wait``` 시스템 콜을 사용하기 위해 사용된다.
 
 > You can choose whatever suitable types you like for tid_t and pid_t.
 By default, they’re both int. You can make them a one-to-one mapping,
 so that the same values in both identify the same process,
 or you can use a more complex mapping. It’s up to you.

# Pintos Project2

### 1. Process Termination Messages

 - 어떠한 이유로든간에 user program이 종료되면, 해당 프로세스의 이름과 exit_code를 출력하기.
 - 프로세스의 이름은 process_execute() 함수를 거쳐가야 한다(커맨드라인 argument는 생략).
 - 커널 스레드가 종료되거나(이는 유저프로세스 종료가 아님), halt 라는 시스템콜이 호출되면 프린트하지 말기.

###### 궁금한점 1. 유저프로그램을 돌리는 스레드가 프로젝트 1에서 건드린 스레드와 동일한 스레드인가? 그게 맞다면 그냥 thread_exit() 함수에서 print 코드를 추가하면 되는 것 아닐까?
> process.h 에 정의된 함수는 네 개 이다.

```C

tid_t process_execute (const char *file_name);
/* 유저프로그램을 실행하는 새로운 스레드를 시작한다. 새로운 스레드는 process_execute()가
   리턴되기 전에 스케쥴링되고 종료된다.
   이 함수는 새로운 프로세스의 스레드 id를 리턴하거나,
   혹은 스레드가 생성되지 못하면 TID_ERROR 를 리턴한다. */
   
int process_wait (tid_t);
/* 스레드의 TID가 종료될때까지 기다리고 그 스레드의 exit status를 리턴한다.
   만약 커널에 의해 종료되었을 경우(ex. exeption) -1 을 리턴한다.
   TID가 유효하지 않거나, calling 프로세스의 자식이 아니거나,
   주어진 TID에 의해 process_wait()이 이미 성공적 으로 호출되었다면
   대기하지않고 즉시 -1 을 리턴한다.
   이 함수는 problem 2-2에서 구현되어야 한다. */

void process_exit (void);
/* 현재 프로세스의 메모리를 release한다(free). */

void process_activate (void);
/* 현재 스레드에서 유저코드를 실행시키기 위해 CPU를 셋팅한다.
   이 함수는 매 context switch마다 호출된다. */

```
