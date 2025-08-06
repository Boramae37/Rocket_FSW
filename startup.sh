#!/bin/bash

sudo killall pigpiod

sudo pigpiod

cd supervisor_app/build/

./supervisor
