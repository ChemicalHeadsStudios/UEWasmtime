// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Modules/ModuleManager.h"

THIRD_PARTY_INCLUDES_START
#include "wasi.h"
#include "wasmtime.h"

template<typename T>
struct TWASMVal
{
	static wasm_val_t GetType()
	{
		static_assert(!sizeof(T), "Variant trait must be specialized for this type.");
		wasm_val_t WasmValue;
		WasmValue.kind = WASM_I32;
		WasmValue.of.i32 = 0;
		return WasmValue;
	}
};

template<>
struct TWASMVal<int32_t>
{
	static wasm_val_t WASM_NEW_VAL(int32_t InValue)
	{
		wasm_val_t WasmValue;
		WasmValue.kind = WASM_I32;
		WasmValue.of.i32 = InValue;
		return WasmValue;
	}
};

template<>
struct TWASMVal<int64_t>
{
	static wasm_val_t WASM_NEW_VAL(int64_t InValue)
	{
		wasm_val_t WasmValue;
		WasmValue.kind = WASM_I64;
		WasmValue.of.i64 = InValue;
		return WasmValue;
	}
};

template<>
struct TWASMVal<wasm_ref_t*>
{
	static wasm_val_t WASM_NEW_VAL(wasm_ref_t* InValue)
	{
		wasm_val_t WasmValue;
		WasmValue.kind = WASM_ANYREF;
		WasmValue.of.ref = InValue;
		return WasmValue;
	}
};


THIRD_PARTY_INCLUDES_END

DECLARE_LOG_CATEGORY_EXTERN(LogUEWasmTime, Log, All);

class FUEWasmTimeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Handle to the test dll we will load */
	void*	WasmTimeHandle;
};
