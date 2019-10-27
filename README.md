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
핀토스는 아래와 같은 구현순서를 추천하고 있다. (아마 이 순서로 진행해야 수월하게 과제 완료할 수 있는듯)
 - Argument passing
 - User memory access (upto "System Call" problem)
 - System call infrastructure (upto "System Call" problem)
 - The ```exit``` system call (upto "System Call" problem)
 - The ```write``` system call for writing to fd 1, the system console (upto "System Call" problem)
 - Change ```process_wait()``` to an infinite loop. (upto "System Call"? I don't know yet)

이 녀석들을 차근차근 살펴보자.
-----------------------------------

### 1. Argument Passing (문제정의)
 > 현재 ```process_execute()```는 새로운 프로세스에 대해 passing arguments를 지원하지 않고 있다.
 이 함수를 확장하여, 프로그램 파일 이름을 argument로 사용하는 대신 공백으로 단어로 나누는 기능을 구현해라.
 첫 번째 단어는 프로그램 이름이고 두 번째 단어는 첫 번째 argument이다. 즉,
 ```process_execute ("grep foo bar")``` 는 두 개의 arguments ```foo``` 및 ```bar```를 전달하여 ```grep```을 실행해야 한다.
 
 > command line 에서 여러 공백은 단일 공백과 동일하므로 ```process_execute ("grep foo bar")``` 는 원래 예제와 동일하다.
 command line argument의 길이에 제한을 둘 수 있다. 가령 argument를 단일 페이지(4kB)에 맞는 argument로.
 (핀토스 유틸리티가 커널에 전달할 수 있는 명령행 인수에는 128바이트 관련 제한이 없긴 하다).
 
 > 내가 원하는 방식으로 argument strings를 parse 할 수 있다. 감이 안 잡힌다면, 
 "lib/string.h" 에 프로토타입이 있는 ```strtok_r()``` 을 살펴보십시오.
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
  
  유저프로그램의 실행을 위해 ```file_name```이 전달되는 큰 그림은 아래와 같다.
  
  > a. 프로그램 실행 --> ```process_execute()``` 안에서 ```thread_create()``` 및 ```start_process()```.
  
  > b. ```start_process (void *file_name_)``` 안에서 유저프로그램 ```load(file_name, &if_.eip, &if_.esp)```.
  
  >> ```eip```는 실행할 명령의 주소, ```esp```는 현재 진행하는 함수의 제일 아래부분의 스택포인터이다.
  
  >> 여기에서 파일이 실행가능하다면, ```intr_exit```(in "threads/intr-stubs.S") 인터럽트로부터의 리턴을 시뮬레이션(?)하여 유저프로세스를 실행함. 이 ```intr_exit```은 스택의 모든 arguments를 ```struct intr_frame``` 형식으로 가져오기 때문에, 우리는 스택포인터(```&esp```)를 우리의 스택프레임으로 가리키게 한 다음에 그것을 점프한다(?).
  
  > c. ```load (const char *file_name, void (**eip) (void), void **esp)``` 안에서 ```setup_stack (esp)```를 통해 스택포인터, 즉 스택을 초기화(이를 통해 esp는 ```PHYS_BASE```로 초기화됨).
  
  #### Argument Passing 구현계획
  > a. ```process_execute()```에서 ```thread_create()```를 호출하기 전 파일 이름을 ```strtok_r```을 통해 토큰화한 뒤, 이를 ```thread_create()```의 첫 번째 인자로 넣어준다.
  
  >> ```process_execute()```에서 ```thread_create()```를 호출할 때 ```start_process (void *file_name_)```이 호출된다(이 때 ```file_name```은 ```thread_create()```에 argument로 전달된 토큰이 아닌 ```process_execute()```의 argument로 전달된 ```file_name```이다). 이후 ```start_process()``` 내에서 ```load (file_name, &if_.eip, &if_.esp)``` 를 호출하는데, 이 때 argument로 전달되는 ```file_name```은 역시 토큰이 아닌 그냥 생짜 ```file_name```이다.
  
  > b. ```load (const char *file_name, void (**eip) (void), void **esp)``` 에서 실질적으로 파일을 여는데,이 때 전달된 ```file_name```을 ```file_name```의 첫번째 토큰으로 변경해야 한다. (```file = filesys_open (file_name)```에서의 ```file_name```을 토큰의 제일 첫번째 토큰으로 변경)
  
  >> ```load ()``` 내에서 호출되는 ```set_up (esp)``` 를 변경해야 한다. ```static bool setup_stack (void **esp)```를 보면, 스택을 위한 페이지를 할당받는 게 성공하면 argument로 받은 ```esp```가 ```PHYS_BASE```로 초기화되는 것을 확인할 수 있다.
  
  >> ###### 1.1. Program Startup Details 에 언급했듯, 우리는 command의 각 단어의 주소를 ```argv```에 넣어 이 ```argv```와 null pointer sentinel를 스택에 넣어야 한다. 이렇게 하기 위해서는, ```set_up ()```를 호출하는 ```load ()```에서 ```file_name```을 토대로 ```char **argv``` 및 ```int argc``` 를 초기화 한 뒤, ```esp```와 함께 이 둘도 ```set_up ()```의 argument로 보내주어야 한다.
  
  >>> ```load ()``` 에서 ```strtok_r()```을 이용하여 각 단어를 ```argv[argc++]```에 넣는다.
  
  >>> ```file = filesys_open (file_name)```을 ```file = filesys_open (argv[0])``` 으로 변경한다.
  
  >>> ```setup_stack ()``` 에 세 개의 argument (```void **esp, char** argv, int argc```) 가 전달되도록 변경한다.
  
  >>> ```load ()``` 에서 ```setup_stack ()``` 호출문의 argument를 위 세 개로 변경한다.
  
  >> ```setup_stack ()``` 내에서, argument로 받은 ```argv, argc```를 스택에 push한다.
  
  >> 스택은 다음의 꼬라지와 같다.
  
  >>> <div><img width="1000" src="https://user-images.githubusercontent.com/49421142/66751949-37982380-eecb-11e9-9764-78b9f73561af.png">
 
  >>> (```word-align``` 은 접근속도를 빠르게 하기 위해 4의 배수로 맞추기 위해 추가하는 녀석이라고 한다. 위 사진처럼 하자.)
  
  >>> 스택의 push 방법은, ```*esp```를 스택의 원하는 위치로 조정한 뒤 ```*esp```에 넣고자 하는 녀석을 대입해주면 된다.
  
  >>> 가령, 초기 스택포인터를 초기화하는 것은 ```*esp = PHYS_BASE;``` 이다. 이후 차례로 넣고자하는 녀석의 크기만큼 스택을 확장해준뒤(가령 ```argv[i]```에 있는 한 단어를 넣고자 한다면, 하나의 문자는 1바이트이므로 확장해야 하는 스택의 사이즈는 ```argv[i]```의 사이즈가 되며, 이는 ```strlen[i]+1``` 이다(```'\0'```도 포함). 즉 ```*esp -= (strlen(argv[i]))+1 ```을 이용하여 스택 확장), 주소를 넣고자 한다면 ```*(int*)*esp = argc;```, 내용을 복사하여 넣고 싶다면 ```memcpy(*esp, argv[i], strlen(argv[i])+1);``` 처럼 한다.
  
  >>> return address의 크기는 4이며, ```*(int*)*esp = 0;``` 이다.


### 2. User Memory Access (문제정의)
> 모든 시스템콜은 유저메모리를 read하는 게 필요하다. 그 중 몇몇 시스템콜은 유저메모리를 write하는 게 필요하다.

> 커널은 유저프로그램으로부터 제공받는 포인터을 통해 유저메모리에 접근해야 한다. 하지만 그 포인터가 이상한 녀석일 수도 있다. (null이라던가, unmapped virtual memory라던가, 커널 virtual address space를 가리키고 있다거나(above ```PHYS_BASE```)...) 이러한 포인터들은, 해당 프로세스를 종료하고 자원을 회수함으로써 거절되어야 한다.

> 위 문제를 해결하기 위한 방법은 두 가지가 있는데, 첫 번째는 유저로부터 제공된 포인터가 타당한지 검증하는 방법이고("userprog/pagedir.c"와 "threads/vaddr.h"를 봐야 한다), 두 번째 방법은 유저포인터가(스택포인터) ```PHYS_BASE``` 아래에 있는지 확인하는 방법이다("userprog/exception.c"의 ```page_fault()```를 수정해야한다). 일반적으로 두 번째 방법이 빠르다.

> 두 경우 모두 메모리누수가 일어나지 않도록 철저히 확인해야 한다. 어떤 경우에서?

>> example) 시스템콜이 lock이나 힙에 할당된 메모리를 획득한 상황에서, 잘못된 유저포인터와 조우하게 된다면 lock을 release하거나 메모리페이지를 free해야만 한다. 첫 번째 방법으로 포인터를 판단한다면 이 상황은 비교적 간단하게 해결될 수 있다. 하지만 만약 두 번째 방법(```PHYS_BASE```를 확인하는 방법)으로 포인터를 판단한다면 좀 어렵다. 왜냐하면 메모리 접근으로부터 error code를 리턴할 방법이 없기 때문이다. 이를 위해 핀토스는 두 번째 판단방법을 사용하는 사람을 위해 추가적인 code를 제공한다(docs p.27).

