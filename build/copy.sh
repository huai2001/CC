#!/usr/bin/env bash
SRC_DIR=/opt/GitHub/CC/bin/aarch64
DST_DIR=/opt/GitHub/Mir2Server/bin/aarch64
cp -f -r ${SRC_DIR}/debug/libcc.widgets.dylib ${DST_DIR}/debug/
cp -f -r ${SRC_DIR}/release/libcc.widgets.dylib ${DST_DIR}/release/
cp -f -r ${SRC_DIR}/release/libcc.widgets.dylib ${DST_DIR}/