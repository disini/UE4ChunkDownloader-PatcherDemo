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
