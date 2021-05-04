#include "UEWasmAPI.h"

namespace UEWas
{
	bool TWasmFunctionSignature::LinkExtern(const FString& ExternModule, const FString& ExternName, const TWasmLinker& Linker,
	                                        const TWasmExtern& Extern)
	{
		return false;// HandleError(TEXT("Link Extern"), wasmtime_linker_define(Linker.Get(), &MakeWasmName(ExternModule).Get()->Value,&MakeWasmName(ExternName).Get()->Value, Extern));
	}

	bool TWasmFunctionSignature::LinkFunctionAsHostImport(const TWasmExecutionContext& Context, wasmtime_func_callback_with_env_t OverrideCallback)
	{
		wasmtime_func_callback_with_env_t Callback = OverrideCallback != nullptr ? OverrideCallback : ImportCallback;
#if !UE_BUILD_SHIPPING
		// @todo Don't assert here because we want to add new callbacks while live coding. Don't rely on this functionality in shipping.
		if (!Callback)
		{
			UE_LOG(LogUEWasmTime, Error, TEXT("!!! No callback specified for %s. Will CRASH in shipping."), *WasmNameToString(Name));
			return false;
		}
#endif

		auto ArgumentsSignature = MakeWasmValTypeVecConst(ArgumentsSignatureArray);
		auto ResultSignature = MakeWasmValTypeVecConst(ResultSignatureArray);

		TWasmFuncType FunctionSignature = MakeWasmFuncType(MoveTemp(ArgumentsSignature), MoveTemp(ResultSignature));
		const TWasmFunc& FuncCallback = MakeWasmFunc(Context.Store, FunctionSignature, Callback, Context);
		FunctionSignature.Reset();

		wasmtime_error_t* Error = wasmtime_linker_define(Context.Linker.Get(), &ModuleName.Get()->Value, &Name.Get()->Value,
		                                                 WasmFunctionAsExtern(FuncCallback));
		
		return HandleError(TEXT("Linking"), Error, nullptr);
	}

	
	bool TWasmFunctionSignature::Call(const uint32& FuncExternIndex, const TWasmInstance& Instance, TArray<wasm_val_t> Args,
	                                  TArray<wasm_val_t>& Results)
	{
		if (Args.Num() != ArgumentsSignatureArray.Num())
		{
			UE_LOG(LogUEWasmTime, Error, TEXT("Function (%s): argument size mismatch. Given %i, need %i."), *WasmNameToString(Name),
			       Args.Num(), ArgumentsSignatureArray.Num());
			return false;
		}
		
		const TWasmExternVec& Exports = WasmGetInstanceExports(Instance);
		if (!Exports.IsValid())
		{
			UE_LOG(LogUEWasmTime, Warning, TEXT("Error accessing export function!"));
			return false;
		}

		if(Exports.Get()->Value.size < FuncExternIndex)
		{
			UE_LOG(LogUEWasmTime, Warning, TEXT("Tried calling non-existent function!"));
			return false;
		}

		wasm_func_t* Func = wasm_extern_as_func(Exports.Get()->Value.data[FuncExternIndex]);
		if (!Func)
		{
			UE_LOG(LogUEWasmTime, Warning, TEXT("Error casting export to function!"));
			return false;
		}

		Results.Reset(ResultSignatureArray.Num());
		for (int32 Index = 0; Index < ResultSignatureArray.Num(); Index++)
		{
			Results.Emplace(TWasmValue<wasm_ref_t*>::NewValue(nullptr));
		}
		
		wasm_val_vec_t ResultsVec = wasm_val_vec_t{(uint32)Results.Num(), Results.GetData()};
		wasm_val_vec_t ArgsVec = wasm_val_vec_t{(uint32)Args.Num(), Args.GetData()};
		if (!HandleError(TEXT("Function call"), nullptr, wasm_func_call(Func, &ArgsVec, &ResultsVec)))
		{
			UE_LOG(LogUEWasmTime, Warning, TEXT("Error calling function!"));
			return false;
		}
		return true;
	}

	bool TWasmFunctionSignature::Exists(const TWasmModule& Module, const TWasmLinker& Linker) const
	{
	
		return true;
	}
}
