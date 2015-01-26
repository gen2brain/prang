#!/usr/bin/env bash

set -e

./make.bash

adb install -r bin/Prang-debug.apk

adb shell am start -a android.intent.action.MAIN -n com.github.gen2brain.prang/.PrangActivity
