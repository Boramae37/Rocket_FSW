#!/bin/bash
set -e

# 1. pigpio 크로스 컴파일 (한 번만 실행)
if [ ! -f "external/pigpio/libpigpio.so" ]; then
    echo "Building pigpio for ARM64..."
    
    mkdir -p external
    cd external
    
    if [ ! -d "pigpio" ]; then
        git clone https://github.com/joan2937/pigpio.git
    fi
    
    cd pigpio
    
    # pigpio 크로스 컴파일
    make clean
    make CROSS_PREFIX=aarch64-linux-gnu- \
         CC=aarch64-linux-gnu-gcc \
         AR=aarch64-linux-gnu-ar \
         RANLIB=aarch64-linux-gnu-ranlib \
         STRIP=aarch64-linux-gnu-strip \
         -j$(nproc)
    
    cd ../..
    echo "✅ pigpio 크로스 컴파일 완료"
fi



# 현재 디렉터리의 모든 하위 폴더를 순회합니다.
for APP_DIR in */; do
    # 해당 폴더 안에 CMakeLists.txt 파일이 있는지 확인합니다.
    if [ -f "${APP_DIR}/CMakeLists.txt" ]; then
        
        # --- 빌드 시작을 알리는 메시지 ---
        echo "======================================="
        echo "Processing App: ${APP_DIR}"
        echo "======================================="

        # 각 앱 폴더 내의 build 디렉터리 경로를 설정합니다.
        BUILD_DIR="${APP_DIR}build-arm-cross"

        # build 디렉터리가 없으면 생성합니다.
        mkdir -p "${BUILD_DIR}"

        # 1. CMake를 실행하여 빌드 파일을 생성합니다.
        # -S : 소스 디렉터리 (app 폴더)
        # -B : 빌드 디렉터리 (app/build 폴더)

         # 툴체인 파일 경로를 절대 경로로 수정
        TOOLCHAIN_PATH="arm64-toolchain.cmake"

        cmake -S "${APP_DIR}" -B "${BUILD_DIR}" -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_PATH}" \
                                                -DCMAKE_BUILD_TYPE=Release

        # 2. 실제 빌드를 실행합니다.
        cmake --build "${BUILD_DIR}" --parallel $(nproc)

        echo "Build successful for: ${APP_DIR}"
        echo "" # 가독성을 위해 한 줄 띄우기
    fi
done

# chmod +x run_make.sh 실행해서 실행 파일로 만들어줄 것
# echo "[sensor_app building]"
# make -C sensor_app/build -j4

# echo "[softwarebus_app building]"
# make -C softwarebus_app/build -j4

# echo "[communication_app building]"
# make -C communication_app/build -j4

# echo "[control_app building]"
# make -C control_app/build -j4

# echo "[supervisor_app building]"
# make -C supervisor_app/build -j4