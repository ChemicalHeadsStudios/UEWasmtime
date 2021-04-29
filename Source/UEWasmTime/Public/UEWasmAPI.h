#pragma once

THIRD_PARTY_INCLUDES_START
#include "wasmtime.h"
THIRD_PARTY_INCLUDES_END


#define DECLARE_CUSTOM_WASMTYPE(Name, WasmType, DeleterFunction) \
struct T##Name##CustomDeleter : TDefaultDelete<WasmType> \
{\
	bool bDontDelete;\
	T##Name##CustomDeleter() = default;\
	T##Name##CustomDeleter(bool bInDontDelete)\
	{\
		bDontDelete = bInDontDelete;\
	}\
 	void operator()(WasmType* Ptr) const\
	{\
		if (Ptr)\
		{\
			if(!bDontDelete)\
			{\
				DeleterFunction(Ptr);\
			}\
			Ptr = nullptr;\
		}\
	}\
};\
typedef TUniquePtr<WasmType, T##Name##CustomDeleter> T##Name
	

FORCEINLINE void PrintError(const FString& Caller, wasmtime_error_t* ErrorPointer, wasm_trap_t* TrapPointer)
{
	wasm_byte_vec_t ErrorMessage = {0, nullptr};
	if (ErrorPointer != nullptr)
	{
		wasmtime_error_message(ErrorPointer, &ErrorMessage);
		wasmtime_error_delete(ErrorPointer);
	}
	else if (TrapPointer != nullptr)
	{
		wasm_trap_message(TrapPointer, &ErrorMessage);
		wasm_trap_delete(TrapPointer);
	}

	if (ErrorMessage.data != nullptr)
	{
		const FString& ErrorString = FString(ErrorMessage.size, ErrorMessage.data);
		UE_LOG(LogBadLadsPlugin, Error, TEXT("WASMError: (%s) %s"), *Caller, *ErrorString);
		checkf(false, TEXT("WASMError: (%s) %s"), *Caller, *ErrorString);
		wasm_byte_vec_delete(&ErrorMessage);
	}
}

namespace UEWas
{
	template <typename T>
	struct TWasmValue
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

	template <>
	struct TWasmValue<int32_t>
	{
		static wasm_val_t WASM_NEW_VAL(int32_t InValue)
		{
			wasm_val_t WasmValue;
			WasmValue.kind = WASM_I32;
			WasmValue.of.i32 = InValue;
			return WasmValue;
		}
	};

	template <>
	struct TWasmValue<int64_t>
	{
		static wasm_val_t WASM_NEW_VAL(int64_t InValue)
		{
			wasm_val_t WasmValue;
			WasmValue.kind = WASM_I64;
			WasmValue.of.i64 = InValue;
			return WasmValue;
		}
	};

	template <>
	struct TWasmValue<wasm_ref_t*>
	{
		static wasm_val_t WASM_NEW_VAL(wasm_ref_t* InValue)
		{
			wasm_val_t WasmValue;
			WasmValue.kind = WASM_ANYREF;
			WasmValue.of.ref = InValue;
			return WasmValue;
		}
	};

	template <typename T>
	struct TWasmRef
	{
		T Value;
	};

	template <typename T>
	struct TWasmGenericDeleter : TDefaultDelete<T>
	{
		
		TFunction<void(T*)> CustomDeleter;
		
		TWasmGenericDeleter(TFunction<void(T*)>&& InCustomDeleter)
		{
			CustomDeleter = MoveTemp(InCustomDeleter);
		}

		void operator()(T* Ptr) const
		{
			if (Ptr)
			{
				check(CustomDeleter);
				CustomDeleter(Ptr);
				Ptr = nullptr;
			}
		}
	};


	DECLARE_CUSTOM_WASMTYPE(WasiConfig, wasi_config_t, wasi_config_delete);
	DECLARE_CUSTOM_WASMTYPE(WasiInstance, wasi_instance_t, wasi_instance_delete);

	DECLARE_CUSTOM_WASMTYPE(WasmStore, wasm_store_t, wasm_store_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmInstance, wasm_instance_t, wasm_instance_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmEngine, wasm_engine_t, wasm_engine_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmModule, wasm_module_t, wasm_module_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmFuncType, wasm_functype_t, wasm_functype_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmFunc, wasm_func_t, wasm_func_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmLinker, wasmtime_linker_t, wasmtime_linker_delete);
	
