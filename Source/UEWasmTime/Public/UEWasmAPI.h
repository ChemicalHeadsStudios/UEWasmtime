#pragma once
#include <memory>
#include "UEWasmTime.h"
THIRD_PARTY_INCLUDES_START
#include "wasmtime.h"
THIRD_PARTY_INCLUDES_END


#define DECLARE_CUSTOM_WASMTYPE_CUSTOM(Name, SmartPtrType, WasmType, DeleterFunction) \
struct T##Name##CustomDeleter : TDefaultDelete<WasmType> \
{\
	bool bDontDelete;\
	T##Name##CustomDeleter(bool bInDontDelete = false)\
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
typedef SmartPtrType<WasmType, T##Name##CustomDeleter> T##Name

/**
 * Declares WASM type TUniquePtr.
 */
#define DECLARE_CUSTOM_WASMTYPE(Name, WasmType, DeleterFunction) \
	DECLARE_CUSTOM_WASMTYPE_CUSTOM(Name, TUniquePtr, WasmType, DeleterFunction)

/**
* Declares WASM type TUniquePtr.
*/
#define DECLARE_CUSTOM_WASMTYPE_SHARED_PTR(Name, WasmType, DeleterFunction) \
	DECLARE_CUSTOM_WASMTYPE_CUSTOM(Name, TSharedPtr, WasmType, DeleterFunction)

template <typename T>
struct TWasmRef
{
	T Value;
};

#define DECLARE_CUSTOM_WASMTYPE_VEC_CUSTOM(Name, WasmType, WasmVecType, AllocateFunction, AllocationSignature, DeleterFunction)\
struct T##Name##CustomDeleter : TDefaultDelete<TWasmRef<WasmType>>\
{\
	bool bDontDeleteRef;\
	T##Name##CustomDeleter(bool bInDontDeleteRef = false)\
	{\
		bDontDeleteRef = bInDontDeleteRef;\
	}\
	void operator()(TWasmRef<WasmType>* Ptr) const\
	{\
		if (Ptr)\
		{\
			if(!bDontDeleteRef)\
			{\
				DeleterFunction(&Ptr->Value);\
			}\
			delete Ptr;\
			Ptr = nullptr;\
		}\
	}\
	static void\
	AllocationSignature\
	{\
		return AllocateFunction(&Out, Num, Data);\
	}\
	typedef WasmVecType StaticElementType;\
	typedef WasmType StaticWasmType;\
	typedef T##Name##CustomDeleter StaticWasmDeleter;\
};\
typedef TUniquePtr<TWasmRef<WasmType>, T##Name##CustomDeleter> T##Name

#define DECLARE_CUSTOM_WASMTYPE_VEC(Name, WasmType, WasmVecType, AllocateFunction, DeleterFunction)\
	DECLARE_CUSTOM_WASMTYPE_VEC_CUSTOM(Name, WasmType, WasmVecType, AllocateFunction, Allocate(WasmType& Out, WasmVecType* Data, uint32 Num), DeleterFunction)

#define DECLARE_CUSTOM_WASMTYPE_VEC_CONST(Name, WasmType, WasmVecType, AllocateFunction, DeleterFunction)\
	DECLARE_CUSTOM_WASMTYPE_VEC_CUSTOM(Name, WasmType, WasmVecType, AllocateFunction, Allocate(WasmType& Out, WasmVecType* const* Data, uint32 Num), DeleterFunction)


FORCEINLINE bool HandleError(const FString& Caller, wasmtime_error_t* ErrorPointer, wasm_trap_t* TrapPointer)
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
		UE_LOG(LogUEWasmTime, Warning, TEXT("WASMError: (%s) %s"), *Caller, *ErrorString);
		// checkf(false, TEXT("WASMError: (%s) %s"), *Caller, *ErrorString);
		wasm_byte_vec_delete(&ErrorMessage);
		return false;
	}
	return true;
}

/**
 * UEWasm smart pointer and utility library.
 */
namespace UEWas
{
	DECLARE_CUSTOM_WASMTYPE(WasiConfig, wasi_config_t, wasi_config_delete);
	DECLARE_CUSTOM_WASMTYPE(WasiInstance, wasi_instance_t, wasi_instance_delete);

