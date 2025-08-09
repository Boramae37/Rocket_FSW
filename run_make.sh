#!/bin/bash
set -e

# 현재 디렉터리의 모든 하위 폴더를 순회합니다.
for APP_DIR in */; do
    # 해당 폴더 안에 CMakeLists.txt 파일이 있는지 확인합니다.
    if [ -f "${APP_DIR}/CMakeLists.txt" ]; then
        
        # --- 빌드 시작을 알리는 메시지 ---
        echo "======================================="
        echo "Processing App: ${APP_DIR}"
        echo "======================================="

        # 각 앱 폴더 내의 build 디렉터리 경로를 설정합니다.
        BUILD_DIR="${APP_DIR}build"

        # build 디렉터리가 없으면 생성합니다.
        mkdir -p "${BUILD_DIR}"

        # 1. CMake를 실행하여 빌드 파일을 생성합니다.
        # -S : 소스 디렉터리 (app 폴더)
        # -B : 빌드 디렉터리 (app/build 폴더)
        cmake -S "${APP_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release

        # 2. 실제 빌드를 실행합니다.
        #cmake --build "${BUILD_DIR}"

        echo "Build successful for: ${APP_DIR}"
        echo "" # 가독성을 위해 한 줄 띄우기
    fi
done

# chmod +x run_make.sh 실행해서 실행 파일로 만들어줄 것
echo "[sensor_app building]"
make -C sensor_app/build -j4

echo "[softwarebus_app building]"
make -C softwarebus_app/build -j4

echo "[communication_app building]"
make -C communication_app/build -j4

echo "[control_app building]"
make -C control_app/build -j4

echo "[supervisor_app building]"
make -C supervisor_app/build -j4