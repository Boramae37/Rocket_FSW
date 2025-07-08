# CMake 프로그램의 최소 버전
cmake_minimum_required(VERSION 3.25.1)

# 프로젝트 정보
project(
  Genesis_sensor_app
  VERSION 0.1
  DESCRIPTION "Rocket_FSW_Project"
  LANGUAGES CXX

)
# 실행 파일 저장 위치.. run 폴더에 넣기
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../run)


# 확인할 디렉토리 추가
#add_subdirectory(lib)

# 실행 파일 만들
add_executable (sensor main.cc)


# program 에 communication 를 링크
#target_link_libraries(sensor)
