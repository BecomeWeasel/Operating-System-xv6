운영체제 과제 2(implementing simple schedulers on xv6)
---
---
### 테스트 환경 :
##### OS : Ubuntu 16.04
##### gcc : gcc 5.4.0

### 개요 :
운영체제 두번째 과제인 **Implementing simple schedulers (FCFS,MLFQ)**에 대한 내용입니다.

크게 **FCFS** 정책과 **MLFQ** 정책을 사용하게끔 분기됩니다.

---

## FCFS

### 과제 명세 :
먼저 **FCFS** 스케쥴링의 명세는 다음과 같습니다.
![image](/uploads/9376767e42f0679e907b477b13881137/image.png)

1. 먼저 생성(fork())된 프로세스가 먼저 스케줄링 되어야 한다.
2. 스케줄링된 프로세스는 종료되기 전까지는 swithc-out 되지 않는다.
3. 프로세스가 스케쥴링 된 이후 100ticks이 지날때까지 종료되거나 sleeping 하지 않으면 종료해야한다.
4. 실행중인 프로세스가 sleeping으로 전환되면 다음 프로세스가 스케줄링된다.
5. sleeping 상태이면서 먼저 생성된 P가 깨어나면 그 프로세스로 스케줄링 된다.

### 작동과정 설명 :
첫번째로 **FCFS** 스케쥴링 정책 내에서의 작동 과정,설정값들과 예시입니다.

먼저, **ptable**에는 **pid** 순서대로(**만들어진 순서대로**) 삽입되어 있습니다.
![image](/uploads/884497de7c33bd122c0b64d26c90f137/image.png)

그렇기 때문에 **ptable**에서 ptable.proc[0,1,2,3....]이렇게 접근을 한다면 생성된 프로세스에 접근을 할 수 있습니다.

또, **pid** 순서대로 들어있기 때문에 별도의 큰 조작 없이도 **First Come First Served**의 스케쥴링이 수행가능합니다. (1번 명세)

하지만 기존의 정의되어 있는 **struct proc**에는 이 Process가 얼마만큼의 시간동안 수행되었는지에 대한 정보를 담고있지 않기 때문에
![image](/uploads/5b2ccefaaed7ef774667e8c12e06c734/image.png)
위와 같이 **uint ctime**과 **uint stime**을 추가해줍니다.
**ctime**의 역할은 프로세스가 생성된 시간을 의미하고, 

*(사실 pid가 이 역할을 대체할 수 있고 더 정확합니다.)*

**stime**은 스케쥴러에 의해 이 Process가 선택된 시간(**ticks**)을 의미합니다.
![image](/uploads/00a6b066762c33ded7fa12b22e20e920/image.png)
위의 사진처럼 **FCFS** 알고리즘에 의해서 선택되었을때 , **stime**을 현재의 **ticks**로 설정해줍니다.

**FCFS** 알고리즘의 역할은 **가장 이전에 생성된,다시 말해서 pid가 가장 작고**, **현재 수행가능한 상태(RUNNABLE)**인 

Process를 고르는 것입니다..

그렇기 때문에  아래와 같은 code block으로 조건을 만족하는 적합한 Process를 선택합니다.
```
for(p=ptable.proc;p<&ptable.proc[NPROC];p++){  // ptable.proc을 앞에서부터 순차적으로 탐색함, NPROC까지 탐색함
if(p->state!=RUNNABLE)
     continue;
else{
     p->stime=ticks;
     ...
     p->state=RUNNING
    ...
     swtch(&(c->scheduelr),p->context);
    ...
}
```
SLPEEING이 끝나고 wake 하게 될때도 **ptable**을 순차적으로  앞에서부터 탐색하기 때문에
**pid가 낮은,먼저 생성된** Process를 처리할 수 있습니다.

또, **실행된지 100ticks**가 넘어갔을때 종료하는 조건은 trap.c 내에
![image](/uploads/b63c63d6710bfafd031e6c31ebfb46cf/image.png)
로 구성되어 있습니다. **현재 tick**에서 **stime**을 뺀 결과가 100보다 크거나 같다는 뜻은,

이 Process가 100tick이상 run되었다는 뜻입니다.***(항상 스케쥴될때 stime=0으로 현재 ticks으로 초기화)*** 

그렇기 때문에 이 Process의 killed을 1로 바꿔주고, 이렇게 되면 다음 timer때 이 프로세스는 죽습니다.

