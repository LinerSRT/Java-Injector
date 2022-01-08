#include "pch.h"
#include "dllmain.h"
#include "cheat.h"
#include "utils.h"
#include "dllmain.h"
#include <iostream>
#include <HookLib.h>

PVOID unload(PVOID arg) {
	HMODULE hm = NULL;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPWSTR)&unload, &hm);
	FreeLibraryAndExitThread(hm, 0);
}

typedef void(*JVM_MonitorNotify)(JNIEnv* env, jobject obj);

JVM_MonitorNotify MonitorNotify = NULL;

void MonitorNotify_Hook(JNIEnv* env, jobject obj) {
	MonitorNotify(env, obj);
	JNINativeMethod methods[] = { (PCHAR)"notify", (PCHAR)"()V", (PVOID)MonitorNotify };
	env->RegisterNatives(env->FindClass("java/lang/Object"), methods, sizeof(methods) / sizeof(JNINativeMethod));
	cheat(env);
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)unload, NULL, 0, NULL);
}

typedef jstring(*JVM_GetSystemPackage)(JNIEnv* env, jstring name);

JVM_GetSystemPackage GetSystemPackage = NULL;


jstring GetSystemPackage_Hook(JNIEnv* env, jstring name) {
	name = GetSystemPackage(env, name);
	RemoveHook(GetSystemPackage);
	JNINativeMethod methods[] = { (char*)"notify", (char*)"()V", (void*)&MonitorNotify_Hook };
	env->RegisterNatives(env->FindClass("java/lang/Object"), methods, sizeof(methods) / sizeof(JNINativeMethod));
	return name;
}



PVOID WINAPI hookThread(PVOID arg) {
	HMODULE jvm = GetModuleHandlePeb(L"jvm.dll");
	MonitorNotify = (JVM_MonitorNotify)GetProcAddressPeb(jvm, "JVM_MonitorNotify");
	JVM_GetSystemPackage getSystemPackage = (JVM_GetSystemPackage)GetProcAddressPeb(jvm, "JVM_GetSystemPackage");
	SetHook(getSystemPackage, GetSystemPackage_Hook, reinterpret_cast<PVOID*>(&GetSystemPackage));
	return NULL;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	DisableThreadLibraryCalls(hModule);

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)hookThread, NULL, 0, NULL);
		break;
	}
	return TRUE;
}