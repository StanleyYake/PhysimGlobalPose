#!/bin/bash
ps aux | grep [r]os | awk '{print $2}'
kill -9 $(ps aux | grep [r]os | grep -v grep | awk '{print $2}')
kill -9 $(ps aux | grep detect_bbox | grep -v grep | awk '{print $2}')
roscore&
source $PHYSIM_GLOBAL_POSE/devel/setup.sh
$PHYSIM_GLOBAL_POSE/src/3rdparty/rcnn_detection_package/bin/detect_bbox&