---
### FCFS Test  결과와 간단한 설명
**조건 : NUM_CHILD를 5에서 7로 수정,**

 첫번째 테스트 **(sleep이나 yield를 하지 않고)**
![image](/uploads/26c4e44ff3c7c9e1032ab8f0c319c246/image.png)
예상대로, sleep,yield 아무것도 하지 않았으니 먼저 생성되는 P5부터 P11까지 출력을 순차적으로 진행합니다.

두번째 테스트 **(yield를 할때)**
![image](/uploads/d320b1efb3467ccd89b3cb52d1574e49/image.png)
예상대로, yeild를 하더라도 먼저 생성된 프로세스에게 우선권이 있으므로 다시 CPU 사용권한이 돌아와 진행합니다.

세번째 테스트 **(sleep)**을 할때
![image](/uploads/a91f11df912e082c6ac0713405d9217f/image.png)
sleep을 하면 순차적으로 진행되는것처럼 보이지만 , 
...P25->P19->P20->P21->P22->P23->P24->**P19** 이부분에서 

P24의 다음으로 P25가 스케쥴링되는것이 아니라 wakeup한 P19가 선택되어 진행됩니다. 

이러한 현상은 꾸준히 관찰할수 있습니다만, 테스팅되는 하드웨어에 따라 상황이 달라짐을 확인했습니다.

네번째 테스트 **100tick을 초과했을때**
![image](/uploads/0e06336891b53083ccef92a099c4b318/image.png)
예상대로 , **pid**가 가장 작은 프로세스가 실행되다가 100tick이 지나면 강제종료되고,

다음 프로세스가 스케줄링됨을 확인할수 있습니다. 또 모든 자식 프로세스가 강제 종료되면 OK메세지를 확인가능합니다.

---
### 트러블슈팅
**FCFS**는 비교적 간단한 구현이어서 **FCFS** 그 자체는 크게 어렵지는 않았습니다. 
다만 xv6의 내부적인 기능을 처음 구현하다보니 sched 함수나 scheduler 함수와 같은 중추적인 함수들을 분석하는데 시간을 쏟았습니다.

또 trap.c 함수 내에서의 trap 함수가 어떻게 사용되는지 분석하는데 시간을 투자했습니다..

---

## MLFQ (Multi Level Feedback Queue)

### 과제 명세 :
먼저 **MLFQ** 스케쥴링의 명세는 다음과 같습니다.
![image](/uploads/5fe7300fb417744603dc91b4369ece6b/image.png)
![image](/uploads/20b2cad85cfe658d7fad59fc4b8a77c3/image.png)

### 작동 과정 설명:
**MLFQ** 구현의 가장 중요한 점은 **L0 큐**와 **L1 큐** 각각의 스케쥴링 알고리즘이 같은 부분이 존재하지만 몇가지 점이 다르다는 것입니다.

**L0 큐**는 기본적으로 **Round robin(Q=4ticks)**이고, **L1 큐**는 **Round robin(Q=8tciks)**에 **priority**가 고려대상이라는 것입니다.

먼저 **ptable**의 구조는 이전과 동일합니다. 차이점은 proc.h에 정의되어 있는 proc 구조체입니다.
![image](/uploads/e26551eaee0e952c883a23d9e95cb2d5/image.png)
속해있는 Q level 을 드러내는 **int lev**, 우선순위를 나타내는 **int priority**, 작동한 시간을 의미하는 **int rtime**(0부터 시작), 

이 Process가 CPU를 독점하는지 체크하는 **int monopolize** ( 1은 독점중,0은 독점중이 아님)등이 추가되었습니다.

먼저 proc.c 안에 정의된 **allocproc** 함수에서 
![image](/uploads/e69b06dc23d124a6b17d9a54b36b2d2a/image.png)
와 같은 코드로, 초기에 **priority**를 0으로, 초기 **lev**을 0으로, **monopolize** 값을 0으로 초기화해줍니다.

이를 통해 처음 실행되는 프로세스의 우선순위를 0으로 , 그리고 가장 높은 레벨의 큐(L0)로 삽입됩니다.




그 다음으로 스케쥴링을 담당하는 **scheduler**함수입니다.

첫번째 사진은 **RUNNABLE**한 프로세스 중 **L0큐**에 속해있는 프로세스의 존재를 측정하고,

