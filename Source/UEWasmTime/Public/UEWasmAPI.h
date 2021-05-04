#pragma once
#include <memory>
#include <string>

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
* Declares WASM type TSharedPtr.
*/
// @todo: Use UE's TSharedPtr. UE doesn't let you use it's TSharedPtr on incomplete types, we use std::shared_ptr typedefs instead of this.
// #define DECLARE_CUSTOM_WASMTYPE_SHARED_PTR(Name, WasmType, DeleterFunction) \
// 	DECLARE_CUSTOM_WASMTYPE_CUSTOM(Name, TSharedPtr, WasmType, DeleterFunction)

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


FORCEINLINE bool HandleError(const FString& Caller, wasmtime_error_t* ErrorPointer, wasm_trap_t* TrapPointer = nullptr)
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
	class TWasmExecutionContext;
	DECLARE_CUSTOM_WASMTYPE(WasiConfig, wasi_config_t, wasi_config_delete);
	DECLARE_CUSTOM_WASMTYPE(WasiInstance, wasi_instance_t, wasi_instance_delete);

	DECLARE_CUSTOM_WASMTYPE(WasmConfig, wasm_config_t, wasm_config_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmStore, wasm_store_t, wasm_store_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmInstance, wasm_instance_t, wasm_instance_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmEngine, wasm_engine_t, wasm_engine_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmModule, wasm_module_t, wasm_module_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmFuncType, wasm_functype_t, wasm_functype_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmFunc, wasm_func_t, wasm_func_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmLinker, wasmtime_linker_t, wasmtime_linker_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmGlobalVal, wasm_global_t, wasm_global_delete);
	DECLARE_CUSTOM_WASMTYPE(WasmExport, wasm_extern_t, wasm_extern_delete);

	// VEC types have an overhead of an additional pointer.
	DECLARE_CUSTOM_WASMTYPE_VEC(WasmByteVec, wasm_byte_vec_t, wasm_byte_t, wasm_byte_vec_new, wasm_byte_vec_delete);
	DECLARE_CUSTOM_WASMTYPE_VEC_CONST(WasmExternVec, wasm_extern_vec_t, wasm_extern_t, wasm_extern_vec_new, wasm_extern_vec_delete);
	DECLARE_CUSTOM_WASMTYPE_VEC_CONST(WasmValTypeVec, wasm_valtype_vec_t, wasm_valtype_t, wasm_valtype_vec_new, wasm_valtype_vec_delete);
	DECLARE_CUSTOM_WASMTYPE_VEC(WasmValVec, wasm_val_vec_t, wasm_val_t, wasm_val_vec_new, wasm_val_vec_delete);

	// TSharedPtr checks for type completeness so we use std::shared_ptr instead.
	typedef std::shared_ptr<wasm_valtype_t> TWasmValType;

	typedef const wasm_extern_t* TWasmExternConst;
	typedef wasm_extern_t* TWasmExtern;
	typedef TWasmByteVec TWasmName;

	FORCEINLINE FString WasmNameToString(const TWasmName& Name)
	{
		if (Name.Get())
		{
			if (Name.Get()->Value.size > 0)
			{
				return FString(Name.Get()->Value.size, UTF8_TO_TCHAR(Name.Get()->Value.data));
			}
		}
		return TEXT("");
	}

	FORCEINLINE TWasmName MakeWasmName(const FString& InString)
	{
		auto NamePtr = new TWasmRef<wasm_name_t>();
		if (!InString.IsEmpty())
		{
			wasm_name_new_from_string(&NamePtr->Value, TCHAR_TO_UTF8(*InString));
		}
		else
		{
			wasm_name_new_empty(&NamePtr->Value);
		}
		return TWasmName(NamePtr);
	}

	FORCEINLINE TWasmValType MakeWasmValType(wasm_valtype_t* InValType)
	{
		check(InValType);
		TWasmValType Val = TWasmValType(InValType, [](wasm_valtype_t* ValType)
		{
			if (ValType)
			{
				wasm_valtype_delete(ValType);
			}
		});
		return Val;
	}

	FORCEINLINE TWasmExternVec WasmGetInstanceExports(const TWasmInstance& Instance)
	{
		check(Instance.Get());
		auto ExternVec = new TWasmRef<wasm_extern_vec_t>();
		wasm_instance_exports(Instance.Get(), &ExternVec->Value);
		return TWasmExternVec(ExternVec);
	}

	FORCEINLINE TWasmExternConst WasmGlobalAsExternConst(const TWasmGlobalVal& Val)
	{
		return wasm_global_as_extern_const(Val.Get());
	}

	FORCEINLINE TWasmExtern WasmGlobalAsExtern(const TWasmGlobalVal& Val)
	{
		return wasm_global_as_extern(Val.Get());
	}

	FORCEINLINE TWasmExtern WasmFunctionAsExtern(const TWasmFunc& Func)
	{
		return wasm_func_as_extern(Func.Get());
	}

	FORCEINLINE TWasmFunc WasmExportToFunction(const TWasmExtern& Extern)
	{
		return TWasmFunc(wasm_extern_as_func(Extern), TWasmFuncCustomDeleter(true));
	}

	template <typename VecArrayType = TWasmByteVec>
	FORCEINLINE VecArrayType MakeWasmVecConst(typename VecArrayType::StaticElementType* const* Data, const uint32& Num,
	                                          bool bDontDelete = false)
	{
		const auto Vec = new TWasmRef<typename VecArrayType::StaticWasmType>();
		if (Data != nullptr && Num != 0)
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

	template <typename VecArrayType = TWasmByteVec>
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

		if(RawInstance)
		{
			// if(Trap && Trap)
			// {
			// 	wasm_trap_delete(Trap);
			// }

			// if(Error)
			// {
			// 	wasmtime_error_delete(Error);
			// }
			//
			return TWasmInstance(RawInstance);
		}
		return {};
	}

	FORCEINLINE TWasmStore MakeWasmStore(const TWasmEngine& Engine)
	{
		check(Engine.Get());
		return TWasmStore(wasm_store_new(Engine.Get()));
	}

	FORCEINLINE TWasmConfig MakeWasmConfig()
	{
		return TWasmConfig(wasm_config_new());
	}

	FORCEINLINE TWasmEngine MakeWasmEngine(TWasmConfig&& Config)
	{
		return TWasmEngine(wasm_engine_new_with_config(Config.Release()));
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

	template <typename T>
	FORCEINLINE TWasmGlobalVal MakeWasmGlobalVal(const TWasmStore& Store, const T& Value,
	                                             const wasm_mutability_enum& Mutability = wasm_mutability_enum::WASM_CONST)
	{
		TWasmGlobalVal Out = {};
		// wasm_global_t* Global;
		// auto WrappedValue = TWasmValue<T>::NewValue(Value);
		// if (HandleError(TEXT("New Global"), wasmtime_global_new(Store.Get(), GlobalType, &WrappedValue, &Global)))
		// {
		// 	Out = TWasmGlobalVal(Global);
		// }
		//
		// wasm_globaltype_delete(GlobalType);
		return Out;
	}
	
	class UEWASMTIME_API TWasmFunctionSignature
	{
	protected:
		int32 CachedExternIndex = INDEX_NONE;
		TWasmName ModuleName;
		TWasmName Name;
		TArray<TWasmValType> ArgumentsSignatureArray;
		TArray<TWasmValType> ResultSignatureArray;
		wasmtime_func_callback_with_env_t ImportCallback;
	public:
		TWasmFunctionSignature(TWasmFunctionSignature&& MoveSignature)
		{
			ModuleName = MoveTemp(MoveSignature.ModuleName);
			Name = MoveTemp(MoveSignature.Name);
			ArgumentsSignatureArray = MoveTemp(MoveSignature.ArgumentsSignatureArray);
			ResultSignatureArray = MoveTemp(MoveSignature.ResultSignatureArray);
			ImportCallback = MoveTempIfPossible(MoveSignature.ImportCallback);
		};

		TWasmFunctionSignature(const FString& InModuleName, const FString& InFunctionName, TArray<TWasmValType>&& InArgsSignature,
		                       TArray<TWasmValType>&& InResultSignature, wasmtime_func_callback_with_env_t InImportCallback = nullptr)
		{
			ModuleName = MakeWasmName(InModuleName);
			Name = MakeWasmName(InFunctionName);
			ArgumentsSignatureArray = MoveTemp(InArgsSignature);
			ResultSignatureArray = MoveTemp(InResultSignature);
			ImportCallback = InImportCallback;
		};

		TWasmFunctionSignature(const FString& InModuleName, const FString& InFunctionName, const TArray<TWasmValType>& InArgsSignature = {},
		                       const TArray<TWasmValType>& InResultSignature = {}, wasmtime_func_callback_with_env_t InImportCallback = nullptr)
		{
			ModuleName = MakeWasmName(InModuleName);
			Name = MakeWasmName(InFunctionName);
			ArgumentsSignatureArray = InArgsSignature;
			ResultSignatureArray = InResultSignature;
			ImportCallback = InImportCallback;
		};


		bool LinkExtern(const FString& ExternModule, const FString& ExternName, const TWasmLinker& Linker, const TWasmExtern& Extern);
		bool LinkExtern(const FString& ExternModule, const FString& ExternName, const TWasmExecutionContext& Context, const TWasmExtern& Extern);

		bool LinkFunctionAsHostImport(const TWasmExecutionContext& Context,
		                              wasmtime_func_callback_with_env_t OverrideCallback = nullptr);

		bool Call(const uint32& FuncExternIndex, const TWasmInstance& Instance, TArray<wasm_val_t> Args,
		          TArray<wasm_val_t>& Results);

		bool Exists(const TWasmModule& Module, const TWasmLinker& Linker) const;


		FORCEINLINE FString GetName() const
		{
			return WasmNameToString(Name);
		}
	};
	
	UEWASMTIME_API typedef TSharedPtr<TWasmFunctionSignature> TWasmFunctionSignaturePtr;
	UEWASMTIME_API typedef TSharedRef<TWasmFunctionSignature> TWasmFunctionSignatureRef;

	UEWASMTIME_API typedef TMap<FName, uint32> TWasmExternMap;
	UEWASMTIME_API typedef TSharedPtr<TWasmExternMap> TWasmExternMapPtr;


	FORCEINLINE TWasmExternMapPtr GenerateWasmExternMap(const TWasmModule& Module)
	{
		TWasmExternMapPtr ExternMap = TWasmExternMapPtr(new TWasmExternMap());
		wasm_exporttype_vec_t ExportTypes;
		wasm_module_exports(Module.Get(), &ExportTypes);

		for (uint32 Index = 0; Index < ExportTypes.size; Index++)
		{
			auto Name = wasm_exporttype_name(ExportTypes.data[Index]);
			const FString& ExportName = FString(Name->size, UTF8_TO_TCHAR(Name->data));
			ExternMap->Emplace(*ExportName, Index);
			UE_LOG(LogUEWasmTime, Warning, TEXT("Exportmap %s"), *ExportName);
		}

		wasm_exporttype_vec_delete(&ExportTypes);
		return ExternMap;
	}

	
	class UEWASMTIME_API TWasmExecutionContext
	{
	public:
		TWasmStore Store;
		TWasiInstance LinkerInstance;
		TWasmLinker Linker;
		TWasmInstance Instance;
		TWasmExternMapPtr ExternMapping;
		void* AdditionalEnvironment;
		
	protected:
		bool bValid;

	public:
		TWasmExecutionContext(const TWasmModule& Module, const TWasmEngine& InEngine,
		                      const TArray<TWasmFunctionSignaturePtr>& ImportFunctions, const TWasmExternMapPtr& InExternMapping,
		                      const FString& WorkspacePath)
		{
			ExternMapping = InExternMapping;
			TWasiConfig TempConfig = MakeWasiConfig();
			Store = MakeWasmStore(InEngine);
			if (Store.IsValid())
			{
				if (wasi_config_preopen_dir(TempConfig.Get(), TCHAR_TO_UTF8(*WorkspacePath), TCHAR_TO_UTF8(TEXT(""))))
				{
					if (TempConfig.IsValid())
					{
						LinkerInstance = MakeWasiInstance(Store, MoveTemp(TempConfig));
						if (LinkerInstance.IsValid())
						{
							Linker = MakeWasmLinker(LinkerInstance, Store);
							if (Linker.IsValid())
							{
								for (const TWasmFunctionSignaturePtr& Import : ImportFunctions)
								{
									check(Import.Get());
									Import->LinkFunctionAsHostImport(*this);
								}
								
								Instance = MakeWasmInstance(Module, Linker);
								if(Instance.IsValid())
								{
									bValid = true;
								}
							}
						}
					}
				}
			}
		}

		FORCEINLINE bool IsValid() const
		{
			return bValid;
		}
	};

	typedef TSharedPtr<TWasmExecutionContext> TWasmExecutionContextPtr; 

	
	FORCEINLINE TWasmFunc MakeWasmFunc(const TWasmStore& WasmStore, const TWasmFuncType& WasmFunctype,
									wasmtime_func_callback_with_env_t FunctionCallback, const TWasmExecutionContext& Ptr)
	{
		check(FunctionCallback);
		check(WasmFunctype.Get())
		check(WasmStore.Get());
		return TWasmFunc(wasmtime_func_new_with_env(WasmStore.Get(), WasmFunctype.Get(), FunctionCallback, (void*)&Ptr, nullptr));
	}

	FORCEINLINE byte_t* GetWasmExecutionMemory(const TWasmExecutionContext& Context, uint64_t& MemorySize, uint64_t& MemoryDataSize)
	{
		const uint32* MemoryIndex = Context.ExternMapping->Find(TEXT("memory"));
		if (MemoryIndex)
		{
			const TWasmExternVec& Exports = WasmGetInstanceExports(Context.Instance);
			if (!Exports.IsValid())
			{
				UE_LOG(LogUEWasmTime, Warning, TEXT("Error accessing export export!"));
				return nullptr;
			}

			if (Exports.Get()->Value.size < *MemoryIndex)
			{
				UE_LOG(LogUEWasmTime, Warning, TEXT("Tried calling non-existent export!"));
				return nullptr;
			}

			const auto Memory = wasm_extern_as_memory(Exports.Get()->Value.data[*MemoryIndex]);
			if (Memory)
			{
				MemorySize = wasm_memory_size(Memory);
				MemoryDataSize = wasm_memory_data_size(Memory);
				return wasm_memory_data(Memory);
			}
		}
		return nullptr;
	}

	
	// template <>
	// struct TWasmValue<FString>
	// {
	// 	static wasm_val_t NewValue(const FString& InValue, const TWasmModule& WasmModule, const TWasmLinker& Linker,
	// 	                           const TWasmFunctionSignaturePtrWithIndex& AllocFunction,
	// 	                           const TWasmFunctionSignaturePtrWithIndex& PinFunction = {})
	// 	{
	// 		wasm_val_t WasmValue;
	// 		WasmValue.kind = WASM_I32;
	// 		WasmValue.of.i32 = INDEX_NONE;
	//
	// 		std::string Utf8String = std::string(TCHAR_TO_UTF8(*InValue));
	// 		// UTF8 string length + null terminator
	// 		const int32 SizeInBytes = Utf8String.length() + 1;
	// 		TArray<wasm_val_t> Results;
	// 		if (AllocFunction.Function && AllocFunction.Call(WasmModule, Linker,
	// 		                                                 {TWasmValue<int32>::NewValue(SizeInBytes), TWasmValue<int32>::NewValue(0)},
	// 		                                                 Results))
	// 		{
	// 			UE_LOG(LogUEWasmTime, Warning, TEXT("Alloc %i"), Results[0].of.i32);
	// 		} else
	// 		{
	// 			UE_LOG(LogUEWasmTime, Warning, TEXT("Failed alloc"));
	// 		}
	// 		return WasmValue;
	// 	}
	//
	// 	static TWasmValType GetType()
	// 	{
	// 		return MakeWasmValTypeInt32();
	// 	}
	// };
	//
	FORCEINLINE TWasmFunctionSignaturePtr MakeWasmFunctionSignature(TWasmFunctionSignature&& Signature)
	{
		return MakeShareable<TWasmFunctionSignature>(new TWasmFunctionSignature(MoveTemp(Signature)));
	}

	FORCEINLINE TWasmExport WasmGetCallerExport(const wasmtime_caller_t* Caller, const FString& ExportName)
	{
		return TWasmExport(wasmtime_caller_export_get(Caller, &MakeWasmName(ExportName).Get()->Value));
	}

	// FORCEINLINE TWasmExternVec WasmGetExports(const TWasmModule& WasmModule, const TWasmLinker& Linker, int32 CheckIndex = INDEX_NONE)
	// {
	// 	const TWasmInstance LinkerInstance = MakeWasmInstance(WasmModule, Linker);
	// 	if (!LinkerInstance.IsValid())
	// 	{
	// 		UE_LOG(LogUEWasmTime, Warning, TEXT("Failed to create module instance."));
	// 		return {nullptr};
	// 	}
	//
	// 	TWasmExternVec Exports = WasmGetInstanceExports(LinkerInstance);
	// 	if (!Exports.IsValid())
	// 	{
	// 		UE_LOG(LogUEWasmTime, Warning, TEXT("Error accessing export function!"));
	// 		return {nullptr};
	// 	}
	//
	// 	if(CheckIndex > 0)
	// 	{
	// 		if (Exports.Get()->Value.size < CheckIndex)
	// 		{
	// 			UE_LOG(LogUEWasmTime, Warning, TEXT("Tried fetching out of bounds function!"));
	// 			return {nullptr};
	// 		}
	// 	}
	//
	// 	if(Exports.Get()->Value.size <= 0)
	// 	{
	// 		UE_LOG(LogUEWasmTime, Warning, TEXT("No exports found in instance!"));
	// 		return {nullptr};
	// 	}
	//
	// 	return Exports;
	// }

	FORCEINLINE FString WasmMemoryReadString(const wasmtime_caller_t* Caller, const int32& PointerOffset, const int32& NumChars)
	{
		const TWasmExport& Export = WasmGetCallerExport(Caller, TEXT("memory"));
		if (Export.IsValid())
		{
			wasm_memory_t* Memory = wasm_extern_as_memory(Export.Get());
			if (Memory)
			{
				const uint64_t NumBytesMemory = wasm_memory_data_size(Memory);
				const uint64_t Address = PointerOffset;
				const uint64_t StringLength = NumChars;
				if (NumBytesMemory > 0 && NumBytesMemory > (Address + StringLength))
				{
					TArray<byte_t> Copied;
					Copied.Reserve(StringLength);

					byte_t* Data = wasm_memory_data(Memory);
					for (uint64_t Index = 0; Index < StringLength; Index++)
					{
						const TCHAR& Char = Data[Address + Index];
						Copied.Emplace(Char);
						if (Char == '\0')
						{
							break;
						}
					}
					
					if (Copied.Num() > 0)
					{
						return FString(Copied.Num() - 1, Copied.GetData());
					}
				}
			}
		}
		return TEXT("");
	}
}
