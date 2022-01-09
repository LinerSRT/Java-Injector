#include "pch.h"
#include "Java.h"

jclass getClass(JNIEnv* jniEnv, const char* name)
{
    return jniEnv->FindClass(name);
}

jmethodID getMethod(JNIEnv* jniEnv, jclass clazz, const char* name, const char* signature)
{
    return jniEnv->GetMethodID(clazz, name, signature);
}

jmethodID getMethod(JNIEnv* jniEnv, const char* className, const char* methodName, const char* signature)
{
    return jniEnv->GetMethodID((jclass)getClass(jniEnv, className), methodName, signature);
}

jmethodID getMethod(JNIEnv* jniEnv, jobject obj, const char* methodName, const char* signature)
{
    return getMethod(jniEnv, jniEnv->GetObjectClass(obj), methodName, signature);
}


jmethodID getStaticMethod(JNIEnv* jniEnv, jclass clazz, const char* name, const char* signature)
{
    return jniEnv->GetStaticMethodID(clazz, name, signature);
}

jmethodID getStaticMethod(JNIEnv* jniEnv, const char* className, const char* methodName, const char* signature)
{
    return jniEnv->GetStaticMethodID((jclass)getClass(jniEnv, className), methodName, signature);
}


jmethodID getStaticMethod(JNIEnv* jniEnv, jobject obj, const char* methodName, const char* signature)
{
    return getStaticMethod(jniEnv, jniEnv->GetObjectClass(obj), methodName, signature);
}

jobject getObject(JNIEnv* jniEnv, jclass clazz, jmethodID methodID, ...)
{
    jobject result;
    va_list args;
    va_start(args, methodID);
    result = jniEnv->NewObject(clazz, methodID, args);
    va_end(args);
    return result;
}

jobject getObject(JNIEnv* jniEnv, const char* className, const char* methodName, const char* signature, ...)
{
    jobject result;
    va_list args;
    va_start(args, signature);
    jclass clazz = getClass(jniEnv, className);
    jmethodID methodID = getMethod(jniEnv, clazz, methodName, signature);
    result = jniEnv->NewObject(clazz, methodID, args);
    va_end(args);
    return result;
}


jobject getObjectField(JNIEnv* jniEnv, const char* className, const char* fieldName, const char* signature) {
    jclass clazz = getClass(jniEnv, className);
    jfieldID filedID = jniEnv->GetFieldID(clazz, fieldName, signature);
    return jniEnv->GetObjectField(clazz, filedID);
}

jobject getObjectField(JNIEnv* jniEnv, jclass clazz, jfieldID filedID) {
    return jniEnv->GetObjectField(clazz, filedID);
}

jfieldID getObjectFieldID(JNIEnv* jniEnv, const char* className, const char* fieldName, const char* signature) {
    return getObjectFieldID(jniEnv, getClass(jniEnv, className), fieldName, signature);
}


jfieldID getObjectFieldID(JNIEnv* jniEnv, jclass clazz, const char* fieldName, const char* signature)
{
    return jniEnv->GetFieldID(clazz, fieldName, signature);
}
