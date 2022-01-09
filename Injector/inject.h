#include <jni.h>
#pragma once
#include <iostream>
using namespace std;

struct Classes
{
	JNIEnv* jniEnv;
	jclass fileClass;
	jclass filesClass;
	jclass stringClass;
	jclass fileChooserClass;
	jclass fileNameExtensionFilterClass;
	jclass classClass;
	jclass threadClass;
	jclass urlClassLoader;
	jclass classLoader;
	jclass uriClass;
	jclass urlClass;
};

struct Methods
{
	JNIEnv* jniEnv;
	jmethodID splitString;
	jmethodID equalsString;
	jmethodID setDialogTitle;
	jmethodID setAcceptAllFileFilterUsed;
	jmethodID addChoosableFileFilter;
	jmethodID setCurrentDirectory;
	jmethodID showDialog;
	jmethodID getSelectedFile;
	jmethodID instanceFileFilter;
	jmethodID instanceFile;
	jmethodID instanceFileA;
	jmethodID getParent;
	jmethodID getAbsolutePath;
	jmethodID existsFile;
	jmethodID toPath;
	jmethodID readAllBytes;
	jmethodID getName;
	jmethodID toURI;
	jmethodID toURL;
	jmethodID urlToString;
	jmethodID addURL;
	jmethodID loadClass;
};

Classes getClasses(JNIEnv* jniEnv);
Methods getMethods(JNIEnv* jniEnv, Classes classes);

void initStruct(JNIEnv* jniEnv);
void prepareInject(JNIEnv* jniEnv, Classes classes, Methods methods);
jobject getTargetClassLoader(JNIEnv* jniEnv, Classes classes, Methods methods, jstring classLoaderName);
void injectFile(JNIEnv* jniEnv, Classes classes, Methods methods, jobject injectFile, jstring className, jstring classLoaderName);


void printStackTrace(JNIEnv* jniEnv, jthrowable throwable);
string jstring2string(JNIEnv* env, jstring jStr);
