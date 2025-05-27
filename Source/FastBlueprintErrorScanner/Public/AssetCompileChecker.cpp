

#include "AssetCompileChecker.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"


void AssetCompileChecker::CheckAssets()
{
	Results.Reserve(AssetsToCheck.Num());
	for (const FAssetData& AssetToCheck : AssetsToCheck)
	{
		// Load the BP to check.
		const FString AssetPath = AssetToCheck.GetObjectPathString();
		if (!AssetToCheck.GetClass()) continue;
		UBlueprint* LoadedBlueprint = Cast<UBlueprint>(LoadObject(AssetToCheck.GetClass(), AssetPath));
		if (!LoadedBlueprint) continue;

		// Check and record compile errors.
		FFBESCompileResult CompileResult { AssetPath, 0 };
		CompileResult.NumErrors = HasBlueprintCompileErrors(LoadedBlueprint);
		Results.Add(CompileResult);
	}
	OnAssetCompileCheckFinished();
}

void AssetCompileChecker::OnAssetCompileCheckFinished()
{
	// ReSharper disable once CppExpressionWithoutSideEffects
	OnCheckFinishedDelegate.ExecuteIfBound(Results);
	// Full cleanup.
	Results.Empty();
	AssetsToCheck.Empty();
	delete this;
}

bool AssetCompileChecker::HasBlueprintCompileErrors(UBlueprint* InBlueprint) const
{
	if (!InBlueprint) return false;
	
	static TSet<EBlueprintStatus, DefaultKeyFuncs<EBlueprintStatus>, TInlineSetAllocator<4>> NeedRecompileStatuses = { BS_Unknown, BS_Dirty };
	if (NeedRecompileStatuses.Contains(InBlueprint->Status))
	{
		FKismetEditorUtilities::CompileBlueprint(InBlueprint, EBlueprintCompileOptions::SkipSave);
	}

	return InBlueprint->Status == BS_Error;
}
