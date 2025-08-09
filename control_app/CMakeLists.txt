# CMake 프로그램의 최소 버전
cmake_minimum_required(VERSION 3.25.1)


# 기본 변수 : CMAKE_SOURCE_DIR : 실행시키는 최상단 CMakeLists.txt 의 경로(위치)
#             그냥 헤더를 추가할 경우, add_executable에 c파일 경로(상대경로) 입력, targe include directories에 참조할 폴더 경로넣기

# 프로젝트 정보
project(
  Genesis_Control_app
  VERSION 0.1
  DESCRIPTION "Rocket_FSW_Project_Sensor"
  LANGUAGES C CXX
)

include(FetchContent)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.12.0  # 원하는 버전
)

FetchContent_MakeAvailable(spdlog)

# 실행 파일 저장 위치.. run 폴더에 넣기
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../run)


# pigpio 라이브러리 경로 설정
set(PIGPIO_ROOT "${CMAKE_SOURCE_DIR}/../external/pigpio")

# pigpio 헤더 파일 경로
include_directories(${PIGPIO_ROOT})


# spdlog 패키지 찾기
# find_package(spdlog REQUIRED)
# add_subdirectory(${CMAKE_SOURCE_DIR}/../external/spdlog)


# 실행 파일 만들
add_executable (control main.cc ${CMAKE_SOURCE_DIR}/../ipc_set/connection.cc)

# include
target_include_directories(control PRIVATE ${CMAKE_SOURCE_DIR}/../ipc_set)

target_link_libraries(control PRIVATE ${PIGPIO_ROOT}/libpigpiod_if2.so ${PIGPIO_ROOT}/libpigpio.so pthread rt)
target_link_libraries(control PRIVATE spdlog::spdlog)