	DECLARE_CUSTOM_WASMTYPE(WasmStore, wasm_store_t, wasm_store_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmInstance, wasm_instance_t, wasm_instance_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmEngine, wasm_engine_t, wasm_engine_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmModule, wasm_module_t, wasm_module_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmFuncType, wasm_functype_t, wasm_functype_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmFunc, wasm_func_t, wasm_func_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmLinker, wasmtime_linker_t, wasmtime_linker_delete);
	
	// VEC types have an overhead of an additional pointer.
	DECLARE_CUSTOM_WASMTYPE_VEC(WasmByteVec, wasm_byte_vec_t, wasm_byte_t, wasm_byte_vec_new, wasm_byte_vec_delete);
	DECLARE_CUSTOM_WASMTYPE_VEC_CONST(WasmExternVec, wasm_extern_vec_t, wasm_extern_t, wasm_extern_vec_new, wasm_extern_vec_delete);
	DECLARE_CUSTOM_WASMTYPE_VEC_CONST(WasmValTypeVec, wasm_valtype_vec_t, wasm_valtype_t, wasm_valtype_vec_new, wasm_valtype_vec_delete);
	DECLARE_CUSTOM_WASMTYPE_VEC(WasmValVec, wasm_val_vec_t, wasm_val_t, wasm_val_vec_new, wasm_val_vec_delete);

	 // TSharedPtr checks for type completeness so we use std::shared_ptr instead.
	typedef std::shared_ptr<wasm_valtype_t> TWasmValType;
	
	typedef wasm_extern_t* TWasmExtern;
	typedef TWasmByteVec TWasmName;

	FORCEINLINE FString WasmNameToString(const TWasmName& Name)
	{
		if(Name.Get())
		{
			if(Name.Get()->Value.size > 0)
			{
				return FString(UTF8_TO_TCHAR(Name.Get()->Value.data), Name.Get()->Value.size);
			}
		}
		return TEXT("");
	}
	
