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
	return true;
}

FPPatchStats UPDGameInstance::GetPatchStatus()
{


	FPPatchStats RetStats;

	return RetStats;
}

void UPDGameInstance::OnDownloadComplete(bool bSuccess)
{

}

bool UPDGameInstance::IsChunkLoaded(int32 ChunkID)
{
	return true;
}

bool UPDGameInstance::DownloadSingleChunk(int32 ChunkID)
{
	return true;
}

void UPDGameInstance::OnSingleChunkDownloadComplete(bool bSuccess)
{

}
