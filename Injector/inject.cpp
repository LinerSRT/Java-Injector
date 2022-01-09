#include "pch.h"
#include <windows.h>
#include <stdlib.h>  
#include <iostream>
#include "utils.h"
#include "jni.h"
#include "Java.h"
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


void initStruct(JNIEnv* jniEnv)
{
	Classes classes = getClasses(jniEnv);
	Methods methods = getMethods(jniEnv, classes);
	prepareInject(jniEnv, classes, methods);
}

void prepareInject(JNIEnv* jniEnv, Classes classes, Methods methods) {
	jobject fileChooser = getObject(
		jniEnv,
		"javax/swing/JFileChooser",
		"<init>",
		"()V"
	);
	//Create string[] array with supported extensions for FileChooser
	jobjectArray extensions = jniEnv->NewObjectArray(2, classes.stringClass, JNI_FALSE);
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
	jobject fileFilter = jniEnv->NewObject(classes.fileNameExtensionFilterClass, methods.instanceFileFilter, jniEnv->NewStringUTF("ZIP or JAR file"), extensions);
	//Setting dialog title to FileChooser
	jniEnv->CallVoidMethod(fileChooser, methods.setDialogTitle, jniEnv->NewStringUTF("Choose file to inject"));
	//Disable show all files in FileChooser
	jniEnv->CallVoidMethod(fileChooser, methods.setAcceptAllFileFilterUsed, false);
	//Specify our file filter for FileChooser
	jniEnv->CallVoidMethod(fileChooser, methods.addChoosableFileFilter, fileFilter);
	//Setting intial directory for FileChooser where is injector dll located
	jniEnv->CallVoidMethod(
		fileChooser,
		methods.setCurrentDirectory,
		jniEnv->NewObject(classes.fileClass, methods.instanceFile, jniEnv->NewStringUTF(getDllPath().c_str()))
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
		classes.fileClass,
		methods.instanceFileA,
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
			jint result = jniEnv->CallIntMethod(fileChooser, methods.showDialog, NULL, jniEnv->NewStringUTF("Inject")); excetionThrow = jniEnv->ExceptionOccurred();
			if (excetionThrow) {
				printStackTrace(jniEnv, excetionThrow);
				jniEnv->ExceptionClear();
				//If an exception occured, stop injection
				return;
			}
			if (result == 0) {
				//Getting selected file from FileChooser dialog
				injectableFile = jniEnv->CallObjectMethod(fileChooser, methods.getSelectedFile); excetionThrow = jniEnv->ExceptionOccurred();
				if (excetionThrow) {
					printStackTrace(jniEnv, excetionThrow);
					jniEnv->ExceptionClear();
					//If an exception occured, stop injection
					return;
				}
			}
			else {
				//If an exception occured while selecting file or user canceled selection, stop injection
				return;
			}
		}
		if (injectableFile) {
			//Check if file for injection exists
			jboolean fileExists = jniEnv->CallBooleanMethod(injectableFile, methods.existsFile); excetionThrow = jniEnv->ExceptionOccurred();
			if (excetionThrow) {
				printStackTrace(jniEnv, excetionThrow);
				jniEnv->ExceptionClear();
				//If an exception occured, stop injection
				return;
			}
			if (!fileExists) {
				injectableFile = NULL;
			}
			else {
				//Read injecable file
				jobject allBytes = jniEnv->CallStaticObjectMethod(classes.filesClass, methods.readAllBytes, jniEnv->CallObjectMethod(injectableFile, methods.toPath)); excetionThrow = jniEnv->ExceptionOccurred();
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

	jobjectArray values = (jobjectArray)jniEnv->CallObjectMethod(comment, methods.splitString, jniEnv->NewStringUTF("\r?\n"));
	jsize valuesLength = jniEnv->GetArrayLength(values);
	jstring commentClass = valuesLength > 0 ? (jstring)jniEnv->GetObjectArrayElement(values, 0) : NULL;
	jstring commentLoader = valuesLength > 1 ? (jstring)jniEnv->GetObjectArrayElement(values, 1) : NULL;
	injectFile(jniEnv,
		classes,
		methods,
		injectableFile,
		commentClass,
		commentLoader);
}

jobject getTargetClassLoader(JNIEnv* jniEnv, Classes classes, Methods methods, jstring classLoaderName) {
	jobject targetClassLoader = NULL;
	JVM_GetAllThreads getAllThreads = (JVM_GetAllThreads)GetProcAddressPeb(GetModuleHandlePeb(L"jvm.dll"), "JVM_GetAllThreads");
	jobjectArray threadsArray = getAllThreads(jniEnv, NULL);
	int threadsCount = jniEnv->GetArrayLength(threadsArray);
	jobject* classLoaders = new jobject[threadsCount];

	int count = 0;
	for (int i = 0; i < threadsCount; i++) {
		jobject thread = jniEnv->GetObjectArrayElement(threadsArray, i);
		jfieldID ctxClsLoader = jniEnv->GetFieldID(classes.threadClass, "contextClassLoader", "Ljava/lang/ClassLoader;");
		jobject classLoader = jniEnv->GetObjectField(thread, ctxClsLoader);
		if (classLoader) {
			bool valid = true;

			for (int j = 0; (j < count && count != 0); j++) {
				jstring threadClsLoader = (jstring)jniEnv->CallObjectMethod(jniEnv->GetObjectClass(classLoader), methods.getName);
				jstring itClsLoader = (jstring)jniEnv->CallObjectMethod(jniEnv->GetObjectClass(classLoaders[j]), methods.getName);
				if (jniEnv->CallBooleanMethod(threadClsLoader, methods.equalsString, itClsLoader)) {
					valid = false;
					break;
				}
			}

			if (valid) {
				classLoaders[count++] = classLoader;
			}
		}
	}

	jobjectArray classNames = jniEnv->NewObjectArray(count, classes.stringClass, NULL);
	for (int i = 0; i < count; i++) {
		jstring itClassLoader = (jstring)jniEnv->CallObjectMethod(jniEnv->GetObjectClass(classLoaders[i]), methods.getName);
		if (classLoaderName && jniEnv->CallBooleanMethod(classLoaderName, methods.equalsString, itClassLoader)) {
			targetClassLoader = classLoaders[i];
			break;
		}
		jniEnv->SetObjectArrayElement(classNames, i, itClassLoader);
	}
	if (!targetClassLoader) {
		jclass JOptionPane = jniEnv->FindClass("javax/swing/JOptionPane");
		jmethodID showInputDialog = jniEnv->GetStaticMethodID(JOptionPane, "showInputDialog", "(Ljava/awt/Component;Ljava/lang/Object;Ljava/lang/String;ILjavax/swing/Icon;[Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
		jstring title = jniEnv->NewStringUTF("Choose class loader");

		do {
			jobject selectedClsLoader = jniEnv->CallStaticObjectMethod(NULL, showInputDialog, NULL, NULL, title, -1, NULL, classNames, NULL);

			if (selectedClsLoader) {
				for (int i = 0; i < count; i++) {
					jstring itClsName = (jstring)jniEnv->GetObjectArrayElement(classNames, i);

					if (jniEnv->CallBooleanMethod(itClsName, methods.equalsString, selectedClsLoader)) {
						targetClassLoader = classLoaders[i];
						break;
					}
				}

				break;
			}
			else {
				return targetClassLoader;
			}
		} while (true);
	}

	delete[] classLoaders;
	return targetClassLoader;
}

void injectFile(JNIEnv* jniEnv, Classes classes, Methods methods, jobject injectFile, jstring className, jstring classLoaderName) {
	jobject targetClsLoader = getTargetClassLoader(jniEnv, classes, methods, classLoaderName);
	jthrowable excetionThrow = jniEnv->ExceptionOccurred();
	jobject uri = jniEnv->CallObjectMethod(injectFile, methods.toURI);
	jobject url = jniEnv->CallObjectMethod(uri, methods.toURL);
	jniEnv->CallVoidMethod(targetClsLoader, methods.addURL, url);
	if (excetionThrow) {
		printStackTrace(jniEnv, excetionThrow);
		jniEnv->ExceptionClear();
		return;
	}
	jclass main = (jclass)jniEnv->CallObjectMethod(targetClsLoader, methods.loadClass, className);
	excetionThrow = jniEnv->ExceptionOccurred();
	if (!main || excetionThrow) {
		printStackTrace(jniEnv, excetionThrow);
		jniEnv->ExceptionClear();
		return;
	}
	jmethodID mainInit = jniEnv->GetMethodID(main, "<init>", "()V");
	if (!mainInit || jniEnv->ExceptionCheck()) {
		jniEnv->ExceptionClear();
		return;
	}
	jniEnv->NewObject(main, mainInit);
}

Classes getClasses(JNIEnv* jniEnv)
{
	Classes classes = {
		jniEnv,
		getClass(
			jniEnv,
			"java/io/File"
		),
		getClass(
			jniEnv,
			"java/nio/file/Files"
		),
		getClass(
			jniEnv,
			"java/lang/String"
		),
		getClass(
			jniEnv,
			"javax/swing/JFileChooser"
		),
		getClass(
			jniEnv,
			"javax/swing/filechooser/FileNameExtensionFilter"
		),
		getClass(
			jniEnv,
			"java/lang/Class"
		),
		getClass(
			jniEnv,
			"java/lang/Thread"
		),
		getClass(
			jniEnv,
			"java/net/URLClassLoader"
		),
		getClass(
			jniEnv,
			"java/lang/ClassLoader"
		),
		getClass(
			jniEnv,
			"java/net/URI"
		),
		getClass(
			jniEnv,
			"java/net/URL"
		)
	};
	return classes;
}

Methods getMethods(JNIEnv* jniEnv, Classes classes)
{
	Methods methods = {
		jniEnv,
		getMethod(
			jniEnv,
			classes.stringClass,
			"split",
			"(Ljava/lang/String;)[Ljava/lang/String;"
		),
		getMethod(
			jniEnv,
			classes.stringClass,
			"equals",
			"(Ljava/lang/Object;)Z"
		),
		getMethod(
			jniEnv,
			classes.fileChooserClass,
			"setDialogTitle",
			"(Ljava/lang/String;)V"
		),
		getMethod(
			jniEnv,
			classes.fileChooserClass,
			"setAcceptAllFileFilterUsed",
			"(Z)V"
		),
		getMethod(
			jniEnv,
			classes.fileChooserClass,
			"addChoosableFileFilter",
			"(Ljavax/swing/filechooser/FileFilter;)V"
		),
		getMethod(
			jniEnv,
			classes.fileChooserClass,
			"setCurrentDirectory",
			"(Ljava/io/File;)V"
		),
		getMethod(
			jniEnv,
			classes.fileChooserClass,
			"showDialog",
			"(Ljava/awt/Component;Ljava/lang/String;)I"
		),
		getMethod(
			jniEnv,
			classes.fileChooserClass,
			"getSelectedFile",
			"()Ljava/io/File;"
		),
		getMethod(
			jniEnv,
			classes.fileNameExtensionFilterClass,
			"<init>",
			"(Ljava/lang/String;[Ljava/lang/String;)V"
		),
		getMethod(
			jniEnv,
			classes.fileClass,
			"<init>",
			"(Ljava/lang/String;)V"
		),
		getMethod(
			jniEnv,
			classes.fileClass,
			"<init>",
			"(Ljava/lang/String;Ljava/lang/String;)V"
		),
		getMethod(
			jniEnv,
			classes.fileClass,
			"getParent",
			"()Ljava/lang/String;"
		),
		getMethod(
			jniEnv,
			classes.fileClass,
			"getAbsolutePath",
			"()Ljava/lang/String;"
		),
		getMethod(
			jniEnv,
			classes.fileClass,
			"exists",
			"()Z"
		),
		getMethod(
			jniEnv,
			classes.fileClass,
			"toPath",
			"()Ljava/nio/file/Path;"
		),
		getStaticMethod(
			jniEnv,
			classes.filesClass,
			"readAllBytes",
			"(Ljava/nio/file/Path;)[B"
		),
		getMethod(
			jniEnv,
			classes.classClass,
			"getName", "()Ljava/lang/String;"
		),
		getMethod(
			jniEnv,
			classes.fileClass,
			"toURI", "()Ljava/net/URI;"
		),
		getMethod(
			jniEnv,
			classes.uriClass,
			"toURL", "()Ljava/net/URL;"
		),
		getMethod(
			jniEnv,
			classes.urlClass,
			"toString", "()Ljava/lang/String;"
		),
		getMethod(
			jniEnv,
			classes.urlClassLoader,
			"addURL", "(Ljava/net/URL;)V"
		),
		getMethod(
			jniEnv,
			classes.classLoader,
			"loadClass", "(Ljava/lang/String;)Ljava/lang/Class;"
		)
	};
	return methods;
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

