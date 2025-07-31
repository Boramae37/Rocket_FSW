#!/bin/bash

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