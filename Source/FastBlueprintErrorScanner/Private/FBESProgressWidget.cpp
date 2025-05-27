
#include "FBESProgressWidget.h"
#include "AssetCompileChecker.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Async/Async.h"
#include "Components/Button.h"
#include "Components/CircularThrobber.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Misc/FileHelper.h"


void UFBESProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	InitWidget();
}

void UFBESProgressWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// may need to only do this if we arnt finished with work.
	if (!IsWorkFinished()) UpdateProgressUI();
}

void UFBESProgressWidget::InitWidget()
{
	NumProcsFinishedWork = 0;
	Text_ProgressPercent->SetText(FText::FromString(TEXT("[0%]")));
	Text_CountPass->SetText(FText::FromString(TEXT("0")));
	Text_CountError->SetText(FText::FromString(TEXT("0")));
	Text_EclipseTime->SetText(FText::FromString(TEXT("00:00")));
	Button_Close->OnClicked.AddUniqueDynamic(this, &UFBESProgressWidget::OnClickedButtonClose);
	Button_Close->SetIsEnabled(false);
	ProgressBar_Percent->SetPercent(0);
	CircularThrobber_Circle->SetVisibility(ESlateVisibility::Visible);
	Text_Title->SetText(FText::FromString(TEXT("Scanning")));
}

void UFBESProgressWidget::OnWorkComplete(const TArray<FFBESCompileResult>& Results)
{
	// Called on a worker thread.
	AsyncTask(ENamedThreads::GameThread, [this, Results]()
	{
		ResultsData.Append(Results);
		NumProcsFinishedWork += 1;
		if (IsWorkFinished()) OnAllWorkComplete();
	});
}

void UFBESProgressWidget::OnAllWorkComplete()
{
	UpdateProgressUI();

	CircularThrobber_Circle->SetVisibility(ESlateVisibility::Collapsed);
	Text_Title->SetText(FText::FromString("Completed"));
	Button_Close->SetIsEnabled(true);
}

void UFBESProgressWidget::OnClickedButtonClose()
{
	// ReSharper disable once CppExpressionWithoutSideEffects
	OnCloseWidgetDelegate.ExecuteIfBound(ResultsData);

	const TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(TakeWidget());
	if (Window.IsValid()) Window->RequestDestroyWindow();
}

void UFBESProgressWidget::UpdateProgressUI()
{
	const FTimespan Gap = FDateTime::Now() - StartTime;
	Text_EclipseTime->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Gap.GetMinutes(), Gap.GetSeconds())));
	
	uint32 ErrorCount = 0;
	for (const FFBESCompileResult& Result : ResultsData)
	{
		if (Result.HasErrors()) ErrorCount++;
	}
	const uint32 PassCount = ResultsData.Num() - ErrorCount;
	
	// Progress 0 - 1.
	float NormalizedProgress = 0;
	if (TotalAssetsToProcess > 0)
	{
		NormalizedProgress = (PassCount + ErrorCount) / TotalAssetsToProcess;
		NormalizedProgress = FMath::Clamp(NormalizedProgress, 0.0f, 1.0f);
	}
	const int8 ProgressPercent = FMath::Clamp(NormalizedProgress * 100.0f, 0, 100);

	ProgressBar_Percent->SetPercent(NormalizedProgress);
	Text_ProgressPercent->SetText(FText::FromString(FString::Printf(TEXT("[%d%%]"), ProgressPercent)));
	Text_CountPass->SetText(FText::FromString(FString::FromInt(PassCount)));
	Text_CountError->SetText(FText::FromString(FString::FromInt(ErrorCount)));
}

void UFBESProgressWidget::RunWork()
{
	StartTime = FDateTime::Now();
	NumProcsRunningWork = bRunAsMultiThread ? FPlatformMisc::NumberOfCores() : 1;
	
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	AssetRegistryModule.Get().SearchAllAssets(true);
	
	TArray<FAssetData> AssetsToProcess;
	AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), AssetsToProcess, true);
	TotalAssetsToProcess = AssetsToProcess.Num();

	// If not multi-threaded, we can just run the all work on this one thread.
	if (!bRunAsMultiThread)
	{
		// Run the work on a separate thread.
		AssetCompileChecker* Checker = new AssetCompileChecker(AssetsToProcess);
		Checker->OnCheckFinishedDelegate.BindUObject(this, &UFBESProgressWidget::OnWorkComplete);
		Checker->Run();
		return;
	}
	// If we are multi-threaded, we need to split the work into chunks.
	const int32 ChunkSize = FMath::CeilToInt(static_cast<float>(AssetsToProcess.Num()) / NumProcsRunningWork);
	for (int32 i = 0; i < NumProcsRunningWork; ++i)
	{
		const int32 StartIndex = i * ChunkSize;
		if (StartIndex >= AssetsToProcess.Num()) break;

		const int32 EndIndex = FMath::Min(StartIndex + ChunkSize, AssetsToProcess.Num());

		// Slice creates a lightweight view of the data
		TConstArrayView<FAssetData> AssetSlice = MakeArrayView(AssetsToProcess).Slice(StartIndex, EndIndex - StartIndex);
		// Copy the slice into a TArray to pass to the thread
		TArray<FAssetData> AssetChunk(AssetSlice);

		Async(EAsyncExecution::Thread, [this, Chunk = MoveTemp(AssetChunk)]()
		{
			AssetCompileChecker* Checker = new AssetCompileChecker(Chunk);
			Checker->OnCheckFinishedDelegate.BindUObject(this, &UFBESProgressWidget::OnWorkComplete);
			Checker->Run();
		});
	}
}