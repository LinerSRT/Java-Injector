#pragma once
#include "jni.h"
jstring GetSystemPackage_Hook(JNIEnv* env, jstring name);


void createConsole();