	typedef TUniquePtr<TWasmRef<wasm_byte_vec_t>, TWasmGenericDeleter<TWasmRef<wasm_byte_vec_t>>> TWasmByteVec;
	typedef TUniquePtr<TWasmRef<wasm_extern_vec_t>, TWasmGenericDeleter<TWasmRef<wasm_extern_vec_t>>> TWasmExternVec;
	typedef TUniquePtr<TWasmRef<wasm_valtype_vec_t>, TWasmGenericDeleter<TWasmRef<wasm_valtype_vec_t>>> TWasmValTypeVec;
	typedef wasm_extern_t* TWasmExtern;
	typedef TWasmByteVec TWasmName;

	FORCEINLINE TWasmName MakeWasmName(const FString& InString)
	{
		auto NamePtr = new TWasmRef<wasm_name_t>();
		if(!InString.IsEmpty())
		{
			wasm_name_new_from_string(&NamePtr->Value, TCHAR_TO_UTF8(*InString));
		} else
		{
			wasm_name_new_empty(&NamePtr->Value);
		}
		return TWasmName(NamePtr, TWasmGenericDeleter<TWasmRef<wasm_name_t>>([](TWasmRef<wasm_name_t>* VecHandle)
		{
			wasm_name_delete(&VecHandle->Value);
			delete VecHandle;
		}));
	};
	
	FORCEINLINE TWasmExternVec WasmGetInstanceInstanceExports(const TWasmInstance& Instance)
	{
		check(Instance.Get());
		auto ExternVec = new TWasmRef<wasm_extern_vec_t>();
		wasm_instance_exports(Instance.Get(), &ExternVec->Value);
		return TWasmExternVec(ExternVec, TWasmGenericDeleter<TWasmRef<wasm_extern_vec_t>>([](TWasmRef<wasm_extern_vec_t>* ExternVec)
		{
			wasm_extern_vec_delete(&ExternVec->Value);
			delete ExternVec;
		}));
	}
	
	FORCEINLINE TWasmExtern WasmFromFunction(const TWasmFunc& Func)
	{
		return wasm_func_as_extern(Func.Get());
	}

	FORCEINLINE TWasmFunc WasmToFunction(const TWasmExtern& Extern)
	{
		return TWasmFunc(wasm_extern_as_func(Extern), TWasmFuncCustomDeleter(true));
	}

	FORCEINLINE TWasmFunc MakeWasmFunc(const TWasmStore& WasmStore, const TWasmFuncType& WasmFunctype,
	                                   wasm_func_callback_t FunctionCallback)
	{
		check(FunctionCallback);
		check(WasmFunctype.Get())
		check(WasmStore.Get());
		return TWasmFunc(wasm_func_new(WasmStore.Get(), WasmFunctype.Get(), FunctionCallback));
	}

	FORCEINLINE TWasmValTypeVec CopyWasmValTypeVec(const TWasmValTypeVec& In, bool bDontDelete = false)
	{
		check(In.Get());

		auto Out = new TWasmRef<wasm_valtype_vec_t>();
		wasm_valtype_vec_copy(&Out->Value, &In.Get()->Value);
		return TWasmValTypeVec(Out, TWasmGenericDeleter<TWasmRef<wasm_valtype_vec_t>>([bDontDelete](TWasmRef<wasm_valtype_vec_t>* VecRef)
		{
			if (!bDontDelete)
			{
				wasm_valtype_vec_delete(&VecRef->Value);
			}
			delete VecRef;
		}));
	}

	FORCEINLINE TWasmValTypeVec MakeWasmValTypeVec(wasm_valtype_t* const* Data, const uint32& Num, bool bDontDelete = false)
	{
		check(Data);
		auto ValTypeVec = new TWasmRef<wasm_valtype_vec_t>();
		wasm_valtype_vec_new(&ValTypeVec->Value, Num, Data);
		return TWasmValTypeVec(ValTypeVec, TWasmGenericDeleter<TWasmRef<wasm_valtype_vec_t>>(
			                       [bDontDelete](TWasmRef<wasm_valtype_vec_t>* ValTypeVecHandle)
			                       {
				                       if (!bDontDelete)
				                       {
					                       wasm_valtype_vec_delete(&ValTypeVecHandle->Value);
				                       }
				                       delete ValTypeVecHandle;
			                       }));
	}

