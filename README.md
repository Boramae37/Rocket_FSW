# 미리 설치할 파일(외부 라이브러리)-------------

## spdlog

```
sudo apt install libspdlog-dev
```
이게 fmt 포함함

<!-- ## fmt library

    sudo apt install libfmt-dev -->

## cmake

```
sudo apt install cmake
```

make로 빌드를 하기위한 파일을 만들어줌

폴더 지저분해지므로 build 폴더에서 진행.. 

```
cmake ..
```
CMakeLists.txt 파일을 바탕으로 진행됨. 해당 파일이 build 폴더랑 같은 폴더에 있으므로, build 속에서는 .. 으로 그 부모 폴더를 가리켜야 한다.


## make 
프로젝트 빌드 프로그램(?)

```
sudo apt install make
```
보통 깔려 있음

사용법 

cmake 진행한 폴더에서 (build 폴더)
```
make -j4
```
-j는 멀티 스레딩으로 make 진행하라는 거고, 4면 4코어.. 라즈베리파이 zeor 2w는 4코어 cpu 사용하므로 make -j4 사용



## pigpio

아래 사이트 참고해서 설치
https://abyz.me.uk/rpi/pigpio/download.html

wget, unzip 안되면 sudo apt install 하면 됨

오래 걸린다고 하니 천천히 기다릴 것...

프로그램 실행전에 
```
sudo systemctl start pigpiod
```
로 데몬 실행 후에 프로그램 실행해야함


## tmux (복붙 편하게 하려면)
```
sudo apt install tmux
```
이제 
```
tmux
```
로 실행하고 nvim 켜서, 편집 창에서 드래그한 뒤 y눌러 복사, p로 붙여넣기 가능

이외 기능 더 있으니 찾아서 써도 좋음


# 실행 방법

## 준비

## I2C, UART 활성화

```
sudo raspi-config
```
실행 후, interface에서 i2c, uart 활성화 해주자

uart의 경우 처음 물음(로그인 관련) no. 2번째 물음에는 yes 하면 된다.


### UART Setup

라즈베리파이 zeor 2w는 기본적으로 블루투스를 위한 Mini UART를 사용한다. 따라서 이를 Hardware UART로 설정을 변경해줘야 한다. 성능과 안정성면에서 Hardware UART가 훨씬 좋기 때문이다.

```
ls -l /dev/serial0
```
결과가 만약 serial0가 ttyS0 등으로 연결되어 있다고 나오면, mini UART인 것이다. 
Hardware UART의 경우 ttyAMA0 로 연결된다.(일반적으로.. 2025.08.06 기준 zero 2w)

boot 폴더의 config.txt를 수정하면 된다.

neovim 설치했을 경우, nano 대신 nvim 쓰면 됨
```
sudo nano /boot/config.txt
```

맨 아래에 
```
enable_uart=1
dtoverlay=disable-bt
```
써주고, 닫는다.

이제 재시작한다.

```
sudo reboot
```
혹시 재실행으로 안되거나 버그 있을 수 있으니, 파워 off / on 해도 된다.


## 실행

각 _app 폴더 안에 build 폴더를 만들어줘야 함
그리고 각 build 폴더에서 
```
cmake ..
```

가장 상위 폴더로 돌아와서, 
```
./run_make.sh
```
실행하면 make 진행


이후 startup.sh 을 통해 프로그램 실행하면 됨
```
sudo ./startup.sh
```

만약 이걸로 안되면,
```
sudo killall pigpiod
sudo pigpiod
cd supervisor_app/build/
./supervisor
```
실행하면 됨. 한줄 씩 실행하는 것 추천

build 폴더 만들기와 cmake, run_make 모두 최초 1회만 진행하면 됨. (코드 변경 없을 시..)



# 폴더 설명

includes : 헤더 파일 모음
lib : 헤더의 구현파일 모음
어떤 라이브러리를 가져다 쓰다면, 헤더를 꼭 참조해야한다..

build : cmake 실행시, 여러 파일들이 지저분하게 생성됨. 따라서 bulid에 들어가서 cmake를 실행하도록 하자

      


# 유용한 명령어

cmake 주소(CMakeLists.txt 파일이 있는..) => build의 바로 부모 폴더면 cmake .. 만 해도 됨


### UART 경로 확인
```
ls -l /dev/serial*
```

### uart 테스트
```
sudo stty -F /dev/serial0 raw 115200 cs8 clocal -cstopb
```
이거로 xbee와 연결된 하드웨어 uart 설정 가능, 

```
sudo stty -F
```
로 확인 가능(설정 내용 뜰거임)

```
cat /dev/serial0
```
실행시 수신 모드가 됨. 반대(지상국)에서 xcty로 송신시 수신한 내용 띄워줌

반대로
```
echo "test" > /dev/serial0
```
이걸 통해 송신 테스트 가능

