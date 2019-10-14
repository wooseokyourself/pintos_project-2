# Pintos Project2

### 1. Process Termination Messages

 - 어떠한 이유로든간에 user program이 종료되면, 해당 프로세스의 이름과 exit_code를 출력하기.
 - 프로세스의 이름은 process_execute() 함수를 거쳐가야 한다(커맨드라인 argument는 생략).
 - 커널 스레드가 종료되거나(이는 유저프로세스 종료가 아님), halt 라는 시스템콜이 호출되면 프린트하지 말기.

###### 궁금한점 1. 유저프로그램을 돌리는 스레드가 프로젝트 1에서 건드린 스레드와 동일한 스레드인가? 그게 맞다면 그냥 thread_exit() 함수에서 print 코드를 추가하면 되는 것 아닐까?
> process.h 에 정의된 함수는 네 개 이다.
<pre><code>
tid_t process_execute (const char *file_name);
/* 유저프로그램을 실행하는 새로운 스레드를 시작한다. 새로운 스레드는 process_execute()가 리턴되기 전에 스케쥴링되고 종료된다.
   이 함수는 새로운 프로세스의 스레드 id를 리턴하거나, 혹은 스레드가 생성되지 못하면 TID_ERROR 를 리턴한다. */
   
int process_wait (tid_t);
/* 스레드의 TID가 종료될때까지 기다리고 그 스레드의 exit status를 리턴한다. 만약 커널에 의해 종료되었을 경우(ex. exeption)
   -1 을 리턴한다. TID가 유효하지 않거나, calling 프로세스의 자식이 아니거나, 주어진 TID에 의해 process_wait()이 이미 성공적
   으로 호출되었다면 대기하지않고 즉시 -1 을 리턴한다.
   이 함수는 problem 2-2에서 구현되어야 한다. */

void process_exit (void);
void process_activate (void);
</code></pre>