있다면 **L0큐**를 탐색하여 적절한 Process를 찾는 부분입니다.
![image](/uploads/eab780101427383ea1576f1450658d60/스크린샷_2019-04-28_21.05.37)
먼저 **L0 큐 **내에 존재하는한 Process를 개수를 
**적절한 프로세스**는 다음과 같이 찾을수 있습니다.
1. 프로세스가 **구동가능한 상태인가?** ***(RUNNABLE)***
2. 프로세스의 **큐 레벨이 0**인가?

위 두가지 조건을 모두 만족해야 **적절한 프로세스**라고 할수 있습니다.
적절한 프로세스를 찾았다면, **rtime**을 0으로 초기화해주고 **context swithcing**이 일어납니다.

두번째 사진은 **L0 큐**에 적절한 Process가 없을때 **L1 큐**를 탐색하고 가장 높은 Priority 를 가지는 Process를 선택하는 부분입니다.
![image](/uploads/468737b0055f688578e7701ea0d42f92/image.png)
프로세스를 **순차적으로 돌면서** **L1 큐**에 있는 것중에서 **Priority가 가장 높은 프로세스를 선택**합니다.
그 후 그 프로세스로 **context swithcing**이 일어납니다.
priority 우선순위가 같다면 , ptable을  시작에서부터 순차적으로 탐색하기때문에 **FCFS**로 행동합니다.

---
#### **MLFQ** 구조는 trap.c가 중요합니다.

trap.c에서 MLFQ와 관련해서 동작하는 것은 크게
1. 지금 현재 프로세스의 **rtime**을 증가시키는것.
2. 지금 현재 구동중인 프로세스가 **L0큐**인데,**rtime**이 **4 tick** 이상이고, **독점적**이지 않을때 L1 큐로 강등하고 **yield()**수행함. 
3. 지금 현재 구동중인 프로세스가 **L1큐**인데,**rtime**이 **8 tick** 이상이고, **독점적**이지 않을때 **priority**가 0보다 크다면 1만큼 감소시키고,
**yield()**을 수행.
4. ***Starvation***을 방지하기 위해  **100 tick** 마다  **Priority boosting**을 수행.

하는 4가지로 구분 될 수 있습니다. 

그 중에 첫번째로

*1. 지금 현재 프로세스의 **rtime**을 증가시키는것.*입니다.
![image](/uploads/3932951b472e1206a7614f0b1784b069/image.png)
위와 같은 코드로 수행됩니다. 

주요 원리는 *TIMER interrupt는 매 tick 마다 발생*되므로, 그에 맞춰서 현재 구동중인 프로세스 정보를 얻어오고 그 프로세스의 **rtime**을 1만큼 증가시킵니다.


두번째로 

* 2. 지금 현재 구동중인 프로세스가 **L0큐**인데,**rtime**이 **4 tick** 이상이고, **독점적**이지 않을때 L1 큐로 강등하고 **yield()**수행함.  *
![image](/uploads/769d847ec85571e2656c894e7918c234/image.png)
위와 같은 코드로 수행됩니다. 

현재 레벨이 0이고, 실행된 **rtime**이 4tick상이고, 독점적이지 않는다면 **yield()**를 호출하고 L1 큐로 강등시킵니다. **yield** 함수는 sched 함수를 호출하고 sched에서 scheduler가 호출되니 다음 프로세스를 스케쥴링합니다.


세번째로

*3. 지금 현재 구동중인 프로세스가 **L1큐**인데,**rtime**이 **8 tick** 이상이고, **독점적**이지 않을때 **priority**가 0보다 크다면 1만큼 감소시키고,
**yield()**을 수행.*
![image](/uploads/7b6991f3dc1d0b3abda372421e79dc34/image.png)
위와 같은 코드로 수행됩니다.

전체적으로 두번째 L0 와 비슷하지만, priority가 0보다 크다면 1만큼 감소시키는것이 다릅니다. 감소를 하거나 하지 않은 두가지 경우 모두에서  **yield**를 호출 하기 때문에 **yield** 함수는 sched 함수를 호출하고 sched에서 scheduler가 호출되니 다음 프로세스를 스케쥴링합니다.

네번째로는 

*4. ***Starvation***을 방지하기 위해  **100 tick** 마다  **Priority boosting**을 수행.*
*trap.c*
![image](/uploads/577c507e9e7d029a43c3eb3622b1034a/image.png)
*proc.c*
![image](/uploads/1541e962398dd8e7549459f4c58bbc19/image.png)

