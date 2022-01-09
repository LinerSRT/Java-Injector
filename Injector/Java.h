#pragma once
#include "jni.h"

jclass getClass(JNIEnv* jniEnv, const char* name);
jmethodID getMethod(JNIEnv* jniEnv, jclass clazz, const char* name, const char* signature);
jmethodID getMethod(JNIEnv* jniEnv, const char* className, const char* methodName, const char* signature);
jmethodID getMethod(JNIEnv* jniEnv, jobject obj, const char* methodName, const char* signature);
jmethodID getStaticMethod(JNIEnv* jniEnv, jclass clazz, const char* name, const char* signature);
jmethodID getStaticMethod(JNIEnv* jniEnv, const char* className, const char* methodName, const char* signature);
jmethodID getStaticMethod(JNIEnv* jniEnv, jobject obj, const char* methodName, const char* signature);
jobject getObject(JNIEnv* jniEnv, jclass clazz, jmethodID methodID, ...);
jobject getObject(JNIEnv* jniEnv, const char* className, const char* methodName, const char* signature, ...);
jobject getObjectField(JNIEnv* jniEnv, const char* className, const char* fieldName, const char* signature);
jobject getObjectField(JNIEnv* jniEnv, jclass clazz, jfieldID filedID);
jfieldID getObjectFieldID(JNIEnv* jniEnv, const char* className, const char* fieldName, const char* signature);
jfieldID getObjectFieldID(JNIEnv* jniEnv, jclass clazz, const char* fieldName, const char* signature);


