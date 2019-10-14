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

----------------------------------------

# Pintos Project2
핀토스는 아래와 같은 구현순서를 추천하고 있다.
 - Argument passing
 - User memory access
 - System call infrastructure
 - The ```exit``` system call
 - The ```write``` system call for writing to fd 1, the system console
 - Change ```process_wait()``` to an infinite loop.

이 녀석들을 차근차근 살펴보자.
### 1. Argument Passing
 > 현재 ```process_execute()```는 새로운 프로세스에 대해 passing arguments를 지원하지 않고 있다.
 이 함수를 확장하여, 프로그램 파일 이름을 argument로 사용하는 대신 공백으로 단어로 나누는 기능을 구현해라.
 첫 번째 단어는 프로그램 이름이고 두 번째 단어는 첫 번째 argument이다. 즉,
 ```process_execute ("grep foo bar")``` 는 두 개의 arguments ```foo``` 및 ```bar```를 전달하여 ```grep```을 실행해야 한다.
 
 > command line 에서 여러 공백은 단일 공백과 동일하므로 ```process_execute ("grep foo bar")``` 는 원래 예제와 동일하다.
 command line argument의 길이에 제한을 둘 수 있다. 가령 argument를 단일 페이지(4kB)에 맞는 argument로.
 (핀토스 유틸리티가 커널에 전달할 수 있는 명령행 인수에는 128바이트 관련 제한이 없긴 하다).
 
 > 내가 원하는 방식으로 argument strings를 parse 할 수 있다. 감이 안 잡힌다면, 
 "lib/string.h" 에 프로토타입이 있는 ```strtoc_r()``` 을 살펴보십시오.
 매뉴얼을 보면 자세한 내용을 알 수 있습니다(프롬프트에서 man strtok_r 실행).
