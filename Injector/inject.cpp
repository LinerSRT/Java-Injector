#include "pch.h"
#include <windows.h>
#include "utils.h"
#include <stdlib.h>  
#include "jni.h"
#include "Java.h"
#include <iostream>
#include "Logger.h"
#include "inject.h"
#pragma warning(disable:4996)


jstring getZipCommentFromBuffer(JNIEnv* env, jbyteArray buffer) {
	char endOfDirectoryFlag[] = { 0x50, 0x4b, 0x05, 0x06 };
	int endLength = sizeof(endOfDirectoryFlag) / sizeof(char);
	int bufferLength = env->GetArrayLength(buffer);
	jbyte* byteBuffer = env->GetByteArrayElements(buffer, JNI_FALSE);

	for (int i = bufferLength - endLength - 22; i >= 0; i--) {
		bool isEndOfDirectoryFlag = true;
		for (int k = 0; k < endLength; k++) {
			if (byteBuffer[i + k] != endOfDirectoryFlag[k]) {
				isEndOfDirectoryFlag = false;
				break;
			}
		}
		if (isEndOfDirectoryFlag) {
			int commentLen = byteBuffer[i + 20] + byteBuffer[i + 22] * 256;
			int realLen = bufferLength - i - 22;
			jclass stringClass = getClass(env, "java/lang/String");
			jmethodID createString = getMethod(env, stringClass, "<init>", "([BII)V");
			return (jstring)env->NewObject(stringClass, createString, buffer, i + 22, min(commentLen, realLen));
		}
	}

	return NULL;
}

typedef jobjectArray(JNICALL* JVM_GetAllThreads)(JNIEnv* env, jclass dummy);

std::string jstring2string(JNIEnv* env, jstring jStr) {
	if (!jStr)
		return "";

	const jclass stringClass = env->GetObjectClass(jStr);
	const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
	const jbyteArray stringJbytes = (jbyteArray)env->CallObjectMethod(jStr, getBytes, env->NewStringUTF("UTF-8"));

	size_t length = (size_t)env->GetArrayLength(stringJbytes);
	jbyte* pBytes = env->GetByteArrayElements(stringJbytes, NULL);

	std::string ret = std::string((char*)pBytes, length);
	env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

	env->DeleteLocalRef(stringJbytes);
	env->DeleteLocalRef(stringClass);
	return ret;
}

