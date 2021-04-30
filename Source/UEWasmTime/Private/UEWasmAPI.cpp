#include "UEWasmAPI.h"

namespace UEWas
{
	bool TWasmFunctionSignature::LinkFunctionAsHostImport(const TWasmContext& Context, wasm_func_callback_t CallbackFunction)
	{
		return LinkFunctionAsHostImport(Context.Store, Context.Linker, CallbackFunction);
	}

	bool TWasmFunctionSignature::LinkFunctionAsHostImport(const TWasmStore& Store,
	                                                      const TWasmLinker& Linker, wasm_func_callback_t InCallbackFunction)
	{
		auto ArgumentsSignature = MakeWasmValTypeVecConst(ArgumentsSignatureArray);
		auto ResultSignature = MakeWasmValTypeVecConst(ResultSignatureArray);

		TWasmFuncType FunctionSignature = MakeWasmFuncType(MoveTemp(ArgumentsSignature), MoveTemp(ResultSignature));
		const TWasmFunc& FuncCallback = MakeWasmFunc(Store, FunctionSignature, InCallbackFunction);
		FunctionSignature.Reset();

		const TWasmName& ModuleName = MakeWasmName(TEXT(""));
		wasmtime_error_t* Error = wasmtime_linker_define(Linker.Get(), &ModuleName.Get()->Value, &Name.Get()->Value,
		                                                 WasmFromFunction(FuncCallback));
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

		const TWasmExternVec Exports = WasmGetInstanceInstanceExports(FuncInstance);

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