	FORCEINLINE TWasmExternVec MakeWasmExternVec(wasm_extern_t* const* Data, const uint32& Num)
	{
		check(Data);
		auto VecRef = new TWasmRef<wasm_extern_vec_t>();
		wasm_extern_vec_new(&VecRef->Value, Num, Data);
		return TWasmExternVec(VecRef, TWasmGenericDeleter<TWasmRef<wasm_extern_vec_t>>([](TWasmRef<wasm_extern_vec_t>* ExternVec)
		{
			wasm_extern_vec_delete(&ExternVec->Value);
			delete ExternVec;
		}));
	}

	FORCEINLINE TWasmByteVec MakeWasmVec(uint8* Data, const uint32& Num)
	{
		check(Data);
		auto VecRef = new TWasmRef<wasm_byte_vec_t>();
		wasm_byte_vec_new(&VecRef->Value, Num * sizeof(uint8), (byte_t*)Data);
		return TWasmByteVec(VecRef, TWasmGenericDeleter<TWasmRef<wasm_byte_vec_t>>([](TWasmRef<wasm_byte_vec_t>* VecHandle)
		{
			wasm_byte_vec_delete(&VecHandle->Value);
			delete VecHandle;
		}));
	}


	FORCEINLINE TWasmFuncType MakeWasmFuncType(const TWasmValTypeVec& Params, const TWasmValTypeVec& Results)
	{
		check(Params.Get());
		check(Results.Get());

		wasm_valtype_vec_t ParamsVec = Params.Get()->Value;
		wasm_valtype_vec_t ResultsVec = Results.Get()->Value;

		return TWasmFuncType(wasm_functype_new(&ParamsVec, &ResultsVec));
	}


	FORCEINLINE TWasmLinker MakeWasmLinker(const TWasiInstance& WasiInstance, const TWasmStore& Store)
	{
		// Create our linker which will be linking our modules together, and then add
		// our WASI instance to it.
		TWasmLinker Linker = TWasmLinker(wasmtime_linker_new(Store.Get()));
		wasmtime_error_t* Error = wasmtime_linker_define_wasi(Linker.Get(), WasiInstance.Get());
		if (Error != nullptr)
		{
			PrintError(TEXT("Failed to create Linker, failed to link WasiInstance."), Error, nullptr);
		}
		return Linker;
	}

	FORCEINLINE TWasiInstance MakeWasiInstance(const TWasmStore& Store, TWasiConfig& Config)
	{
		wasm_trap_t* Trap;
		wasi_instance_t* WasiInstance = wasi_instance_new(Store.Get(), "wasi_snapshot_preview1", Config.Release(), &Trap);
		if (!WasiInstance)
		{
			PrintError(TEXT("New WasiInstance"), nullptr, Trap);
			WasiInstance = nullptr;
		}

		return TWasiInstance(WasiInstance);
	}

	FORCEINLINE TWasmInstance MakeWasmInstance(const TWasmStore& Store, const TWasmModule& Module, const TWasmLinker& Linker)
	{
		check(Store.Get());
		check(Module.Get());
		check(Linker.Get());

		wasm_instance_t* Instance;
		wasm_trap_t* Trap;
		wasmtime_error_t* Error = wasmtime_linker_instantiate(Linker.Get(), Module.Get(), &Instance, &Trap);
		if (!Instance)
		{
			PrintError(TEXT("New Instance"), Error, Trap);
			Instance = nullptr;
		} 

		return TWasmInstance(Instance);
	}

	FORCEINLINE TWasmStore MakeWasmStore(const TWasmEngine& Engine)
	{
		check(Engine.Get());
		return TWasmStore(wasm_store_new(Engine.Get()));
	}

	FORCEINLINE TWasmEngine MakeWasmEngine()
	{
		return TWasmEngine(wasm_engine_new());
	}

	FORCEINLINE TWasiConfig MakeWasiConfig()
	{
		return TWasiConfig(wasi_config_new());
	}

	FORCEINLINE TWasmModule MakeWasmModule(const TWasmStore& Store, const TWasmByteVec& Binary)
	{
		check(Store.Get());
		check(Binary.Get());
		return TWasmModule(wasm_module_new(Store.Get(), &Binary.Get()->Value));
	}
}
