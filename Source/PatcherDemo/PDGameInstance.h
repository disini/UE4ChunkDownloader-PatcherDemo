// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "PDGameInstance.generated.h"


// patching delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPatchCompleteDelegate, bool, Succeeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChunkMountedDelegate, int32, ChunkID, bool, Succeeded);

// reports game patching stats that can be used for progress feedback
USTRUCT(BlueprintType)
struct FPPatchStats
{
	GENERATED_BODY()

		UPROPERTY(BlueprintReadOnly)
		int32 FilesDownloaded;

	UPROPERTY(BlueprintReadOnly)
		int32 TotalFilesToDownload;

	UPROPERTY(BlueprintReadOnly)
		float DownloadPercent;

	UPROPERTY(BlueprintReadOnly)
		int32 MBDownloaded;

	UPROPERTY(BlueprintReadOnly)
		int32 TotalMBToDownload;

	UPROPERTY(BlueprintReadOnly)
		FText LastError;
};

/**
 *  Extends GameInstance functionality to provide runtime chunk download and patching
 */
UCLASS()
class PATCHERDEMO_API UPDGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Overrides */
	virtual void Init() override;
	virtual void Shutdown() override;

	/** Initializes the patching system with the passed deployment name */
	UFUNCTION(BlueprintCallable, Category = "Patching")
		void  InitPatching(const FString& VariantName);

	/** Receives the version query response  */
	void OnPatchVersionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	/** Starts the game patching process. Returns false if the patching manifest is not up to date. */
	UFUNCTION(BlueprintCallable, Category = "Patching")
		bool PatchGame();

	/** Returns a patching status report we can use to populate progress bars, etc. */
	UFUNCTION(BlueprintPure, Category = "Patching")
		FPPatchStats GetPatchStatus();

	/** Fired when the patching manifest has been queried and we're ready to decide whether to patch the game or not*/
	UPROPERTY(BlueprintAssignable, Category = "Patching");
	FPatchCompleteDelegate OnPatchReady;


	/** Fired when the patching process succeeds or fails */
	UPROPERTY(BlueprintAssignable, Category = "Patching");
		FPatchCompleteDelegate OnPatchComplete;





protected:

	FString PatchPlatform;

	/** URL to query for patch version */
	UPROPERTY(EditDefaultsOnly, Category = "Patching")
		FString PatchVersionURL;

	/** List of Chunk IDs to try and download */
	UPROPERTY(EditDefaultsOnly, Category = "Patching")
		TArray<int32> ChunkDownloadList;

	/** Set to true once the patching manifest is checked for updates */
	bool bIsDownloadManifestUpToDate = false;

	/** Set to true if the main patching process is in progress */
	bool bIsPatchingGame = false;

	/** Called when the chunk download process finishes */
	void OnDownloadComplete(bool bSuccess);

public:

	/** Returns true if a given chunk is downloaded and mounted */
	UFUNCTION(BlueprintPure, Category = "Patching")
		bool IsChunkLoaded(int32 ChunkID);

	/** Fired when the patching process succeeds or fails */
	UPROPERTY(BlueprintAssignable, Category = "Patching");
		FPatchCompleteDelegate OnSingleChunkPatchComplete;

	/** List of single chunks to download separate of the main patching process */
	TArray<int32> SingleChunkDownloadList;

	/** If true, the system is currently downloading individual chunks */
	bool bIsDownloadingSingleChunks = false;

	/** Returns whether the system is currently downloading single chunks */
	UFUNCTION(BlueprintPure, Category = "Patching")
		bool IsDownloadingSingleChunks()
	{
		return bIsDownloadingSingleChunks;
	}

	/** Adds a single chunk to the download list and starts the load and mount process */
	UFUNCTION(BlueprintCallable, Category = "Patching")
		bool DownloadSingleChunk(int32 ChunkID);

	/** Called when single chunk download process finishes */
	void OnSingleChunkDownloadComplete(bool bSuccess);

	//UFUNCTION(BlueprintPure, Category = "Patching")

};
