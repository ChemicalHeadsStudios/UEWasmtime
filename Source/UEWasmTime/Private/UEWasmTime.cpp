// Copyright SIA CHEMICAL HEADS 2020

#include "UEWasmTime.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FUEWasmTimeModule"

DEFINE_LOG_CATEGORY(LogUEWasmTime);

void FUEWasmTimeModule::StartupModule()
{
	const FString& BinaryPath = FPaths::Combine(IPluginManager::Get().FindPlugin("UEWasmTime")->GetBaseDir(),
														TEXT("Binaries"), TEXT("ThirdParty"), TEXT("UEWasmTimeLibrary"));
	FString Handle = TEXT("wasmtime.dll");
	FString Platform = FString::Printf(TEXT("wasmtime-%s-x86_64-windows-c-api"), TEXT(WASMTIME_VERSION));

#if PLATFORM_LINUX
	Handle = TEXT("libwasmtime.so");
	Platform = FString::Printf(TEXT("wasmtime-%s-x86_64-linux-c-api"), TEXT(WASMTIME_VERSION));
#endif


	const FString& LibraryPath = FPaths::Combine(BinaryPath, Platform, TEXT("lib"), Handle);
	UE_LOG(LogUEWasmTime, Log, TEXT("Loading shared library located at %s"), *LibraryPath);
	void* DllHandle = FPlatformProcess::GetDllHandle(*LibraryPath);
	if (DllHandle)
	{
		UE_LOG(LogUEWasmTime, Log, TEXT("Successfully loaded shared library: %s"), *Handle);
		WasmTimeHandle = DllHandle;
	}
	else
	{
		UE_LOG(LogUEWasmTime, Log, TEXT("Failed to load shared library: %s"), *Handle);
	}
	
}

void FUEWasmTimeModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUEWasmTimeModule, UEWasmTime)