void inject(JNIEnv* jniEnv) {
	jclass fileClass = getClass(
		jniEnv,
		"java/io/File"
	);
	jclass filesClass = getClass(
		jniEnv,
		"java/nio/file/Files"
	);
	jclass stringClass = getClass(
		jniEnv,
		"java/lang/String"
	);
	jclass fileChooserClass = getClass(
		jniEnv,
		"javax/swing/JFileChooser"
	);
	jclass fileNameExtensionFilterClass = getClass(
		jniEnv,
		"javax/swing/filechooser/FileNameExtensionFilter"
	);
	jobject fileChooser = getObject(
		jniEnv,
		"javax/swing/JFileChooser",
		"<init>",
		"()V"
	);
	jmethodID splitString = getMethod(
		jniEnv,
		stringClass,
		"split", 
		"(Ljava/lang/String;)[Ljava/lang/String;"
	);
	jmethodID equalsString = getMethod(
		jniEnv,
		stringClass,
		"equals", 
		"(Ljava/lang/Object;)Z"
	);
	jmethodID setDialogTitle = getMethod(
		jniEnv,
		fileChooser,
		"setDialogTitle",
		"(Ljava/lang/String;)V"
	);
	jmethodID setAcceptAllFileFilterUsed = getMethod(
		jniEnv,
		fileChooser,
		"setAcceptAllFileFilterUsed",
		"(Z)V"
	);
	jmethodID addChoosableFileFilter = getMethod(
		jniEnv,
		fileChooser,
		"addChoosableFileFilter",
		"(Ljavax/swing/filechooser/FileFilter;)V"
	);
	jmethodID setCurrentDirectory = getMethod(
		jniEnv,
		fileChooser,
		"setCurrentDirectory", 
		"(Ljava/io/File;)V"
	);
	jmethodID showDialog = getMethod(
		jniEnv,
		fileChooser,
		"showDialog", 
		"(Ljava/awt/Component;Ljava/lang/String;)I"
	);
	jmethodID getSelectedFile = getMethod(
		jniEnv,
		fileChooser,
		"getSelectedFile",
		"()Ljava/io/File;"
	);
	jmethodID instanceFileFilter = getMethod(
		jniEnv,
		fileNameExtensionFilterClass,
		"<init>",
		"(Ljava/lang/String;[Ljava/lang/String;)V"
	);
	jmethodID instanceFile = getMethod(
		jniEnv,
		fileClass,
		"<init>", 
		"(Ljava/lang/String;)V"
	);
	jmethodID instanceFileA = getMethod(
		jniEnv,
		fileClass,
		"<init>", 
		"(Ljava/lang/String;Ljava/lang/String;)V"
	);
	jmethodID getParent = getMethod(
		jniEnv,
		fileClass,
		"getParent", 
		"()Ljava/lang/String;"
	);
	jmethodID getAbsolutePath = getMethod(
		jniEnv,
		fileClass,
		"getAbsolutePath", 
		"()Ljava/lang/String;"
	);
	jmethodID existsFile = getMethod(
		jniEnv,
		fileClass,
		"exists", 
		"()Z"
	);
	jmethodID toPath = getMethod(
		jniEnv,
		fileClass,
		"toPath", 
		"()Ljava/nio/file/Path;"
	);
	jmethodID readAllBytes = getStaticMethod(
		jniEnv,
		filesClass,
		"readAllBytes", 
		"(Ljava/nio/file/Path;)[B"
	);


	//Create string[] array with supported extensions for FileChooser
	jobjectArray extensions = jniEnv->NewObjectArray(2, stringClass, JNI_FALSE);
	jniEnv->SetObjectArrayElement(extensions, 0, jniEnv->NewStringUTF("zip"));
	jniEnv->SetObjectArrayElement(extensions, 1, jniEnv->NewStringUTF("jar"));
	jthrowable excetionThrow = jniEnv->ExceptionOccurred();
	if (excetionThrow) {
		printStackTrace(jniEnv, excetionThrow);
		jniEnv->ExceptionClear();
		//If an exception occured, stop injection
		return;
	}
	//Create FileFilter instance for FileChooser
	jobject fileFilter = jniEnv->NewObject(fileNameExtensionFilterClass, instanceFileFilter, jniEnv->NewStringUTF("ZIP or JAR file"), extensions);
	//Setting dialog title to FileChooser
	jniEnv->CallVoidMethod(fileChooser, setDialogTitle, jniEnv->NewStringUTF("Choose file to inject"));
	//Disable show all files in FileChooser
	jniEnv->CallVoidMethod(fileChooser, setAcceptAllFileFilterUsed, false);
	//Specify our file filter for FileChooser
	jniEnv->CallVoidMethod(fileChooser, addChoosableFileFilter, fileFilter);
	//Setting intial directory for FileChooser where is injector dll located
	jniEnv->CallVoidMethod(
		fileChooser, 
		setCurrentDirectory,
			jniEnv->NewObject(fileClass, instanceFile, jniEnv->NewStringUTF(getDllPath().c_str()))
	);
	excetionThrow = jniEnv->ExceptionOccurred();
	if (excetionThrow) {
		printStackTrace(jniEnv, excetionThrow);
		jniEnv->ExceptionClear();
		//If an exception occured, stop injection
		return;
	}
	//Create file instance for automatic injection
	jobject injectableFile = jniEnv->NewObject(
		fileClass, 
		instanceFileA, 
		jniEnv->NewStringUTF(getDllPath().c_str()),
		jniEnv->NewStringUTF("inject.jar")
	);
	excetionThrow = jniEnv->ExceptionOccurred();
	if (excetionThrow) {
		printStackTrace(jniEnv, excetionThrow);
		jniEnv->ExceptionClear();
		//If an exception occured, stop injection
		return;
	}

	jstring comment = NULL;
	do {
		if (!injectableFile) {
			//File for automatic injection not found, so open FileChooser dialog
			jint result = jniEnv->CallIntMethod(fileChooser, showDialog, NULL, jniEnv->NewStringUTF("Inject")); excetionThrow = jniEnv->ExceptionOccurred();
			if (excetionThrow) {
				printStackTrace(jniEnv, excetionThrow);
				jniEnv->ExceptionClear();
				//If an exception occured, stop injection
				return;
			}
			if (result == 0) {
				//Getting selected file from FileChooser dialog
				injectableFile = jniEnv->CallObjectMethod(fileChooser, getSelectedFile); excetionThrow = jniEnv->ExceptionOccurred();
				if (excetionThrow) {
					printStackTrace(jniEnv, excetionThrow);
					jniEnv->ExceptionClear();
					//If an exception occured, stop injection
					return;
				}
			} else {
				//If an exception occured while selecting file or user canceled selection, stop injection
				return;
			}
		}
		if (injectableFile) {
			//Check if file for injection exists
			jboolean fileExists = jniEnv->CallBooleanMethod(injectableFile, existsFile); excetionThrow = jniEnv->ExceptionOccurred();
			if (excetionThrow) {
				printStackTrace(jniEnv, excetionThrow);
				jniEnv->ExceptionClear();
				//If an exception occured, stop injection
				return;
			}
			if (!fileExists) {
				injectableFile = NULL;
			} else {
				//Read injecable file
				jobject allBytes = jniEnv->CallStaticObjectMethod(filesClass, readAllBytes, jniEnv->CallObjectMethod(injectableFile, toPath)); excetionThrow = jniEnv->ExceptionOccurred();
				if (excetionThrow) {
					printStackTrace(jniEnv, excetionThrow);
					jniEnv->ExceptionClear();
					//If an exception occured, stop injection
					return;
				}
				//Get comment from injecable file
				if (!(comment = getZipCommentFromBuffer(jniEnv, (jbyteArray)allBytes))) {
					injectableFile = NULL;
				}
			}
		}
	} while (!injectableFile);

	jobjectArray values = (jobjectArray)jniEnv->CallObjectMethod(comment, splitString, jniEnv->NewStringUTF("\r?\n"));
	jsize valuesLength = jniEnv->GetArrayLength(values);
	jstring commentClass = valuesLength > 0 ? (jstring)jniEnv->GetObjectArrayElement(values, 0) : NULL;
	jstring commentLoader = valuesLength > 1 ? (jstring)jniEnv->GetObjectArrayElement(values, 1) : NULL;

	jmethodID getName = jniEnv->GetMethodID(jniEnv->FindClass("java/lang/Class"), "getName", "()Ljava/lang/String;");

	JVM_GetAllThreads getAllThreads = (JVM_GetAllThreads)GetProcAddressPeb(GetModuleHandlePeb(L"jvm.dll"), "JVM_GetAllThreads");
	jobjectArray threadsArray = getAllThreads(jniEnv, NULL);
	int threadsCount = jniEnv->GetArrayLength(threadsArray);
	jobject* classLoaders = new jobject[threadsCount];

	int count = 0;
	for (int i = 0; i < threadsCount; i++) {
		jobject thread = jniEnv->GetObjectArrayElement(threadsArray, i);
		jclass threadCls = jniEnv->FindClass("java/lang/Thread");
		jfieldID ctxClsLoader = jniEnv->GetFieldID(threadCls, "contextClassLoader", "Ljava/lang/ClassLoader;");
		jobject classLoader = jniEnv->GetObjectField(thread, ctxClsLoader);
		if (classLoader) {
			bool valid = true;

			for (int j = 0; (j < count && count != 0); j++) {
				jstring threadClsLoader = (jstring)jniEnv->CallObjectMethod(jniEnv->GetObjectClass(classLoader), getName);
				jstring itClsLoader = (jstring)jniEnv->CallObjectMethod(jniEnv->GetObjectClass(classLoaders[j]), getName);
				if (jniEnv->CallBooleanMethod(threadClsLoader, equalsString, itClsLoader)) {
					valid = false;
					break;
				}
			}

			if (valid) {
				classLoaders[count++] = classLoader;
			}
		}
	}

	jobjectArray classNames = jniEnv->NewObjectArray(count, stringClass, NULL);
	jobject targetClsLoader = NULL;
	for (int i = 0; i < count; i++) {
		jstring itClassLoader = (jstring)jniEnv->CallObjectMethod(jniEnv->GetObjectClass(classLoaders[i]), getName);
		if (commentLoader && jniEnv->CallBooleanMethod(commentLoader, equalsString, itClassLoader)) {
			targetClsLoader = classLoaders[i];
			break;
		}
		jniEnv->SetObjectArrayElement(classNames, i, itClassLoader);
	}

	if (!targetClsLoader) {
		jclass JOptionPane = jniEnv->FindClass("javax/swing/JOptionPane");
		jmethodID showInputDialog = jniEnv->GetStaticMethodID(JOptionPane, "showInputDialog", "(Ljava/awt/Component;Ljava/lang/Object;Ljava/lang/String;ILjavax/swing/Icon;[Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
		jstring title = jniEnv->NewStringUTF("Choose class loader");

		do {
			jobject selectedClsLoader = jniEnv->CallStaticObjectMethod(NULL, showInputDialog, NULL, NULL, title, -1, NULL, classNames, NULL);

			if (selectedClsLoader) {
				for (int i = 0; i < count; i++) {
					jstring itClsName = (jstring)jniEnv->GetObjectArrayElement(classNames, i);

					if (jniEnv->CallBooleanMethod(itClsName, equalsString, selectedClsLoader)) {
						targetClsLoader = classLoaders[i];
						break;
					}
				}

				break;
			}
			else {
				return;
			}
		} while (true);
	}

	delete[] classLoaders;

	jclass urlClassLoaderCls = jniEnv->FindClass("java/net/URLClassLoader");
	jfieldID ucp = jniEnv->GetFieldID(urlClassLoaderCls, "ucp", "Lsun/misc/URLClassPath;");
	jobject ucpObject = jniEnv->GetObjectField(targetClsLoader, ucp);
	jclass urlClassPath = jniEnv->GetObjectClass(ucpObject);
	jfieldID urlsField = jniEnv->GetFieldID(urlClassPath, "urls", "Ljava/util/Stack;");
	jfieldID pathField = jniEnv->GetFieldID(urlClassPath, "path", "Ljava/util/ArrayList;");

	jobject urls = jniEnv->GetObjectField(ucpObject, urlsField);
	jobject path = jniEnv->GetObjectField(ucpObject, pathField);
	jclass stack = jniEnv->GetObjectClass(urls);
	jclass vector = jniEnv->GetSuperclass(stack);
	jclass arraylist = jniEnv->GetObjectClass(path);
	jmethodID addVector = jniEnv->GetMethodID(vector, "add", "(ILjava/lang/Object;)V");
	jmethodID addArrayList = jniEnv->GetMethodID(arraylist, "add", "(Ljava/lang/Object;)Z");

	jmethodID toURI = jniEnv->GetMethodID(fileClass, "toURI", "()Ljava/net/URI;");
	jobject uri = jniEnv->CallObjectMethod(injectableFile, toURI);
	jclass urlClass = jniEnv->GetObjectClass(uri);
	jmethodID toURL = jniEnv->GetMethodID(urlClass, "toURL", "()Ljava/net/URL;");
	jobject url = jniEnv->CallObjectMethod(uri, toURL);
	jniEnv->CallVoidMethod(urls, addVector, 0, url);
	jniEnv->CallBooleanMethod(path, addArrayList, url);
	jclass classLoader = jniEnv->FindClass("java/lang/ClassLoader");
	jmethodID loadClass = jniEnv->GetMethodID(classLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	jclass main = (jclass)jniEnv->CallObjectMethod(targetClsLoader, loadClass, commentClass);
	excetionThrow = jniEnv->ExceptionOccurred();
	if (!main || excetionThrow) {
		printStackTrace(jniEnv, excetionThrow);
		jniEnv->ExceptionClear();
		return;
	}
	jmethodID mainInit = jniEnv->GetMethodID(main, "<init>", "()V");
	if (!mainInit || jniEnv->ExceptionCheck()) {
		jniEnv->ExceptionClear();
		//MessageBox(NULL, L"Init constructor not found.", L"Error", MB_OK);
		return;
	}
	jniEnv->NewObject(main, mainInit);

	//MessageBox(NULL, L"JavaInjector by H2Eng [vk.com/h2eng]", L"Cheat loaded successfully", MB_OK | MB_SYSTEMMODAL);
}

void printStackTrace(JNIEnv* jniEnv, jthrowable throwable)
{
	jmethodID getMessage = getMethod(
		jniEnv,
		"java/lang/Throwable",
		"getMessage",
		"()Ljava/lang/String;"
	);
	jmethodID getCause = getMethod(
		jniEnv,
		"java/lang/Throwable",
		"getCause",
		"()Ljava/lang/Throwable;"
	);
	jmethodID getStackTrace = getMethod(
		jniEnv,
		"java/lang/Throwable",
		"getStackTrace",
		"()[Ljava/lang/StackTraceElement;"
	);
	jmethodID toString = getMethod(
		jniEnv,
		"java/lang/StackTraceElement",
		"toString",
		"()Ljava/lang/String;"
	);

	jstring message = (jstring)jniEnv->CallObjectMethod(throwable, getMessage);
	writeLog("[ERROR] An exception occured in: " + jstring2string(jniEnv, message) + "\n");
	jobjectArray stackTrace = (jobjectArray)jniEnv->CallObjectMethod(
		throwable,
		getStackTrace
	);
	jsize stackTraceLength = jniEnv->GetArrayLength(stackTrace);
	if (stackTrace) {
		jstring stackTraceString = (jstring)jniEnv->CallObjectMethod(
			throwable,
			toString
		);
		const char* msg_str = jniEnv->GetStringUTFChars(stackTraceString, 0);
		writeLog("Exception caused by: " + jstring2string(jniEnv, stackTraceString) + " \n");
	}
	if (stackTraceLength > 0)
	{
		jsize i = 0;
		for (i = 0; i < stackTraceLength; i++)
		{
			jobject trace = jniEnv->GetObjectArrayElement(stackTrace, i);
			jstring stackTraceString = (jstring)jniEnv->CallObjectMethod(trace, toString);
			writeLog("    " + jstring2string(jniEnv, stackTraceString) + " \n");
		}
	}
}

