#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "FBESStruct.h"
#include "FBESProgressWidget.generated.h"


class UProgressBar;
class UButton;
class UTextBlock;
class SWindow;
class UCircularThrobber;

DECLARE_DELEGATE_OneParam(FOnCloseWidgetDelegate, const TArray<FFBESCompileResult>&);


UCLASS()
class FASTBLUEPRINTERRORSCANNER_API UFBESProgressWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:

	FOnCloseWidgetDelegate OnCloseWidgetDelegate;
	FORCEINLINE void SetRunAsMultiThread(const bool bMultiThread) { bRunAsMultiThread = bMultiThread; }
	void RunWork();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ProgressPercent;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CountPass;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CountError;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_EclipseTime;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Close;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Percent;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCircularThrobber> CircularThrobber_Circle;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Title;

protected:
	
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void InitWidget();
	void UpdateProgressUI();

	FORCEINLINE bool IsWorkFinished() const { return NumProcsRunningWork == NumProcsFinishedWork; }
	void OnWorkComplete(const TArray<FFBESCompileResult>& Results);
	void OnAllWorkComplete();
	UFUNCTION()
	void OnClickedButtonClose();
	
	uint16 NumProcsRunningWork = 0;
	uint16 NumProcsFinishedWork = 0;

	uint64 TotalAssetsToProcess = 0;
	
	FDateTime StartTime;
	bool bRunAsMultiThread = true;
	TArray<FFBESCompileResult> ResultsData;
	
};
