# 미리 설치할 파일(외부 라이브러리)-------------

## fmt library

    sudo apt install libfmt-dev

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

# 폴더 설명

includes : 헤더 파일 모음
lib : 헤더의 구현파일 모음
어떤 라이브러리를 가져다 쓰다면, 헤더를 꼭 참조해야한다..

build : cmake 실행시, 여러 파일들이 지저분하게 생성됨. 따라서 bulid에 들어가서 cmake를 실행하도록 하자

      
lib에서 cmake


# 유용한 명령어

cmake 주소(CMakeLists.txt 파일이 있는..) => build의 바로 부모 폴더면 cmake .. 만 해도 됨


UART 경로 확인
```
ls -l /dev/serial*
```


시작 시에 
```
sudo stty -F /dev/serial0 raw 115200 cs8 clocal -cstopb
```
이거 해야 uart 연결될 수도..
