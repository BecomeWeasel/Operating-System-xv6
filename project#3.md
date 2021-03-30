운영체제 과제 3(LWP)
---
---
### 테스트 환경 :
##### OS : Ubuntu 16.04
##### gcc : gcc 5.4.0


### 개요 :
운영체제 세번째 과제인 **Light-weight Process (Thread)**에 대한 내용입니다.

크게 **thread_create**, **thread_exit**, **thread_join**을 통해 구현됩니다.


---

## thread 구현을 위한 proc 구조체 변경사항

`int isThread,int numOfThread,int nextThreadId,thread_t tid,void * retval, struct proc*p creator`등을 추가했습니다.
그중에서  creator 멤버는 기존 ** proc 구조체**의 **parent**와 비슷한 역할을 수행합니다.

이번 설계에서 process와 **thread_create**를 통해 생성된 thread는 *parent-child* 관계가 아니고 **pid**도 다르기 때문에 **creator**라는 포인터를 가짐으로써 *최소한의 연결 관계*를 유지해줍니다. *( 이 방식은 밑에 다시 설명드립니다.)*

*(단 프로세스의 경우에는 creator와 parent가 같다고 생각합니다.)*

### 기본적인 Thread 기능 명세 :
먼저 **Thread**의 기본 명세는 다음과 같습니다.
![image](/uploads/40a1e5a149b14d8fab65c1a96429415e/image.png)


1. LWP는 Process와는 다르게 주소 공간을 공유합니다.
2. LWP는 본인을 생성한 Process와 다른 **pid** 값을 가집니다.
3. LWP는 본인을 생성한 Process를 pointer로 **creator**에 저장함으로써 최소한의 연결관계를 유지합니다.
3. LWP가 **fork**를 만나면 정상적으로 수행해야 합니다. * 단, fork된 process는 LWP의 **creator**에는 접근이 불가합니다.*
4. LWP가 **exec**를 만나면 정상적으로 수행해야 합니다. *exec를 수행할때 , LWP의 creator의 다른 LWP들은 종료되어야 합니다.*
5. LWP가 **exit**를 만나면 다른 모든 LWP가 종료되어야 합니다.
6. LWP의 creator가 kill의 대상이 되면 craetor의 다른 모든 LWP역시 종료되어야 합니다.

### 선언하고 구현한 시스템콜
![image](/uploads/6122271cc214bc86d7acc206e0f735ab/image.png)

1. **thread_create** : thread를 생성합니다. 기존의 시스템콜인 fork와 유사합니다.
2. **thread_exit** : thread를 종료합니다. 기존 시스템콜인 exit와 유사합니다. 이때 결과물을 proc 인자로 받은 retval 안에 저장합니다.
3. **thread_join** : 특정 thread를 기다립니다. 기존 시스템콜인 wait과 유사합니다. 다른 점은 인자로 받은 double-pointer에 return 값을 저장합니다.
4. (**thread_exit_target**) : 명세에는 표시되지 않았지만, 특정 쓰레드를 종료시키기 위해서 직접 구현했습니다.



### Thread의 생성 : thread_create
첫번째로 **thread_create** 함수에 대해서 설명과 예시입니다.

먼저 함수의 인자에 대해서 간략히 설명합니다.
1. 첫번째 인자인 **thread_t * thread**는 thread 생성이 끝난 후 추적을 위해 Thread의 id를 저장하는 용도로 사용했습니다.
2. 두번째 인자인 **함수 포인터 start_routine**은 생성된 Thread가 해야하는 함수의 진입점으로 사용됩니다.
3. 세번째 인자인 **void * arg**는 두번째 인자인 함수를 수행하는데 사용되는 것으로 ustack에 저장됩니다.

전체적인 동작사항은 **fork** 시스콜과 다르지 않지만
![image](/uploads/776540ffcb888d9b96b73dd798687230/스크린샷_2019-06-02_14.19.43)
위의 부분에서 ```allocproc```를 통해 반환받은 proc 구조체를

thread 처럼 동작하게끔 하기 위해  **isThread** 값을 1로 설정해주고 , **tid**값을 현재 proc의 **nextThreadId**값을 가져와서 설정해줍니다. * ( 단 이 tid는 연속적이나, 실행되고 있는 Thread들의 tid는 연속적임을 보장하지 않습니다. Ex) tid : 1 2 5 6*
**creator**를 현재 Process로 설정해줍니다. 이를 통해 thread와 Process간의 *Parent-Child* 관계는 아니지만 *(```np->parent=curproc->parent``` 이기 때문에)* 

**어느 정도의 관계를 가지고 있게끔 구현했습니다. 
또 시작지점을 설정하는 부분에서는 기존 시스템콜인 **exec**을 참고하였습니다.
![mage](/uploads/8b295403924960ff8613158134065669/스크린샷_2019-06-02_14.19.54)

