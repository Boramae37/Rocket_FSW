# CMake 프로그램의 최소 버전
cmake_minimum_required(VERSION 3.25.1)

# 프로젝트 정보
project(
  Genesis
  VERSION 0.1
  DESCRIPTION "Rocket_FSW_Project"
  LANGUAGES CXX)

# 확인할 디렉토리 추가
#add_subdirectory(lib)

add_executable (supervisor main.cc)

# program 에 communication 를 링크
#target_link_libraries(supervisor communication)
