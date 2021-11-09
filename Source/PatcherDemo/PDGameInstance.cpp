// Fill out your copyright notice in the Description page of Project Settings.


#include "PDGameInstance.h"
#include "ChunkDownloader.h"
#include "Misc/CoreDelegates.h"
#include "AssetRegistryModule.h"


DEFINE_LOG_CATEGORY_STATIC(LogPatch, Display, Display);

void UPDGameInstance::Init()
{
	Super::Init();
}

void UPDGameInstance::Shutdown()
{
	Super::Shutdown();

	// Shutdown the chunk downloader
	FChunkDownloader::Shutdown();
}

void UPDGameInstance::InitPatching(const FString& VariantName)
{
	// save the patch platform
	PatchPlatform = VariantName;
	
	// get the HTTP module
	FHttpModule& Http = FHttpModule::Get();

	// create a new HTTP request and bind the response callback
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http.CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UPDGameInstance::OnPatchVersionResponse);

	// configure and send the request
	Request->SetURL(PatchVersionURL);
	Request->SetVerb("GET");
	Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", TEXT("application/json"));
	Request->ProcessRequest();


}

void UPDGameInstance::OnPatchVersionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	// deployment name to use for this patcher configuration
	const FString DeploymentName = "Patcher-Live";

	// content build ID. Our Http response will provide this info
	// so we can update to new patch versions as they become available on the server
	FString ContentBuildId = Response->GetContentAsString();

	UE_LOG(LogPatch, Display, TEXT("Patch Content ID Response: %s"), *ContentBuildId);

	// -----------------------------------------------------

	UE_LOG(LogPatch, Display, TEXT("Getting Chunk Downloader"));

	// initialize the chunk downloader with the chosen platform
	TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetOrCreate();
	Downloader->Initialize(PatchPlatform, 8);

	UE_LOG(LogPatch, Display, TEXT("Loading Cached Build ID"));

	// Try to load the cached build manifest data if it exists.
	// This will populate the Chunk Downloader state with the most recently downloaded manifest data.
	if (Downloader->LoadCachedBuild(DeploymentName))
	{
		UE_LOG(LogPatch, Display, TEXT("Cached Build Succeeded"));
	}
	else {
		UE_LOG(LogPatch, Display, TEXT("Cached Build Failed"));
	}

	UE_LOG(LogPatch, Display, TEXT("Updating Build Manifest"));

	// Check with the server if there's a newer manifest file that matches our data, and download it.
	// ChunkDownloader uses a lambda function to let us know when the async download is complete.
	TFunction<void(bool)> ManifestCompleteCallback = [this](bool bSuccess)
	{
		// write to the log
		if (bSuccess)
		{
			UE_LOG(LogPatch, Display, TEXT("Manifest Update Succeeded"));			
		}
		else {
			UE_LOG(LogPatch, Display, TEXT("Manifest Update Failed"));
		}

		// Flag the manifest's status so we know whether it's safe to patch or not
		bIsDownloadManifestUpToDate = bSuccess;

		// call our delegate to let the game know we're ready to start patching
		OnPatchReady.Broadcast(bSuccess);
	};

	// Start the manifest update process, passing the BuildID and Lambda callback
	Downloader->UpdateBuild(DeploymentName, ContentBuildId, ManifestCompleteCallback);

}

bool UPDGameInstance::PatchGame()
{
	// Make sure the download manifest is up to date
	if (!bIsDownloadManifestUpToDate)
	{
		// we couldn't contact the server to validate our manifest, so we can't patch
		UE_LOG(LogPatch, Display, TEXT("Manifest Update Failed. Can't patch the game"));

		return false;
	}
	UE_LOG(LogPatch, Display, TEXT("Starting the game patch process."));

	// get the chunk downloader
	TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

	// raise the patching flag
	bIsPatchingGame = true;

	// Write the current status for each chunk file from the manifest.
	// This way we can find out if a specific chunk is already downloaded, in progress, pending, etc.
	for (int32 ChunkID : ChunkDownloadList)
	{
		// print the chunk status to the log for debug purposes. We can do more complex logic here if we want.
		int32 ChunkStatus = static_cast<int32>(Downloader->GetChunkStatus(ChunkID));
		UE_LOG(LogPatch, Display, TEXT("Chunk %i status: %i"), ChunkID, ChunkStatus);
	}
	
	// prepare the pak mounting callback lambda function
	TFunction<void(bool)> MountCompleteCallback = [this](bool bSuccess)
	{
		// lower the chunk mount flag
		bIsPatchingGame = false;
		if (bSuccess)
		{
			UE_LOG(LogPatch, Display, TEXT("Chunk Mount Complete"));

			// call the delegate
			OnPatchComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogPatch, Display, TEXT("Chunk Mount failed"));

			// call the delegate
			OnPatchComplete.Broadcast(false);
		}
	};

	// mount the Paks so their assets can be accessed
	Downloader->MountChunks(ChunkDownloadList, MountCompleteCallback);

	// Note: This is not necessary, since MountChunk calls DownloadChunk under the hood.The Download Complete function is also unnecessary

	/*
	// Prepare the download Complete Lambda.
	// This will be called whenever the download process finishes.
	// We can use it to run further game logic on success/failure.
	
	
	
	*/
	
	






	
	
	
	
	
	
	
	return true;
}

