#!/bin/bash
# 스크립트 실행 중 오류가 발생하면 즉시 중단
set -e

echo "기존 pigpiod 프로세스를 종료합니다..."
# || true를 추가하여 프로세스가 없어도 스크립트가 중단되지 않도록 함
sudo killall pigpiod || true

echo "pigpiod 데몬을 시작합니다..."
sudo pigpiod

echo "pigpiod가 준비될 때까지 1초간 대기합니다..."
sleep 1

# 스크립트 파일이 위치한 디렉터리를 기준으로 경로 설정 (더 안정적)
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
APP_PATH="${SCRIPT_DIR}/run/supervisor"

if [ -f "$APP_PATH" ]; then
    echo "START supervisor..."
    cd "${SCRIPT_DIR}/run/"
    ./supervisor
else
    echo "오류: supervisor 실행 파일을 찾을 수 없습니다: ${APP_PATH}"
    exit 1
fi

echo "스크립트 완료."