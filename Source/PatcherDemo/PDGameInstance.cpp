// Fill out your copyright notice in the Description page of Project Settings.


#include "PDGameInstance.h"
#include "ChunkDownloader.h"
#include "Misc/CoreDelegates.h"
#include "AssetRegistryModule.h"

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

}

void UPDGameInstance::OnPatchVersionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{

}

bool UPDGameInstance::PatchGame()
{

}

FPPatchStats UPDGameInstance::GetPatchStatus()
{

}

void UPDGameInstance::OnDownloadComplete(bool bSuccess)
{

}

bool UPDGameInstance::IsChunkLoaded(int32 ChunkID)
{

}

bool UPDGameInstance::DownloadSingleChunk(int32 ChunkID)
{

}

void UPDGameInstance::OnSingleChunkDownloadComplete(bool bSuccess)
{

}