FPPatchStats UPDGameInstance::GetPatchStatus()
{
	// get the chunk downloader
	TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

	// get the loading stats
	FChunkDownloader::FStats LoadingStats = Downloader->GetLoadingStats();

	// copy the info into the output stats.
	// ChunkDownloader tracks bytes downloaded as uint64s, which have no BP support,
	// So we divide to MB and cast to int32 (signed) to avoid overflow and interpretation errors.
	FPPatchStats RetStats;

	RetStats.FilesDownloaded = LoadingStats.TotalFilesToDownload;
	RetStats.MBDownloaded = (int32)(LoadingStats.BytesDownloaded / (1024 * 1024));
	RetStats.TotalMBToDownload = (int32)(LoadingStats.TotalBytesToDownload / (1024 * 1024));
	RetStats.DownloadPercent = (float)LoadingStats.BytesDownloaded / (float)LoadingStats.TotalBytesToDownload;
	RetStats.LastError = LoadingStats.LastError;

	// return the status struct
	return RetStats;

}

void UPDGameInstance::OnDownloadComplete(bool bSuccess)
{
	// was Pak download sucessful?
	if (!bSuccess)
	{
		UE_LOG(LogPatch, Display, TEXT("Patch Download Failed"));

		// call the delegate
		OnPatchComplete.Broadcast(false);

		return;
	}

	UE_LOG(LogPatch, Display, TEXT("Patch Download Complete"));

	// get the chunk downloader
	TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

	// build the downloaded chunk list
	FJsonSerializableArrayInt DownloadedChunks;

	for (int32 ChunkID : ChunkDownloadList)
	{
		DownloadedChunks.Add(ChunkID);
	}

	// prepare the pak mounting callback lambda function
	TFunction<void(bool)> MountCompleteCallback = [this](bool bSuccess)
	{
		// lower the chunk mount flag
		bIsPatchingGame = false;

		if (bSuccess)
		{
			UE_LOG(LogPatch, Display, TEXT("Chunk Mount complete"));

			// call the delegate
			OnPatchComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogPatch, Display, TEXT("Chunk Mount failed"));

			// call the delegate
			OnPatchComplete.Broadcast(false);
		}
	};

	// mount the Paks so their assets can be accessed
	Downloader->MountChunks(DownloadedChunks, MountCompleteCallback);

}

bool UPDGameInstance::IsChunkLoaded(int32 ChunkID)
{
	// ensure our manifest is up to date
	if (!bIsDownloadManifestUpToDate)
		return false;
	
	// get the chunk downloader
	TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

	// query the chunk downloader for chunk status.Return true only if the chunk is mounted
	return Downloader->GetChunkStatus(ChunkID) == FChunkDownloader::EChunkStatus::Mounted;
}

bool UPDGameInstance::DownloadSingleChunk(int32 ChunkID)
{
	// Make sure the download manifest is up to date
	if (!bIsDownloadManifestUpToDate)
	{
		// we couldn't contact the server to validate our manifest, so we can't patch
		UE_LOG(LogPatch, Display, TEXT("Manifest Update Failed. Can't patch the game"));
		
		return false;
	}
	
	// ignore individual downloads if we're still patching the game
	if (bIsPatchingGame)
	{
		UE_LOG(LogPatch, Display, TEXT("Main game patching underway. Ignoring single chunk downloads."));
		return false;
	}

	// make sure we're not trying to download the chunk multiple times
	if (SingleChunkDownloadList.Contains(ChunkID))
	{
		UE_LOG(LogPatch, Display, TEXT("Chunk: %i already downloading"), ChunkID);

		return false;
	}

	// raise the single chunk download flag.
	bIsDownloadingSingleChunks = true;

	// add the chunk to our download list
	SingleChunkDownloadList.Add(ChunkID);


	UE_LOG(LogPatch, Display, TEXT("Downloading specific Chunk: %i"), ChunkID);

	// get the chunk downloader
	TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

	// prepare the pak mounting callback lambda function
	TFunction<void(bool)> MountCompleteCallback = [this](bool bSuccess)
	{
		if (bSuccess)
		{
			UE_LOG(LogPatch, Display, TEXT("Single Chunk Mount complete"));

			// call the delegate
			OnSingleChunkPatchComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogPatch, Display, TEXT("Single Chunk Mount failed"));

			// call the delegate
			OnSingleChunkPatchComplete.Broadcast(false);
		}
	};

	// mount the Paks so their assets can be accessed
	Downloader->MountChunk(ChunkID, MountCompleteCallback);


	
	return true;
}

void UPDGameInstance::OnSingleChunkDownloadComplete(bool bSuccess)
{
	// lower the download chunks flag
	bIsDownloadingSingleChunks = false;

	// was Pak download successful?
	if (!bSuccess)
	{
		UE_LOG(LogPatch, Display, TEXT("Patch Download failed"));

		// call the delegate
		OnSingleChunkPatchComplete.Broadcast(false);

		return;
	}

	UE_LOG(LogPatch, Display, TEXT("Patch Download Complete"));

	// get the chunl downloader
	TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

	// build the downloaded chunk list
	FJsonSerializableArrayInt DownloadedChunks;

	for (int32 ChunkID : SingleChunkDownloadList)
	{
		DownloadedChunks.Add(ChunkID);
	}

	// prepare the pak mounting callback lambda function
	TFunction<void(bool)> MountCompleteCallback = [this](bool bSuccess)
	{
		if (bSuccess)
		{
			UE_LOG(LogPatch, Display, TEXT("Single Chunk Mount complete"));

			// call the delegate
			OnSingleChunkPatchComplete.Broadcast(true);
		}
		else {
			UE_LOG(LogPatch, Display, TEXT("Single Mount failed"));

			// call the delegate
			OnSingleChunkPatchComplete.Broadcast(false);
		}
	};

	// mount the Paks so their assets can be accessed
	Downloader->MountChunks(DownloadedChunks, MountCompleteCallback);



}
