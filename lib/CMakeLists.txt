# /lib/CMakeLists.txt

# 정적 라이브러리를 만든다. 즉, main과 달리 여기는 라이브러리 파일들이므로, 라이브러리로 만들라고 써주는 거임
# add_library (<라이브러리 이름> [STATIC | SHARED | MODULE ] <소스 1> <소스 2> ...)
add_library(communication STATIC communication.cc)


# 해당 라이브러리 컴파일 시 사용할 헤더파일 경로
# 또한 shape을 참조하는 programe(target)을 컴파일 할때 아래 파일 경로가 자동으로 추가됨 
target_include_directories(communication PUBLIC ${CMAKE_SOURCE_DIR}/includes)



# fmt 라이브러리 찾기
find_package(fmt REQUIRED)

# fmt 라이브러리 링크
target_link_libraries(communication PRIVATE fmt::fmt)


# 해당 라이브러리를 컴파일 할 옵션
# public, private 각각 링크된 대상에게 계승되냐 안되냐의 차이.
# 각각 이 라이브러리를 사용하는 애들한테 옵션이 계승되냐, 안되냐 차이
# Wall, Werror 옵션은 컴파일시 진짜 오류만 컴파일 에러를 띄우냐 v/s 경고지점이 있으면 컴파일 에러를 띄움
target_compile_options(communication PRIVATE -Wall)