위와 같이 trap.c에서 proc.c에 정의된 priboosting 함수를 호출합니다.
priboosting 함수는 아래에서 설명하겠지만, L1에 있는 모든 프로세스들을 L0큐로 올리고, priority를 0으로 초기화합니다.

---
### MLFQ Test 결과와 간단한 설명

**조건 :** 
1. NUM_LOOP2를 **300000(30만)**으로 수정
2. NUM_LOOP3를 **200000(20만)**으로 수정
3. NUM_LOOP4를 **500000(50만)**으로 수정

> 테스트 조건을 수정한 이유는 뚜렷한 경향성을 파악하기 위해서 **충분한 Iteration**을 확보하기 위함입니다.


첫번째 테스트 (**priority를 변경하면서**)
![image](/uploads/a820a54d8856832d54b9073e4fe7e338/image.png)
예상대로 pid 값이 더 큰 프로세스의 **priority**가 더 높게 설정되기 때문에, pid 값이 더 큰 프로세스가 먼저 끝나게 되고,

**pid값이 작은 프로세스** ( 사진에서 **5번 프로세스 혹은 4번**)가 L0에서 실행되는 시간이 긴 경향을 보입니다.


두번째 테스트(**priority 변경 없이**)
![image](/uploads/9db208a88092082689440178e011b91b/image.png)
예상대로 pid 값이 작은 프로세스가 **L1의 비율이 높습니다.** 

이는 , L1 큐에서는 같은 **priority** 를 가지는 프로세스라면 **FCFS**로 동작하기 때문입니다..


세번째 테스트 (**yield**)
![image](/uploads/662f69b215d9ad034c8f2b39111e4576/image.png)
왠만하면 L0의 **4 tick quantum**을 다 사용하지 않기 때문에 시간 사용량이 초기화되니,

계속 L0에 남아있게 됩니다. L0는 **Round robin**이기 때문에 거의 동시에 작업이 완료됩니다.



네번째 테스트 (**monopolize**)
![image](/uploads/d5258221ee29a8b7492cb6cbadb36102/image.png)
**monopolize**라는 시스템콜을 사용한 테스트입니다. 

가장 큰 PID를 가진 프로세스가 CPU 독점을 요청하기 때문에 가장 큰 pid인

Process 45가 CPU를 독점합니다. **MLFQ** 스케쥴링은 일어나지 않기 때문에 
L0의 에서 모든 작업을 완료합니다.

다른 프로세스는 test2의 결과와 유사한것을 볼 수 있습니다.

--- 
#### 트러블슈팅
**FCFS** 와는 다르게 고려해야할 부분이 많았습니다.
대표적으로 두가지 문제가 있었습니다.
1. 스케쥴링 함수를 구현할때 여러가지 조건문이 중첩됨.
2. monopolizing의 구현

다음과 같이 해결했습니다.
1. 중첩되는 모든 케이스를 명시하는것이 아니라 큰 틀에서 if문을 분기하여 처리함. 
2. 현재 프로세스가 **rtime**을 넘겼을때 yield하는 부분에서 monopolize 체크를 위하여, proc 구조체에 플래그를 삽입함. 이를 통해 trap 함수에서 호출될때 혖현재 프로세스의 monopolize flag를 체크해서 독점적이지 않을때만 CPU 자원을 포기하도록 구현함.



--- 
### 시스템 콜

과제 수행을 위해 필요한 **System call**은 총 4개입니다.
1. void yield(void)
2. int getlev(void)
3. void setprioirty(int pid,int priority)
4. void monopolize(int password)

각각의 시스템 콜 구현체를 먼저 보이고 , 공통적인 부분은 마지막에 첨부합니다

첫번재로 **void yield(void) 함수**입니다.
![image](/uploads/cc7b7ebeee31147342bac4fc447c8399/image.png)
sys_yield 시스템콜은 proc.c에 정의되어 있는 yield 함수를 호출하는 것입니다. 현재 프로세스가 CPU 자원을 반납하는 행동입니다.




두번째로 **int getlev(void) 함수**입니다. 현재 프로세스의 큐 레벨을 반환하는 것입니다.
![image](/uploads/22b75a619abe7c14e3ba7f90cac7eb76/image.png)
![image](/uploads/9d611c6099bb9cb11315d2847e4e139e/image.png)
getlev 시스템콜은 proc.c 에 정의 한 getlev 함수를 호출합니다. 

