# Pintos Project2

### 1. Process Termination Messages

 - 어떠한 이유로든간에 user program이 종료되면, 해당 프로세스의 이름과 exit_code를 출력하기.
 - 프로세스의 이름은 process_execute() 함수를 거쳐가야 한다(커맨드라인 argument는 생략).
 - 커널 스레드가 종료되거나(이는 유저프로세스 종료가 아님), halt 라는 시스템콜이 호출되면 프린트하지 말기.

궁금한점 1. 유저프로그램을 돌리는 스레드가 프로젝트 1에서 건드린 스레드와 동일한 스레드인가? 그게 맞다면 그냥 thread_exit() 함수에서 print 코드를 추가하면 되는 것 아닐까?
> 팁에 보면 프로젝트2를 위해서는 "src/userprog" 만 사용하면 된다고 하니 아마 "src/thread" 는 건드릴 필요가 없는듯.
> process.h 에 정의된 함수는 네 개 이다.
<pre><code>
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
</code></pre>
