#!/bin/bash

sudo killall pigpiod

sudo pigpiod

./supervisor_app/build/supervisor

