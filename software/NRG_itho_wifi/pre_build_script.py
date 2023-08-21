GIT_REPO = "https://github.com/joltwallet/esp_littlefs.git#v1.8.0"

import os
from sys import platform

Import("env")

PROJECT_DIR = env.subst("$PROJECT_DIR")

if platform == "linux":
   print('Detected platform: Linux')
   DIR_SEPERATOR = '/'
elif platform == "darwin":
   print('Detected platform: MacOS')
   DIR_SEPERATOR = '/'
elif platform == "win32":
   print('Detected platform: Windows')
   DIR_SEPERATOR = '\\'
else:
   print('Detected platform: Failed!')
   exit()

def update_components():
   COMPONENT_DIR = PROJECT_DIR + DIR_SEPERATOR + 'components'
   LITTLEFS_COMP_DIR = COMPONENT_DIR + DIR_SEPERATOR + 'esp_littlefs'
   if not os.path.exists(LITTLEFS_COMP_DIR):
      print("Cloning LittleFS repo... ")
      GIT_SUB_ADD = "git submodule add " + GIT_REPO + " " + COMPONENT_DIR
      GIT_SUB_INIT = "git submodule update --init --recursive " + LITTLEFS_COMP_DIR
      #env.Execute(GIT_SUB_ADD)
      env.Execute(GIT_SUB_INIT)
   else:
      print("Checking for LittleFS repo updates...")
      GIT_UPDATE_COMMAND = 'git submodule update --recursive --init --remote ' + LITTLEFS_COMP_DIR
      env.Execute(GIT_UPDATE_COMMAND)

def build_before(*args, **kwargs):
   print('\n### running pre build commands...\n')
   update_components()

env.Execute(build_before)