"threads/vaddr.h" 에 있는 ```bool is_user_vaddr(const void *vaddr)``` 함수를 이용하면 두 번째 방법을 사용할 수 있다. 이 함수는 인자로 넘겨받은 ```vaddr```이 ```PHYS_BASE```보다 아래에 있으면 true를 리턴한다.

  #### 유저프로그램 실행에 따른 page의 흐름
  1. 유저프로그램이 실제로 실행되기 위해 ```load()``` 함수가 호출되면, ```file_name```으로부터 파일을 열기 전에 스레드를 하나 생성하고, 그 스레드에 ```pagedir_create()```를 통해 페이지를 하나 생성한다.
  2. 스레드가 생성되고 페이지가 할당된 시점에서 ```process_activate()```가 호출되고, 이 함수 안에서 ```pagedir_activate(uint32_t *pd)``` 함수가 호출된다. 이 함수에서는 어셈블리 코드를 통해 pd를 CPU's page directory base register(PDBR)에 넣는다. 이를 통해서 실질적으로 무언가가 되는 듯 하다.
  
"pagedir.h"의 함수들 중에서 ```void *upage```, ```void *kpage```를 인자로 받는 함수들이 있다. 첫 번째 방법에서 유저로부터 제공된 포인터가 타당한지를 검증하는 코드를 이 함수들 안에 짜야 하는 걸까?
  
  ### 그냥 막 해보자
  #### 시스템콜은 어떤식으로 호출되는거죠?
   - "userprog/syscall.c"의 ```static void syscall_handler(struct intr_frame *f UNUSER)```함수에서 인자로 ```intr_frame *f```가 들어온다. 
   - ```intr_frame```구조체는 "thread/interrupt.h" 에 선언되어있다.
   - "lib/user/syscall.c" 를 보면 syscall 번호를 어셈블리어로 호출하하고 있음을 각 함수의 주석을 읽어보면 알 수 있다. 이 때, ```syscall0```, ```syscall1```, ```syscall2```, ```syscall3``` 함수의 ```asm volatile``` 호출 부분을 보면 각각 ```$4```, ```$8```, ```$12```, ```$16```에 ```esp```를 ```add```하는 것 같은 수상적은 낌새를 눈치챌 수 있다.
   - "lib/user/syscall.c" 를 자세히 보면, ```$4```, ```$8``` 의 숫자가 각 함수의 인자의 수와 4의 배수로 매칭되는 것을 알 수 있다. 즉 인자가 없는 ```syscall0(NUMBER)``` 은 syscall의 번호에 해당하는 4바이트(?)만 할당하면 되기에 스택포인터를 4만큼 늘려주는 건가보다.
   - "lib/user/syscall-nr.h" 에 각 syscall이 enum으로 명시되어있다. "lib/user/syscall.c"의 인자인 ```NUMBER```를 이용하여 요 녀석들을 번호로 호출하는 것 같다.
   - 정리해보자면, "userprog/syscall.c" 에서 인자로 받은 ```intr_frame *f```의 데이터 중 시스템콜을 나타내는 숫자가 포함되는 것 같다. 즉 ```esp```를 4만큼 더해준 뒤(스택을 늘려준 뒤) 그 자리로 syscall 어셈블리 함수의 인자가 차례대로 들어오는 듯하다.
   - 즉 "lib/user/syscall-nr.h"에 선언된 시스템콜들의 순서에 맞게 번호에 따라 호출해주는 함수를 모두 구현해야 한다.
   
   #### 시스템콜을 호출하는 함수는 어디에 어떤 식으로 선언해야 하는 거죠?
   - "lib/user/syscall.c" 에서 각 시스템콜 함수들을 위에서 언급한 ```syscall0()```, ```syscall1()``` 등으로 호출하는 것을 볼 수 있다.
   - 우리는 실제로 각 시스템콜 함수를 구현해야 하는데, 그 구현은 "userprog/syscall.c"에 하면 된다. 그리고 각 시스템콜 함수의 호출은 동일한 파일의 ```static void syscall_handler (struct intr_frame *)``` 이 담당한다.
   - 앞서 확인해봤듯, ```intr_frame``` 구조체는 ```esp```를 가지고 있다. 그러므로 이 핸들러의 인자로 들어오는 녀석의 ```esp```의 위치를 확인해줌으로써 유저포인터가 제대로 된 녀석인지 확인하는 것이 아닐까 조심스레 예측해본다.
   
   #### 시스템콜 함수와 어셈블리의 연결고리인 ```NUMBER```는 우리의 ```syscall_handler()```에서 어떻게 매칭되는거죠?
   
-----------------------------------
이하는 그 외 Problem
-----------------------------------

### 1. Process Termination Messages

 - 어떠한 이유로든간에 user program이 종료되면, 해당 프로세스의 이름과 exit_code를 출력하기.
 - 프로세스의 이름은 process_execute() 함수를 거쳐가야 한다(커맨드라인 argument는 생략).
 - 커널 스레드가 종료되거나(이는 유저프로세스 종료가 아님), halt 라는 시스템콜이 호출되면 프린트하지 말기.


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
