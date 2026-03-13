#!/bin/bash
pip install ninja meson
pip install -r requirements.txt
rm -rf build
meson setup build
meson compile -C build