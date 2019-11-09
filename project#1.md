운영체제 과제 1 ( Simple User-level Unix Shell)
-- 
### 테스트 환경 :
##### OS : Ubuntu 16.04
##### gcc : gcc 5.4.0

### 개요 :
운영체제 첫번째 과제인 Simple User-level Unix shell에 대한 내용입니다.

크게 **interactvie mode**와 **batch mode**로 나뉘어지고 
사용자가 입력한 명령 혹은 batch file을 읽어와서 


명령을 수행하고 그 결과를 출력하는 프로그램입니다.

### 작동과정 설명 :
## Interactvie Mode
첫번째로 **interactvie mode**내에서의 작동 과정과 예시입니다.
크게 사용자로부터 **입력을 받아오고**, 

입력을 **특정한 기준을 가지고 분할**하여
**execvp 함수를 사용**하여 처리합니다.

1. fgets() 함수를 통해 사용자에게 입력을 받습니다.
2. strtok 함수를 사용하여 semi-colon과 space를 기준으로 raw한 입력을 유의미하게 분할합니다.
3. 분할된 명령어들은 char* 이차원 배열에 저장됩니다.
3. 명령의 개수만큼 fork 함수를 통해 자식프로세스를 생성합니다.
4. 자식프로세스안에서 execvp 함수를 통해 저장된 명령어를 처리합니다.
5. 부모 프로세스에서는 모든 자식프로세스가 끝나기를 기다린후, 모두 종료되면 다시 1번으로 돌아가도록 유도합니다.
만약 이 과정에서 에러가 검출된다면 유저에게 알려줍니다.


--- 
### 작동예시 :
실행된 후 먼저 사용자가 명령을 입력할때까지 프로그램은 대기합니다.

그 후 사용자가 명령을 입력하면 (ex.ls -al)

``` > ls -al ```
![image](/uploads/9f4f1d35d29ff4432bb78e3e95f44c58/image.png)
위와 같이 명령을 처리합니다.

복수의 명령어( **semi-colon**으로 구분된)도 **병렬적으로** 처리가 가능합니다. (출력 순서는 섞일수 있습니다.)

``` > ls -al;pwd;ifconfig ```
![image](/uploads/16fa5e8dfc47a510db68e20de40b7e46/image.png)

quit command 입력시 쉘을 종료합니다.

``` > quit ```
![image](/uploads/99751011f8999be44faf76182684eca5/image.png)

### 에러 처리:

**존재하지 않는 명령어**를 입력시 유저에게 알려줍니다.

``` > thisisnotvalid ```
![image](/uploads/5c087bc7767963eedaf0f243ab2badd5/image.png)

**아무 입력을 하지 않고 엔터를 입력할 경우** 다시 입력하도록 유도합니다.
![image](/uploads/3946ad6c8b760ca2ec459ef5e3d3218d/image.png)

**아무 입력을 하지 않은 상태에서 ctrl-D를 입력할 경우** shell은 종료됩니다.
![image](/uploads/ab43e066649b669847a2ba847d9d5bc5/image.png)

### 실행 전제 :
1. **입력의 길이는 최대 1000자**입니다.
2. **입력받는 명령어는 최대 50개**입니다.
3. **명령어당 옵션의 개수(space로 구분하는 ex) -a -l -v -> 3개)는 최대 10개**입니다.

---
### 작동과정 설명 :
## Batch mode
두번째로 **batch mode**에 대한 설명과 예시입니다.
유저가 main 함수의 인자로 넘긴 batch_file을 읽어오고 

명령어를 분할하고 처리하는 과정은 **interactvie mode**와 동일합니다.

1. fgets() 함수를 통해 사용자가 실행될때 첨부한 batch_file을 한줄단위로 읽어옵니다.
2. 마지막줄을 읽었다면 중지합니다.
2. 만약 명령어가 없고 개행문자만 있는 빈 줄(\n)을 읽었다면 다음 라인을 읽기 위해 1번으로 돌아갑니다.
2. strtok 함수를 사용하여 semi-colon과 space를 기준으로 raw한 입력을 유의미하게 분할합니다.
3. 분할된 명령어들은 char* 이차원 배열에 저장됩니다.
3. 명령의 개수만큼 fork 함수를 통해 자식프로세스를 생성합니다.
4. 자식프로세스안에서 execvp 함수를 통해 저장된 명령어를 처리합니다.
5. 부모 프로세스에서는 모든 자식프로세스가 끝나기를 기다린후, 모두 종료되면 다음 라인을 읽기 위해 1번으로 돌아갑니다.
만약 이 과정에서 에러가 검출된다면 유저에게 알려줍니다.

---
### 작동예시:
```./shell [batch_file]``` 형식으로 **batch mode**를 사용합니다.

``` vim test.txt```
![image](/uploads/0e1f3919cbe66a86178b4d79f96110fc/image.png)


![image](/uploads/ca1a0a07e5a35dd8309495002207d4b1/image.png)

### 에러 처리:
**존재하지 않는 명령어**를 입력시 유저에게 알려줍니다.

``` > vim test.txt ```
![image](/uploads/3fd2270ef36cb0e1e003e09ab25b5ae2/image.png)
![image](/uploads/64792fe0a9f0a2d457e6a57dc6b2b849/image.png)

**사용자가 두개의 batch file을 인자로 넣을시** 유저에게 경고를 보여주고 shell을 종료합니다.
![image](/uploads/9c0371a16a20a5df892100ccaab22dec/image.png)


### 실행 전제 :
1. **입력의 길이는 최대 1000자**입니다.
2. **입력받는 명령어는 최대 50개**입니다.
3. **명령어당 옵션의 개수(space로 구분하는 ex) -a -l -v -> 3개)는 최대 10개**입니다.

---
### 기타 에러 처리 사항:
1. fork가 실패했을시 메모리에 심각한 문제가 있다고 판단하여 shell 을 종료합니다.
2. batch file을 fopen을 사용했을대 NULL이 반환될시 비정상적 작동을 막기 위해 shell을 종료합니다. 
2. batch file을 두개 이상 사용시 (ex. ./shell batch_file batch_file 유저에게 경고를 보여준후 shell을 종료합니다.
3. 명령어를 분할하는 단계에서 \n 개행문자를 삭제합니다. 개행문자가 삭제되지 않고 execvp의 인자로 입력될시 illegal option 에러를 표출하기 때문입니다.