```C
char* strtok_r (char *s, const char *delimiters, char **save_ptr)
 /* s는 분리하고자 하는 문자열, delimiters는 구분자(무엇을 기준으로 분리할것인가). 여기에서 분리자를 공백으로 줘야 한다.
    save_ptr은 함수 내에서 토큰이 추출된 뒤 남은 녀석을 가리키기 위한 것이다. 
    즉 strtok_r의 리턴은 s의 가장 앞에 있는 녀석이고, 이후 두번째 녀석에 접근하고 싶다면 두 번째 strtok 호출 전 s = save_ptr 해줘야 한다.
    사용에 대한 자세한 예제는 https://codeday.me/ko/qa/20190508/495336.html 참조. */
 ```
 
 
 #### 1.1. Program Startup Details (docs p.36)
 ```'/bin/ls -l foo bar'```라는 command는 어떻게 다뤄지는가?
  1. command 가 단어로 쪼개진다. ```'/bin/ls'```, ```'-l'```, ```'foo'```, ```'bar'```.
  2. 위 단어들을 스택의 가장 위에 넣는다. (포인터로 참조되므로 각 단어의 순서는 중요하지 않다.)
  3. 각 단어(string)의 주소와 null pointer sentinel 을 스택에 push한다. (right-to-left 순서로)
   > 이 녀석들은 모두 ```argv``` 의 elements 다. null pointer sentinel은 ```argv[argc]```가 널포인터일 경우를 대비한 것이다(C standard).
   > 이 순서는 ```argv[0]```이 가장 낮은 virtual address에 있도록 한다.
   > 첫 번째 push 전에 스택포인터를 4의 배수로 내린다.
  4. ```argv```(argv[0]의 주소)와 ```argc```를 순서대로 push한다.
  5. 가짜 "return address"를 push한다.
   > entry function은 절대 return되지 않지만, 그것의 스택프레임은 다른 프레임 구조와 동일해야한다.
   >> docs p.37의 스택테이블 참조하기
   
 #### 1.2. Stack(User virtual memory) (레이아웃은 docs p.26)
  - virtual address 0 up to ```PHYS_BASE``` (which is defined in "threads/vaddr.h") and defaults to ```0xc0000000```(3GB).
  - Kernel virtual memory가 나머지 virtual address space를 점유한다.
  - 프로세스당 하나를 할당받는다. 커널이 프로세스를 switch하면, 프로세서의 page directory base register를 바꾸면서 user virtual address spaces도 함께 switch한다. (```pagedir_activate()``` in "userprog/pagedir.c")
  - 유저프로그램은 본인의 user virtual memory에만 접근할 수 있다. 나머지는 예외처리된다. 커널은 현재 실행중인 유저프로세스의 virtual memory에도 접근할 수 있다.
  - 유저스택의 사이즈는 고정되어있다. 이는 Project3 에서 확장할 것이다.
  
  #### Argument Passing 구현을 위한 질문
  - 유저프로그램의 실행으로 ```process_execute (const char *file_name)``` 함수가 실행될 때, 스택의 할당과 스택으로 push(fn_copy) 하는 코드는 어디에 있는 건가?
  >> 유저프로그램의 실행을 위해 ```file_name```이 전달되는 큰 그림은 아래와 같다.
  > 프로그램 실행 --> ```process_execute()``` 안에서 ```thread_create()``` 및 ```start_process()```.
  
  > ```start_process (void *file_name_)``` 안에서 유저프로그램 ```load(file_name, &if_.eip, &if_.esp)```.
  >> ```eip```는 실행할 명령의 주소, ```esp```는 현재 진행하는 함수의 제일 아래부분의 스택포인터이다.
  >> 여기에서 파일이 실행가능하다면, ```intr_exit```(in "threads/intr-stubs.S") 인터럽트로부터의 리턴을 시뮬레이션(?)하여 유저프로세스를 실행함. 이 ```intr_exit```은 스택의 모든 arguments를 ```struct intr_frame``` 형식으로 가져오기 때문에, 우리는 스택포인터(```&esp```)를 우리의 스택프레임으로 가리키게 한 다음에 그것을 점프한다(?).
  
  > ```load (const char *file_name, void (**eip) (void), void **esp)``` 안에서 ```setup_stack (esp)```를 통해 스택포인터, 즉 스택을 초기화(이를 통해 esp는 ```PHYS_BASE```로 초기화됨).
  
  #### Argument Passing 구현계획
  > ```process_execute()```에서 ```thread_create()```를 호출하기 전 파일 이름을 ```strtok_r```을 통해 토큰화한 뒤, 이를 ```thread_create()```의 첫 번째 인자로 넣어준다.
  
  >> ```process_execute()```에서 ```thread_create()```를 호출할 때 ```start_process (void *file_name_)```이 호출된다(이 때 ```file_name```은 ```thread_create()```에 argument로 전달된 토큰이 아닌 ```process_execute()```의 argument로 전달된 ```file_name```이다). 이후 ```start_process()``` 내에서 ```load (file_name, &if_.eip, &if_.esp)``` 를 호출하는데, 이 때 argument로 전달되는 ```file_name```은 역시 토큰이 아닌 그냥 생짜 ```file_name```이다.
  
  > ```load (const char *file_name, void (**eip) (void), void **esp)``` 에서 실질적으로 파일을 여는데,이 때 전달된 ```file_name```을 ```file_name```의 첫번째 토큰으로 변경해야 한다. (```file = filesys_open (file_name)```에서의 ```file_name```을 토큰의 제일 첫번째 토큰으로 변경)
  
  >> ```load ()``` 내에서 호출되는 ```set_up (esp)``` 를 변경해야 한다. ```static bool setup_stack (void **esp)```를 보면, 스택을 위한 페이지를 할당받는 게 성공하면 argument로 받은 ```esp```가 ```PHYS_BASE```로 초기화되는 것을 확인할 수 있다.
  
  >> 1.1. Program Startup Details 에 언급했듯, 우리는 command의 각 단어의 주소를 ```argv```에 넣어 이 ```argv```와 null pointer sentinel를 스택에 넣어야 한다. 이렇게 하기 위해서는, ```set_up ()```를 호출하는 ```load ()```에서 ```file_name```을 토대로 ```char *argv []``` 및 ```int argc``` 를 초기화 한 뒤, ```esp```와 함께 이 둘도 ```set_up ()```의 argument로 보내주어야 한다.
  
  >> ```
  
  >> ```argc ``` <-- ```file_name``` 토큰의 수
  
  >> ```argv [0] ```<-- ```file_name``` 토큰의 첫 번째.
  
  >> ...
  
-----------------------------------

### 1. Process Termination Messages

 - 어떠한 이유로든간에 user program이 종료되면, 해당 프로세스의 이름과 exit_code를 출력하기.
 - 프로세스의 이름은 process_execute() 함수를 거쳐가야 한다(커맨드라인 argument는 생략).
 - 커널 스레드가 종료되거나(이는 유저프로세스 종료가 아님), halt 라는 시스템콜이 호출되면 프린트하지 말기.

###### 궁금한점 1. 유저프로그램을 돌리는 스레드가 프로젝트 1에서 건드린 스레드와 동일한 스레드인가? 그게 맞다면 그냥 thread_exit() 함수에서 print 코드를 추가하면 되는 것 아닐까?
> 핀토스는 하나의 프로세스당 하나의 스레드만을 지원한다. 하나의 프로그램 실행은 하나의 프로세스로 보아야 하며,
프로세스 단위로 접근해야한다.
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
> 이 네 개의 큰 흐름을 보자면,
프로세스의 파일 이름은 ```process_execute (const char *file_name)``` 에서 알 수 있다.
그럼 프로세스의 exit_code는 무엇이며, 어떻게 알 수 있을까?
