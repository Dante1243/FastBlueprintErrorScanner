

#pragma once

#include "FBESStruct.h"


DECLARE_DELEGATE_OneParam(FOnAssetCompileCheckFinished, const TArray<FFBESCompileResult>&);

/**
 * This class runs on a worker thread beware!
 */
class AssetCompileChecker
{
	
public:

	AssetCompileChecker(const TArray<FAssetData>& InAssetsToCheck)
	{
		AssetsToCheck = InAssetsToCheck;
	}
	FOnAssetCompileCheckFinished OnCheckFinishedDelegate;

	void Run() { CheckAssets(); }

protected:

	void CheckAssets();
	void OnAssetCompileCheckFinished();

	bool HasBlueprintCompileErrors(UBlueprint* InBlueprint) const;
	FORCEINLINE UObject* LoadObject(UClass* Class, const FString& ObjectPath) const
	{
		return StaticLoadObject(Class, nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_DisableCompileOnLoad);
	}

	TArray<FAssetData> AssetsToCheck;
	TArray<FFBESCompileResult> Results;
	
	
};
