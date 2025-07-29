# CMake 프로그램의 최소 버전
cmake_minimum_required(VERSION 3.25.1)

# 기본 변수 : CMAKE_SOURCE_DIR : 실행시키는 최상단 CMakeLists.txt 의 경로(위치)
#             그냥 헤더를 추가할 경우, add_executable에 c파일 경로(상대경로) 입력, targe include directories에 참조할 폴더 경로넣기
# 프로젝트 정보
project(
  Genesis_softwarebus_app
  VERSION 0.1
  DESCRIPTION "Rocket_FSW_Project_SoftwareBus"
  LANGUAGES C CXX
)
# 실행 파일 저장 위치.. run 폴더에 넣기
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../run)




# 확인할 디렉토리 추가
#add_subdirectory(lib/BMP_SensorAPI)

# 실행 파일 만들
add_executable (softwarebus main.cc ${CMAKE_SOURCE_DIR}/../ipc_set/connection.cc)

# include
target_include_directories(softwarebus PRIVATE ${CMAKE_SOURCE_DIR}/../ipc_set)

# program 에 communication 를 링크
#target_link_libraries(sensor)