proc.c 에서 정의된 getlev 함수는 
현재 프로세스가 독점중인 상태라면 1을 반환하고, 

그렇지 않다면 현재 프로세스의 큐레벨인 **lev** 을 반환합니다. (*return myproc()->lev*);

세번째로 **void setpriority(int pid,int priority) 함수 **입니다.  인자로 받은 pid와 일치하는 프로세스의 priority를 인자 priority로 변경합니다.
![image](/uploads/23ba39493774781914d210c477d398c3/image.png)
![image](/uploads/30588247bda82bef58c725830014381a/image.png)
setpriority 시스템콜은 proc.c에 정의한 setprocpriority 함수를 호출합니다. 

proc.c에서 정의된 setprocpriority 함수는
현재 **ptable**에서 **인자 pid와 매칭되는 프로세스**를 찾고, **인자 priority로 프로세스의 priorirty를 변경**합니다.*(targetP->priority=priority)*

시스템콜이기 때문에 argint를 이용하여 인자를 받습니다.

마지막으로 ** void monopolize(int password) 함수 **입니다. 인자로 받은 password와 미리 설정한 본인의 학번과 비교한뒤 일치하면

현재 프로세스를 **CPU에  독점적인 권한**을 가지게 변경합니다.
![image](/uploads/b888f030441243e189d3fb462ff06ebe/image.png)
![image](/uploads/53169d6ca932811f0fa3c3a29cdfc509/image.png)
monopolize 시스템콜은 proc.c에 정의한 monopolize 함수를 호출합니다.

proc.c에 정의된 monopolize 함수는
먼저 **ptable**의 **lock을 acquire**해주고

현재 프로세스의 **monopolize**가 1이라면,**독점적이라면**, 인자 password를 학번 2016026599와 체크한후, **일치**하면

**monopolize**를 0으로 바꾸어 독점해제하고, 현재 프로세스를 L0으로 이동시키고 **priority**를 0으로 변경합니다.

**일치하지 않으면** 독점을 해제하고, 현재 프로세스를 kill 합니다.*(kill 함수의 행동과 유사하게 행동합니다)* 

**그 후 "wrong password at calling monopolize"라고 유저에게 안내합니다.**

현재 프로세스의 **monopolize**가 0이라면,**독점적이 아니라면,** 인자 password를 학번 2016026599와 체크한후, **일치**하면

**monopolize**를 1으로 바꾸어 독점을 시작하고, 현재 프로세스를 L0으로 이동시키고 **priority**를 0으로 변경합니다.

**일치하지 않으면** 현재 프로세스를 kill 합니다.*(kill 함수의 행동과 유사하게 행동합니다)*.

**그 후 "wrong password at calling monopolize"라고 유저에게 안내합니다.**

그 후 **ptable**의 **lock을 release** 해줍니다.

---
### 시스템 콜 공통 사항

#### usys.S
![image](/uploads/92b6a5016c03db42c037c7af1b0b01c2/image.png)

#### user.h
![image](/uploads/a30f9ec5bd91af092732925684dd4c30/image.png)

#### syscall.h
![image](/uploads/781e6ec5276140a7c0d8148512e1c1e5/image.png)

#### syscall.c
![image](/uploads/0ff0ba599a1337b0f08954261557bb37/image.png)
![image](/uploads/23234e5587b29c7964f8881930350352/image.png)

#### Makefile 시스템 콜 부분

![image](/uploads/9c393b59729571c56f4dc0d1a9ba272c/image.png)

#### Makefile 코드 분기를 위해서 삽입한 부분


![image](/uploads/daad526d1a17caec51e6678a7702cfe9/image.png)

![image](/uploads/88cc18596c3c38549317a9c5198088bb/image.png)


---

### 시스템콜 트러블슈팅
몇가지 마이너한 이슈가 있었습니다.

시스템콜의 인자를 처리할때는 **argint**을 사용한다는 것을 몰라 사용하지 못했고, 좀 더 자료를 찾아본 후 적용가능했습니다.

**monopolize** 함수의 처리가 **trap.c 내부** 에서 일어나야 할지 , **proc.c 내부**에서 일어나야할지 선택해야 했습니다.

**proc 구조체**를 수정해 monopolize flag 자체는 **proc.c 내부에서 수정**하되 **trap 내부에서  체크**하여 적용하는 것으로 구현했습니다.