**ustack**을 만들어주고 **copyout**을 통해서 복사해줍니다.

``` np->tf->eip=(uint)start_routine ```
또 **eip**값을 인자로  들어온 start_routine으로 설정해주어서 thread 가 수행할 
함수의 진입점을 올바르게 설정해줍니다.

또 LWP끼리는 주소공간을 공유해야 하기 때문에
```
acquire(&ptable.lock);
struct proc* p;
~~~
for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
             if(p->parent->pid==np->parent->pid){
                     p->sz=np->sz;
               }
}
~~~
release(&ptable.lock);
```
위와 같은 코드를 통해서 sz값을 공유시켜줍니다. 이를 통해 **Memory Illegal Access** 문제를 방지 할 수 있습니다.

**fork** 시스템콜에서는 생성된 Process의 **pid**값을 반환했지만, thread_create에서는 인자로 전달받은
**thread_t * thread**에 thread의 **tid**를 저장해줍니다.

### Thread의 종료 : thread_exit

두번째로 **thread_exit** 함수에 대해서 설명과 예시입니다.

반환형은 void이고 함수의 인자에 대해서 설명합니다.
1. 첫번째 인자인 **void * retval** 입니다.  thread가 종료될때 이 인자에 결과값을 저장합니다.

전체적인 동작은 ** exit ** 시스템콜과 크게 다르지 않습니다.

![image](/uploads/ce2b52e8683caf4263e93293310e6302/스크린샷_2019-06-02_15.46.52)

한가지 다른점은 
```
curproc->creator->numOfThread--;
``` 
을 통해 자신이 속해있는 Process의 쓰레드 개수를 줄이는 기록을 합니다.

### Thread가 끝나기를 기다림 : thread_join

세번째로 **thread_join** 함수의 설명입니다.

반환형은 int로, join이 성공적이면 0을 .그렇지 않으면 다른 값을 반환합니다.

함수의 인자는 두개 입니다.
1. 첫번째 인자인 **thread_t thread**입니다. join의 대상 thread를 지칭하는데 사용됩니다.
2. 두번째 인자는 ** void ** retval **입니다. thread_exit을 통해서 반환된 값을 저장해줍니다.

**thread_join**도 기존의 **wait**시스템콜과 유사합니다.

![image](/uploads/b0439082ad6e66ea166ed1880430df88/스크린샷_2019-06-02_15.59.34)

다른 점이 몇가지 있는데 

1. ```freevm(p->pgdir)```을 사용하지 않습니다. 주소공간을 공유하기 때문에 이 함수를 join에서는 사용하지 않습니다.
2. ```*retval=p->retval```이라는 새로운 코드를 추가합니다. 인자로 전달받은 **double-pointer retval**에 저장합니다. 이를 통해 완료된 값들을 받아갈 수 있습니다.

또 한가지 중요한점은 

```sleep (curproc,&ptable.lock);```

을 사용한다는 점입니다(기존의 **wait**에도 존재합니다.) 이를 통해서 모든 자식들이 끝나기를 기다립니다.


### xv6와의 상호작용

* 일부기능을 제외하고는 프로세스와 같이 움직여야 하며 , 내장 스케쥴러인 RR을 사용합니다 *

1. **Fork** 의 경우에는 LWP의 대부분의 기능이 Process와 비슷하게 동작하기 때문에 큰 수정 없이 정상작동합니다.
2. **Exec**의 경우에도 큰 수정 없이 정상작동합니다. *단 , thread가 exec를 만났을때,자신을 제외한 같은 프로세스에 속해 있는 다른 thread을 종료해야 하도록 구현했습니다.*
3. **sbrk**의 경우에는 **growproc** 시스템콜의 수정이 필요합니다. 스레드의 주소공간이 늘어났다면, 같은 프로세스 군에 속해있는 모든 thread가 이 변경된 **sz**의 값을 알아야합니다. 그렇기에 아래처럼
![image](/uploads/1f18d9c390af1306e039cd6b08c4c3ef/image.png) 
같은 군에 속해있거나 특정 프로세스(프로세스 군의 main)의 sz를 변경해줍니다. 그렇지 않으면 **page fault**가 나는것을 확인했습니다. 또
 growproc에서 sz을 접근할때, 다른 스레드가 sz를 수정중일 수 있으니 growproc 전체적으로 ptable lock을 잡아야합니다.
5. **kill**의 경우에는 **thread_kill **test 에서 다시 언급하겠지만, Process가 kill을 당하면 그 프로세스에 속해있는 모든 프로세스를 종료해줍니다. 기존의 **kill** 시스템콜에서 조금의 추가가 있습니다.
6. **Exit**의 경우에 thread가 exit을  만나면 같은 프로세스에 속해있는 다른 thread와 자기 자신을 종료하도록 구현했습니다.

