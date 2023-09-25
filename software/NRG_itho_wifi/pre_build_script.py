GIT_REPO = "https://github.com/joltwallet/esp_littlefs.git#v1.8.0"

import os
from sys import platform

Import("env")

PROJECT_DIR = env.subst("$PROJECT_DIR")

if platform == "linux":
    print("Detected platform: Linux")
elif platform == "darwin":
    print("Detected platform: MacOS")
elif platform == "win32":
    print("Detected platform: Windows")
else:
    print("Detected platform: Failed!")
    exit()


def update_components():
    COMPONENT_DIR = os.path.join(PROJECT_DIR, "components")
    LITTLEFS_COMP_DIR = os.path.join(COMPONENT_DIR, "esp_littlefs")
    if not os.path.exists(LITTLEFS_COMP_DIR):
        print("Cloning LittleFS repo... ")
        GIT_SUB_ADD = "git submodule add " + GIT_REPO + " " + COMPONENT_DIR
        GIT_SUB_INIT = "git submodule update --init --recursive " + LITTLEFS_COMP_DIR
        # env.Execute(GIT_SUB_ADD)
        env.Execute(GIT_SUB_INIT)
    else:
        print("Checking for LittleFS repo updates...")
        GIT_UPDATE_COMMAND = (
            "git submodule update --recursive --init --remote " + LITTLEFS_COMP_DIR
        )
        env.Execute(GIT_UPDATE_COMMAND)


def build_before(*args, **kwargs):
    print("\n### running pre build commands...\n")
    update_components()


env.Execute(build_before)
