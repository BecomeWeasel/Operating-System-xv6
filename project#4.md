운영체제 과제 4(double indirect inode)
---
---
### 테스트 환경 :
##### OS : Ubuntu 16.04
##### gcc : gcc 5.4.0


### 개요 :
운영체제 네번째 과제인 **double indirect inode**에 대한 내용입니다.

크게 **fs.c**안의 **bmap , itrunc**함수를 수정하고, **fs.h**와 **file.h**, **param.h**의 값을 조금 수정함으로써 **double indirect inode**를 구현합니다.

---

### 과제 명세 :
먼저 xv6의 기본적인 inode의 구조에 대한 간단한 구조입니다.
![image](/uploads/3bdfe834c00520bc22f3a31fc4fb530b/image.png)

**dinode** 구조체에서 **direct block pointer는 12개**가 존재하고, 1개의 **indirect block pointer**가 존재합니다.

그런 구조를 아래와 같이 수정해야합니다.
![image](/uploads/5dff876b79e7864b6b3ef8462395de38/image.png)

**dinode** 구조체에서 **direct block pointer**가 11개로 바뀌고, 1개의 **single indirect block pointer**, 그리고 1개의 **dobule indirect block pointer**가 존재합니다.


구현하기 위해서는 **bmap**과 **itrunc** 함수를 수정해야 합니다.
![image](/uploads/61a8e0b92fdb2c4d83e409c92ac022cc/image.png)

---

## fs.h 수정 사항

**fs.h**에서 NDIRECT의 값을 1만큼 줄여주고 **(12->11)**,

**NDOUBLEINDIRECT** 의 값을 **NINDIRECT의 제곱**으로 정의합니다.

**addrs 배열**의 크기를 NDIRECT의 값과 매치되게 수정합니다. 

```
( addrs[NDIRECT+1]->addrs[NDIRECT+2])
```
![image](/uploads/43bffc1ffc7cf0b5f2b0dbd800c9f009/image.png)

이렇게 작성하면 **direct block pointer**의 개수는 11개로 줄어들고, 
**single(addrs[11]), dobule indirect block pointer(addrs[12])**가 만들어집니다.

## file.h ,param.h 수정 사항
**inode **의 구조체 안에 있는 addrs 배열도 **dinode**에서 addrs 배열을 수정한것과 같이 수정해줍니다.

```
( addrs[NDIRECT+1]->addrs[NDIRECT+2])
```
![image](/uploads/f81d18993439129826a3d64985d328b9/스크린샷_2019-06-21_16.51.47)

**param.h** 안의 **FSSIZE** 값도 20000이상인 **30000**으로 수정해줍니다.
![image](/uploads/541782dd61dc940bde16715f005596f1/image.png)
---
## fs.c 수정 사항

먼저 **bmap** 함수를 수정했습니다.
기존의 bmap 함수를 최대한 참고하여 , double indirect block pointer의 역할을 수행할 수 있게 수행했습니다.

![image](/uploads/23b449c7b9ac04f9d8c55518d04958f8/image.png)

그 후에는 **itrunc** 함수를 수정했습니다.
기존의 iturnc 함수를 참조하고, double indirect이기 때문에 두번의 for문이 필요한 구조입니다.
![image](/uploads/326cba548f07bc28e8f1befa92b60e9d/image.png)

---

### file_test 결과
**file_test**는 T1,T2,T3로 구성되어 있습니다.

T1은 파일에 8MB정도의 크기를 쓰고, T2는 8MB의 크기를 읽고 T3는 T1과 T2를 10회 정도 반복해서 전체적인 구조에서 문제가 생기지 않는지를 체크합니다.

![image](/uploads/a27d382868d935ff50e9083e053c6621/스크린샷_2019-06-21_17.01.07)

위의 그림과 같이 큰 문제 없이 T1과 T2,T3를 통과합니다. 여러번의 file_test를 시도해도

테스트는 성공적으로 종료됩니다.


---
### 추가 구현점과 문제점

눈에 보이는 큰 문제점은 없습니다. 추가 구현점이라면 , 

Project 3(LWP)에서 만들었던 Thread의 개념을 응용해 **multi-thread** 환경에서 파일을
읽고 쓰는 기능을 만들수 있을것 같습니다.