### xv6와의 상호작용 - 기존 시스템콜 변경점 

## kill 
![image](/uploads/b0c6b1a1d3ce4b9d59f436f91119a0e1/image.png)
**kill** 시스템에 다른부분은 기존과 동일하지만 사진과 같이 

ptable을 돌면서 같은 process 집합에 속해있는 Thread들을 killed=1로 바꿉니다.
이것은 **kill**의 시스템콜의 기능과 유사합니다. 이것을 통해서 프로세스가 kill 되었을때 속해있는 다른 쓰레드들도 kill을 당하게 합니다.

## exit
![image](/uploads/17425f0e7562f96082effafa59ecbad0/스크린샷_2019-06-02_23.49.20)

exit도 ptable을 돌면서 자신과 같은 프로세스군에 있지만 자신을 제외한 스레드를 종료하고 같은 집합에 속해있는 Process도 삭제하는
killAllFromThread을 호출하여 exit을 수행합니다.

위의 상황은 thread가 exit을 만낫을때고 Process가 exit을 만낫을때도 자신이 생성한 thread를 모두 죽이는 역할을 수행하는것이 
killAllFromThread입니다.

## growproc
상기 언급한것과 같이 sz사이즈를 전체적으로 조절합니다. 

*단 좀더 광범위하게 ptable에 대한 lock을 요청하여 다른 쓰레드가 sz를 수정하는것을 방지합니다.*


## Test 결과 
테스트 결과입니다. 

* **단 앞단에 기재한것과 같이 test 구동에 문제가 있기에 테스트 하나를 끝내고
xv6를 다시 실행해야 합니다.** *

### thread_test 1:create,exit,join 
*생성하는 thread의 개수를 3개로 늘려서 진행해보았습니다.*
![image](/uploads/88aed5934e84b9ebf40262897da159a1/스크린샷_2019-06-02_18.42.45)
정상적으로 thread를 3개 생성하고, 3개 exit 한후 join해서 정상적으로 테스트를 수행합니다.

### thread_test 2:fork
![image](/uploads/402aad68dca594cee267a02c52b35e3c/스크린샷_2019-06-02_18.42.48)
thread 내에서 fork를 진행한후 정상적으로 끝내는 모습을 확인할 수 있습니다.

### thread_test 3:sbrk
![image](/uploads/6155d1f53d391be2e16296cd1074b14f/스크린샷_2019-06-02_18.42.55)
thread 내에서 malloc을 사용해 **growproc**을 호출하고 , free(dealloc)을 사용해 다시 **growproc**을 정상적으로 호출하는 것을 볼 수 있습니다. **growproc**을 수정하지 않으면 remap 패닉이 발생합니다.

thread_test 내 3가지 세부 테스트 모두 성공한 것을 확인 할 수 있습니다.
-- --


### thread_exec
![image](/uploads/cb991620e5e5a68289c89a5035f704ca/image.png)
thread가 exec을 만났을때, 자신을 제외한 thread를 모두 종료시키고

정상적으로 Hello, thread를 출력하는것을 확인 할 수 있습니다.
### thread_exit
![image](/uploads/4fdba2cf49166bfe206bfbe0f37fea0a/image.png)
thread가 exit을 만났을때, 그 프로세스 내의 모든 thraed를 종료시키는 테스트입니다.
정상작동합니다.

### thread_kill 
![image](/uploads/57a3542705244a4187d975aeee74ed82/image.png)
thread가 속한 프로세스가 kill 되었을때, 그 프로세스에 속한 thread를 모두 종료시키는 테스트입니다.
정상작동합니다. zombie 프로세스 역시 생기지 않습니다.


### 트러블슈팅
thread의 기본적인 생성과 종료,그리고 join을 구현하는데 있어서 막막한 감이 있었습니다.

기존 xv6의 ** fork,exec,allocproc,wait,exit**의 코드등을 참고해서 새로운 시스템 콜들을 만들었고

기존의 시스템콜 역시 thread의 상황에 맞게 적절히 수정했습니다.

예를들어  thread_create 부분에서 단순히 **fork 시스템콜**의 

```
switchuvm(np)
```

라는 부분이 있었는데, 이것을** thread_create **단으로 가져오니 알 수 없는 이유의
패닉이 자꾸 발생하였고,

switchuvm의 주요 목표가 * **cr3 레지스터의** 값을 변경하는것* 이라는걸 알기 위해서

수업시간에 진행했던 lab pdf를 참조 후 그 부분을  
다음과 같이 수정했습니다.
```
// switchuvm-like
pushcli();
lcr3(V2P(np->pgdir));
popcli();
```
이렇게 수정하고 나니 패닉도 발생하지 않았고, 원하는 대로 함수가 구현되었습니다.

### 미구현점 혹은 남은 문제사항

## 하단에 표시된 문제사항 최신 커밋 기준으로 해결되었습니다.

