
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FBESStruct.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFBES, Log, All);

USTRUCT()
struct FFBESCompileResult
{
	GENERATED_BODY()

	UPROPERTY()
	FString AssetPath;
	UPROPERTY()
	uint32 NumErrors = 0;

	FORCEINLINE bool HasErrors() const { return NumErrors > 0; }
};