#include "UEWasmAPI.h"

namespace UEWas
{
	bool TWasmFunctionSignature::LinkExtern(const FString& ExternModule, const FString& ExternName, const TWasmLinker& Linker,
	                                        const TWasmExtern& Extern)
	{
		return false;// HandleError(TEXT("Link Extern"), wasmtime_linker_define(Linker.Get(), &MakeWasmName(ExternModule).Get()->Value,&MakeWasmName(ExternName).Get()->Value, Extern));
	}

	bool TWasmFunctionSignature::LinkExtern(const FString& ExternModule, const FString& ExternName, const TWasmContext& Context,
		const TWasmExtern& Extern)
	{
		return LinkExtern(ExternModule, ExternName, Context.Linker, Extern);
	}

	bool TWasmFunctionSignature::LinkFunctionAsHostImport(const TWasmContext& Context, wasm_func_callback_t OverrideCallback)
	{
		return LinkFunctionAsHostImport(Context.Store, Context.Linker, OverrideCallback);
	}

	bool TWasmFunctionSignature::LinkFunctionAsHostImport(const TWasmStore& Store,
	                                                      const TWasmLinker& Linker, wasm_func_callback_t OverrideCallback)
	{
		wasm_func_callback_t Callback = OverrideCallback != nullptr ? OverrideCallback : ImportCallback;
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
		const TWasmFunc& FuncCallback = MakeWasmFunc(Store, FunctionSignature, Callback);
		FunctionSignature.Reset();


		wasmtime_error_t* Error = wasmtime_linker_define(Linker.Get(), &ModuleName.Get()->Value, &Name.Get()->Value,
		                                                 WasmFunctionAsExtern(FuncCallback));
		return HandleError(TEXT("Linking"), Error, nullptr);
	}

	bool TWasmFunctionSignature::Call(const TWasmModule& Module, const TWasmLinker& Linker, TArray<wasm_val_t> Args,
	                                  TArray<wasm_val_t>& Results)
	{
		if (Args.Num() != ArgumentsSignatureArray.Num())
		{
			UE_LOG(LogUEWasmTime, Error, TEXT("Function (%s): argument size mismatch. Given %i, need %i."), *WasmNameToString(Name),
			       Args.Num(), ArgumentsSignatureArray.Num());
			return false;
		}

		auto FuncInstance = MakeWasmInstance(Module, Linker);
		if (!FuncInstance.IsValid())
		{
			UE_LOG(LogUEWasmTime, Warning, TEXT("Error instantiating module!"));
			return false;
		}

		const TWasmExternVec Exports = WasmGetInstanceExports(FuncInstance);

		if (Exports.Get()->Value.size == 0)
		{
			UE_LOG(LogUEWasmTime, Warning, TEXT("Error accessing export function!"));
			return false;
		}

		const TWasmFunc& RunFunc = WasmToFunction(Exports.Get()->Value.data[0]);
		if (!RunFunc.IsValid())
		{
			UE_LOG(LogUEWasmTime, Warning, TEXT("Error accessing function!"));
			return false;
		}

		Results.Reset(ResultSignatureArray.Num());
		for (int32 Index = 0; Index < ResultSignatureArray.Num(); Index++)
		{
			Results.Emplace(TWasmValue<wasm_ref_t*>::NewValue(nullptr));
		}

		wasm_val_vec_t ResultsVec = wasm_val_vec_t{(uint32)Results.Num(), Results.GetData()};
		wasm_val_vec_t ArgsVec = wasm_val_vec_t{(uint32)Args.Num(), Args.GetData()};
		if (!HandleError(TEXT("Function call"), nullptr, wasm_func_call(RunFunc.Get(), &ArgsVec, &ResultsVec)))
		{
			UE_LOG(LogUEWasmTime, Warning, TEXT("Error calling function!"));
			return false;
		}
		return true;
	}
}