	FORCEINLINE TWasmValType MakeWasmValType(wasm_valtype_t* InValType)
	{
		check(InValType);
		TWasmValType Val = TWasmValType(InValType, [](wasm_valtype_t* ValType)
		{
			if(ValType)
			{
				wasm_valtype_delete(ValType);
			}
		});
		return Val;
	}

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
		return TWasmName(NamePtr);
	};
	
	FORCEINLINE TWasmExternVec WasmGetInstanceInstanceExports(const TWasmInstance& Instance)
	{
		check(Instance.Get());
		auto ExternVec = new TWasmRef<wasm_extern_vec_t>();
		wasm_instance_exports(Instance.Get(), &ExternVec->Value);
		return TWasmExternVec(ExternVec);
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

	template <typename VecArrayType = TWasmByteVec>
	FORCEINLINE VecArrayType MakeWasmVecConst(typename VecArrayType::StaticElementType* const* Data, const uint32& Num, bool bDontDelete = false)
	{
		const auto Vec = new TWasmRef<typename VecArrayType::StaticWasmType>();
		if(Data != nullptr && Num != 0)
		{
			VecArrayType::Allocate(Vec->Value, Data, Num);
		} 
		return VecArrayType(Vec, typename VecArrayType::StaticWasmDeleter(bDontDelete));
	}
	
	template <typename VecArrayType = TWasmByteVec>
	FORCEINLINE VecArrayType MakeWasmVec(const TArray<typename VecArrayType::StaticElementType*>& Data, bool bDontDelete = false)
	{
		return MakeWasmVec<VecArrayType>(Data.GetData(), Data.Num(), bDontDelete);
	}

	template<typename VecArrayType = TWasmByteVec>
	FORCEINLINE VecArrayType MakeWasmVec(typename VecArrayType::StaticElementType* Data, const uint32& Num, bool bDontDelete = false)
	{
		const auto Vec = new TWasmRef<typename VecArrayType::StaticWasmType>();
		if (Data != nullptr && Num != 0)
		{
			VecArrayType::Allocate(Vec->Value, Data, Num);
		}
		return VecArrayType(Vec, typename VecArrayType::StaticWasmDeleter(bDontDelete));
	}

	FORCEINLINE TWasmValTypeVec MakeWasmValTypeVecConst(TWasmValType* Data, const uint32& Num,
	                                                    bool bDontDelete = false)
	{
		TArray<wasm_valtype_t*> InnerValTypes;
		for (uint32 Index = 0; Index < Num; Index++)
		{
			check(Data[Index].get());
			InnerValTypes.Emplace(Data[Index].get());
		}
		return MakeWasmVecConst<TWasmValTypeVec>(InnerValTypes.GetData(), InnerValTypes.Num(), bDontDelete);
	}

	FORCEINLINE TWasmValTypeVec MakeWasmValTypeVecConst(TArray<TWasmValType>& Array,
													bool bDontDelete = false)
	{
		return MakeWasmValTypeVecConst(Array.GetData(), Array.Num(), bDontDelete);
	}

	FORCEINLINE TWasmFuncType MakeWasmFuncType(TWasmValTypeVec&& Params, TWasmValTypeVec&& Results)
	{
		return TWasmFuncType(wasm_functype_new(&Params.Get()->Value, &Results.Get()->Value));
	}

	FORCEINLINE TWasmValType MakeWasmValTypeInt32()
	{
		return MakeWasmValType(wasm_valtype_new_i32());
	}
	
	FORCEINLINE TWasmValType MakeWasmValTypeInt64()
	{
		return MakeWasmValType(wasm_valtype_new_i64());
	}
	
	FORCEINLINE TWasmValType MakeWasmValTypeFloat32()
	{
		return MakeWasmValType(wasm_valtype_new_f32());
	}
	
	FORCEINLINE TWasmValType MakeWasmValTypeFloat64()
	{
		return MakeWasmValType(wasm_valtype_new_f64());
	}
	
	FORCEINLINE TWasmValType MakeWasmValTypeAnyRef()
	{
		return MakeWasmValType(wasm_valtype_new_anyref());
	}
	
	FORCEINLINE TWasmValType MakeWasmValTypeFuncRef()
	{
		return MakeWasmValType(wasm_valtype_new_funcref());
	}

	FORCEINLINE TWasmLinker MakeWasmLinker(const TWasiInstance& WasiInstance, const TWasmStore& Store)
	{
		// Create our linker which will be linking our modules together, and then add
		// our WASI instance to it.
		TWasmLinker Linker = TWasmLinker(wasmtime_linker_new(Store.Get()));
		wasmtime_error_t* Error = wasmtime_linker_define_wasi(Linker.Get(), WasiInstance.Get());
		HandleError(TEXT("Failed to create Linker, failed to link WasiInstance."), Error, nullptr);
		return Linker;
	}

	FORCEINLINE TWasiInstance MakeWasiInstance(const TWasmStore& Store, TWasiConfig&& Config)
	{
		check(Store.Get());
		check(Config.Get());
		
		wasm_trap_t* Trap;
		wasi_instance_t* WasiInstance = wasi_instance_new(Store.Get(), "wasi_snapshot_preview1", Config.Release(), &Trap);
		if (!WasiInstance)
		{
			HandleError(TEXT("New WasiInstance"), nullptr, Trap);
		}

		return TWasiInstance(WasiInstance);
	}

	FORCEINLINE TWasmInstance MakeWasmInstance(const TWasmModule& Module, const TWasmLinker& Linker)
	{
		check(Module.Get());
		check(Linker.Get());

		wasm_instance_t* RawInstance;
		wasm_trap_t* Trap;
		wasmtime_error_t* Error = wasmtime_linker_instantiate(Linker.Get(), Module.Get(), &RawInstance, &Trap);
		TWasmInstance WasmInstance = TWasmInstance(RawInstance);
		HandleError(TEXT("New Instance"), Error, nullptr);
		
		return WasmInstance;
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

	template <typename T>
	struct TWasmValue
	{
	};

	template <>
	struct TWasmValue<int32_t>
	{
		static wasm_val_t NewValue(int32_t InValue)
		{
			wasm_val_t WasmValue;
			WasmValue.kind = WASM_I32;
			WasmValue.of.i32 = InValue;
			return WasmValue;
		}

		static TWasmValType GetType()
		{
			return MakeWasmValTypeInt32();
		}
	};

	template <>
	struct TWasmValue<int64_t>
	{
		static wasm_val_t NewValue(int64_t InValue)
		{
			wasm_val_t WasmValue;
			WasmValue.kind = WASM_I64;
			WasmValue.of.i64 = InValue;
			return WasmValue;
		}

		static TWasmValType GetType()
		{
			return MakeWasmValTypeInt64();
		}
	};

	template <>
	struct TWasmValue<float32_t>
	{
		static wasm_val_t NewValue(float32_t InValue)
		{
			wasm_val_t WasmValue;
			WasmValue.kind = WASM_F32;
			WasmValue.of.f32 = InValue;
			return WasmValue;
		}

		static TWasmValType GetType()
		{
			return MakeWasmValTypeFloat32();
		}
	};

	template <>
	struct TWasmValue<float64_t>
	{
		static wasm_val_t NewValue(float64_t InValue)
		{
			wasm_val_t WasmValue;
			WasmValue.kind = WASM_F64;
			WasmValue.of.f64 = InValue;
			return WasmValue;
		}

		static TWasmValType GetType()
		{
			return MakeWasmValTypeFloat64();
		}
	};

	template <>
	struct TWasmValue<wasm_ref_t*>
	{
		static wasm_val_t NewValue(wasm_ref_t* InValue)
		{
			wasm_val_t WasmValue;
			WasmValue.kind = WASM_ANYREF;
			WasmValue.of.ref = InValue;
			return WasmValue;
		}

		static TWasmValType GetType()
		{
			return MakeWasmValTypeAnyRef();
		}
	};


	class UEWASMTIME_API TWasmContext
    {
    public:
    	TWasmStore Store;
    	TWasiInstance Instance;
    	TWasmLinker Linker;
    public:
        TWasmContext(const TWasmEngine& InEngine, const FString& WorkspacePath)
        {
	        TWasiConfig TempConfig = MakeWasiConfig();
	        Store = MakeWasmStore(InEngine);
	        if (Store.IsValid())
	        {
		        if (wasi_config_preopen_dir(TempConfig.Get(), TCHAR_TO_UTF8(*WorkspacePath), TCHAR_TO_UTF8(TEXT(""))))
		        {
			        if (TempConfig.IsValid())
			        {
				        Instance = MakeWasiInstance(Store, MoveTemp(TempConfig));
				        if (Instance.IsValid())
				        {
					        Linker = MakeWasmLinker(Instance, Store);
				        }
			        }
		        }
	        }
        }
    
    	FORCEINLINE bool IsValid() const
    	{
    		return Linker.IsValid();
    	}
    };
    
    
    class UEWASMTIME_API TWasmFunctionSignature
    {
    protected:
    	TWasmName Name;
    	TArray<TWasmValType> ArgumentsSignatureArray;
    	TArray<TWasmValType> ResultSignatureArray;
    public:
    	TWasmFunctionSignature(const FString& InFunctionName, TArray<TWasmValType>&& InArgsSignature,
    	                               TArray<TWasmValType>&& InResultSignature)
    	{
    		Name = MakeWasmName(InFunctionName);
    		ArgumentsSignatureArray = MoveTemp(InArgsSignature);
    		ResultSignatureArray = MoveTemp(InResultSignature);
    	};
    
    	TWasmFunctionSignature(const FString& InFunctionName, const TArray<TWasmValType>& InArgsSignature,
    	                               const TArray<TWasmValType>& InResultSignature)
    	{
    		Name = MakeWasmName(InFunctionName);
    		ArgumentsSignatureArray = InArgsSignature;
    		ResultSignatureArray = InResultSignature;
    	};
    
    
    	bool LinkFunctionAsHostImport(const TWasmContext& Context, wasm_func_callback_t CallbackFunction);
    	bool LinkFunctionAsHostImport(const TWasmStore& Store, const TWasmLinker& Linker, wasm_func_callback_t CallbackFunction);
    
    	bool Call(const TWasmModule& Module, const TWasmLinker& Linker, TArray<wasm_val_t> Args, TArray<wasm_val_t>& Results);
    };
}
