#include "../Public/SpoutBPFunctionLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h" // SMODE TECH, Fix UE 4.25
#endif
#include "SpoutDirectX.h" 
#include "SpoutFrameCount.h" // Smode Tech
#include <d3d11.h>
#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h" // SMODE TECH, Fix UE 4.25
#endif


#include "SpoutPluginPrivatePCH.h"
#include "../Public/SpoutModule.h"

// Smode Tech, add includes
#include "SpoutSenderNames.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "opengl32.lib") // SMODE TECH, Link directly with OpenGL

static ID3D11Device* g_D3D11Device;
static ID3D11DeviceContext* g_pImmediateContext = NULL;

static spoutSenderNames * sender;
//static spoutGLDXinterop * interop; // Smode Tech useless
static spoutDirectX * sdx;


#include "D3D11On12Interop.h" // SMODE TECH
static FD3D11On12Interop dx12Interop; // Smode Tech

static TArray<FSenderStruct> FSenders;

static UMaterialInterface* BaseMaterial;

static FName TextureParameterName = "SpoutTexture";


void FSpoutModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
	// Free the dll handle
	/*FPlatformProcess::FreeDllHandle(ExampleLibraryHandle);
	ExampleLibraryHandle = nullptr;*/

	FSenders.Init({}, 0);
	if (sender)
	{
		delete sender;
		sender = nullptr;
	}
	if (sdx)
	{
		delete sdx;
		sdx = nullptr;
	}
  
  // SmodeTech
  ENQUEUE_RENDER_COMMAND(void)(
    [this](FRHICommandListImmediate& RHICmdList)
    {return dx12Interop.releaseInterop(); }
  );
  FlushRenderingCommands();
  // --

	UE_LOG(SpoutUELog, Warning, TEXT("Spout Module Shutdown"));
}


void DestroyTexture(UTexture2D*& Texture)
{
	// Here we destory the texture and its resource
	if (Texture){
		Texture->RemoveFromRoot();

		if (Texture->GetResource())
		{
			BeginReleaseResource(Texture->GetResource());
			FlushRenderingCommands();
		}

		Texture->MarkAsGarbage();
		Texture = nullptr;
	}else{
		UE_LOG(SpoutUELog, Warning, TEXT("Texture is ready"));
	}
}

void ResetMatInstance(UTexture2D*& Texture, UMaterialInstanceDynamic*& MaterialInstance)
{


	if (!Texture || !BaseMaterial || TextureParameterName.IsNone())
	{
		UE_LOG(SpoutUELog, Warning, TEXT("UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU"));
		return;
	}

	// Create material instance
	if (!MaterialInstance)
	{
		MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, NULL);
		if (!MaterialInstance)
		{
			UE_LOG(SpoutUELog, Warning, TEXT("Material instance can't be created"));
			return;
		}
	}

	// Check again, we must have material instance
	if (!MaterialInstance)
	{
		UE_LOG(SpoutUELog, Error, TEXT("Material instance wasn't created"));
		return;
	}

	// Check we have desired parameter
	UTexture* Tex = nullptr;
	if (!MaterialInstance->GetTextureParameterValue(TextureParameterName, Tex))
	{
		UE_LOG(SpoutUELog, Warning, TEXT("UI Material instance Texture parameter not found"));
		return;
	}

	MaterialInstance->SetTextureParameterValue(TextureParameterName, Texture);
}

void ResetTexture(UTexture2D*& Texture, UMaterialInstanceDynamic*& MaterialInstance, FSenderStruct*& SenderStruct)
{

	// Here we init the texture to its initial state
	DestroyTexture(Texture);
	//UE_LOG(SpoutUELog, Warning, TEXT("Texture is ready???????2222"));
	// init the new Texture2D
	Texture = UTexture2D::CreateTransient(SenderStruct->w, SenderStruct->h, PF_B8G8R8A8);
	Texture->AddToRoot();
	Texture->UpdateResource();
	//UE_LOG(SpoutUELog, Warning, TEXT("Texture is ready???????333333"));
	SenderStruct->Texture2DResource = Texture->GetResource();

	////////////////////////////////////////////////////////////////////////////////
	ResetMatInstance(Texture, MaterialInstance);


}

UTextureRenderTarget2D* USpoutBPFunctionLibrary::CreateTextureRenderTarget2D(int32 w, int32 h, EPixelFormat pixelFormat, bool forceLinearGamma) {
	UTextureRenderTarget2D* textureTarget = NewObject<UTextureRenderTarget2D>();
	textureTarget->bNeedsTwoCopies = true;
	textureTarget->InitCustomFormat(w, h, pixelFormat, forceLinearGamma);
	textureTarget->AddressX = TextureAddress::TA_Wrap;
	textureTarget->AddressY = TextureAddress::TA_Wrap;
	#if WITH_EDITORONLY_DATA
	textureTarget->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
	#endif

	textureTarget->AddToRoot();
	//textureTarget->UpdateResourceW();
	textureTarget->UpdateResource();

	return textureTarget;
}

void initSpout()
{
	UE_LOG(SpoutUELog, Warning, TEXT("-----------> Init Spout"));
	
	sender = new spoutSenderNames;
	// interop = new spoutGLDXinterop; // SMODE TECH useless
	sdx = new spoutDirectX;
}

void GetDevice()
{
	//UE_LOG(SpoutUELog, Warning, TEXT("-----------> Set Graphics Device D3D11"));

  // SmodeTech
  dx12Interop.createInterop(GDynamicRHI);
  g_D3D11Device = dx12Interop.getDevice11();
  g_pImmediateContext = dx12Interop.getDeviceContext11();
  // --

	//g_D3D11Device = (ID3D11Device*)GDynamicRHI->RHIGetNativeDevice();
	//g_D3D11Device->GetImmediateContext(&g_pImmediateContext);
}

int32 USpoutBPFunctionLibrary::SetMaxSenders(int32 max){
	if (sender == nullptr)
	{
		initSpout();

	};

	sender->SetMaxSenders(max);
	return max;
}

void USpoutBPFunctionLibrary::GetMaxSenders(int32& max) {
	if (sender == nullptr)
	{
		initSpout();

	};

	max = sender->GetMaxSenders();
}


bool GetSpoutRegistred(FName spoutName, FSenderStruct*& SenderStruct) {

	auto MyPredicate = [&](const FSenderStruct InItem) {return InItem.sName == spoutName; };
	SenderStruct = FSenders.FindByPredicate(MyPredicate);

	bool bIsInListSenders = FSenders.ContainsByPredicate(MyPredicate);
	return bIsInListSenders;

}

void UnregisterSpout(FName spoutName) {
	auto MyPredicate = [&](const FSenderStruct InItem) {return InItem.sName == spoutName; };
	FSenders.RemoveAll(MyPredicate);
}

void ClearRegister() {

	FSenders.Empty();
}

FSenderStruct* RegisterReceiver(FName spoutName){
	
	unsigned int w;
	unsigned int h;
	HANDLE sHandle;
	unsigned long format;

	sender->GetSenderInfo(TCHAR_TO_ANSI(*spoutName.ToString()), w, h, sHandle, format);

	FSenderStruct* newFSenderStruc = new FSenderStruct();
	newFSenderStruc->SetH(h);
	newFSenderStruc->SetW(w);
	newFSenderStruc->SetHandle(sHandle);
	newFSenderStruc->SetName(spoutName);
	newFSenderStruc->bIsAlive = true;
	newFSenderStruc->spoutType = ESpoutType::Receiver;
	newFSenderStruc->MaterialInstanceColor = nullptr;
	newFSenderStruc->TextureColor = nullptr;
	newFSenderStruc->sharedResource = nullptr;
	newFSenderStruc->rView = nullptr;
	newFSenderStruc->texTemp = NULL;
	newFSenderStruc->frame = nullptr;// Smode Tech no frame counter on receiver

	//TextureColor = new
	UE_LOG(SpoutUELog, Warning, TEXT("No material instance, creating...//////"));
	// Prepara Textura, Set the texture update region
	newFSenderStruc->UpdateRegions = new FUpdateTextureRegion2D(0, 0, 0, 0, newFSenderStruc->w, newFSenderStruc->h);
	ResetTexture(newFSenderStruc->TextureColor, newFSenderStruc->MaterialInstanceColor, newFSenderStruc);

	UE_LOG(SpoutUELog, Warning, TEXT("--starting...--___Open Shared Resource___---"));
	HRESULT openResult = g_D3D11Device->OpenSharedResource(newFSenderStruc->sHandle, __uuidof(ID3D11Resource), (void**)(&newFSenderStruc->sharedResource));

	if (FAILED(openResult)) {
		UE_LOG(SpoutUELog, Error, TEXT("--FAIL--___Open Shared Resource___---"));
		return false;

	}
	UE_LOG(SpoutUELog, Warning, TEXT("--starting...--___Create Shader Resource View___---"));
	HRESULT createShaderResourceViewResult = g_D3D11Device->CreateShaderResourceView(newFSenderStruc->sharedResource, NULL, &newFSenderStruc->rView);
	if (FAILED(createShaderResourceViewResult)) {
		UE_LOG(SpoutUELog, Error, TEXT("--FAIL--___Create Shader Resource View___---"));
		return false;

	}

	//texture shared for the spout resource handle
	ID3D11Texture2D* tex = (ID3D11Texture2D*)newFSenderStruc->sharedResource;

	if (tex == nullptr) {
		UE_LOG(SpoutUELog, Error, TEXT("---|||------||||----"));
		return false;
	}
	D3D11_TEXTURE2D_DESC description;
	tex->GetDesc(&description);
	description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	description.Usage = D3D11_USAGE_STAGING;
	description.BindFlags = 0;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	description.MiscFlags = 0;
	description.MipLevels = 1;
	description.ArraySize = 1;


	UE_LOG(SpoutUELog, Warning, TEXT("--- Creating d3d11 Texture 2D---"));

	HRESULT hr = g_D3D11Device->CreateTexture2D(&description, NULL, &newFSenderStruc->texTemp);

	if (FAILED(hr))
	{
		std::stringstream ss;
		ss << " Error code = 0x" << std::hex << hr << std::endl;
		std::cout << ss.str() << std::endl;
		std::string TestString = ss.str();
		UE_LOG(SpoutUELog, Error, TEXT("Failed to create texture of name: ----> %s"), *FString(TestString.c_str()));

		if (hr == E_OUTOFMEMORY) {
			UE_LOG(SpoutUELog, Error, TEXT("OUT OF MEMORY"));
		}
		if (newFSenderStruc->texTemp)
		{
			newFSenderStruc->texTemp->Release();
			newFSenderStruc->texTemp = NULL;
		}
		UE_LOG(SpoutUELog, Error, TEXT("Error creating temporal textura"));
		return false;

	}
	
	FSenders.Add(*newFSenderStruc);
	
	return newFSenderStruc;

}

bool USpoutBPFunctionLibrary::SpoutInfo(TArray<FSenderStruct>& Senders){
	Senders = FSenders;
	return true;
}

bool USpoutBPFunctionLibrary::SpoutInfoFrom(FName spoutName, FSenderStruct& SenderStruct){
	
	//Existe en mi lista ??
	auto MyPredicate = [&](const FSenderStruct InItem) {return InItem.sName == spoutName; };
	FSenderStruct* EncontradoSenderStruct = FSenders.FindByPredicate(MyPredicate);

	if (EncontradoSenderStruct == nullptr){
		UE_LOG(SpoutUELog, Warning, TEXT("No Sender was found with the name : %s"), *spoutName.GetPlainNameString());
		return false;
	}
	else
	{
		SenderStruct = *EncontradoSenderStruct;
	}

	return true;
}


bool USpoutBPFunctionLibrary::CreateRegisterSender(FName spoutName, unsigned int width, unsigned int height, DXGI_FORMAT format)
{

	if (g_D3D11Device == nullptr || g_pImmediateContext == NULL){
		UE_LOG(SpoutUELog, Warning, TEXT("Getting Device..."));
		GetDevice();
	}

	HANDLE sharedSendingHandle = NULL;
	bool texResult = false;
	bool updateResult = false;
	bool senderResult = false;
	
	ID3D11Texture2D * sendingTexture = nullptr; // Smode Tech Fix Crash

	UE_LOG(SpoutUELog, Display, TEXT("ID3D11Texture2D Info : width_%i, height_%i"), width, height);
	UE_LOG(SpoutUELog, Display, TEXT("ID3D11Texture2D Info : Format is %i"), int(format));

	//use the pixel format from basetexture (the native texture textureRenderTarget2D)
	DXGI_FORMAT texFormat = format;
	if (format == DXGI_FORMAT_B8G8R8A8_TYPELESS) {
		texFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	}
	
	texResult = sdx->CreateSharedDX11Texture(g_D3D11Device, width, height, texFormat, &sendingTexture, sharedSendingHandle);
	UE_LOG(SpoutUELog, Display, TEXT("Create shared Texture with SDX : %i"), texResult);

	if (!texResult)
	{
		UE_LOG(SpoutUELog, Error, TEXT("SharedDX11Texture creation failed"));
		return 0;
	}

	const auto tmp = spoutName.GetPlainNameString();
	UE_LOG(SpoutUELog, Display, TEXT("Created Sender: name --> %s"), *tmp);

	senderResult = sender->CreateSender(TCHAR_TO_ANSI(*spoutName.ToString()), width, height, sharedSendingHandle, texFormat);
	UE_LOG(SpoutUELog, Display, TEXT("Created sender DX11 with sender name : %s"), *tmp);

	// remove old sender register
	auto MyPredicate = [&](const FSenderStruct InItem) {return InItem.sName == spoutName; };
	FSenders.RemoveAll(MyPredicate);

	// add new register
	UE_LOG(SpoutUELog, Display, TEXT("Adding Sender to Sender list"));
	FSenderStruct* newFSenderStruc = new FSenderStruct();
	newFSenderStruc->SetW(width);
	newFSenderStruc->SetH(height);
	newFSenderStruc->SetName(spoutName);
	newFSenderStruc->bIsAlive = true;
	newFSenderStruc->spoutType = ESpoutType::Sender;
	newFSenderStruc->SetHandle(sharedSendingHandle);
	newFSenderStruc->activeTextures = sendingTexture;

	newFSenderStruc->MaterialInstanceColor = nullptr;
	newFSenderStruc->TextureColor = nullptr;

	// Smode Tech frame counter
	newFSenderStruc->frame = new spoutFrameCount();
	bool frameResult = newFSenderStruc->frame->CreateAccessMutex(TCHAR_TO_ANSI(*spoutName.ToString()));
	if (!frameResult)
		UE_LOG(SpoutUELog, Warning, TEXT("CreateAccessMutex failure: %s"), *tmp);
	newFSenderStruc->frame->EnableFrameCount(TCHAR_TO_ANSI(*spoutName.ToString()));

	FSenders.Add(*newFSenderStruc);

	return senderResult;
	
}

ESpoutState CheckSenderState(FName spoutName){

	auto MyPredicate = [&](const FSenderStruct InItem) {return InItem.sName == spoutName; };
	bool bIsInListSenders = FSenders.ContainsByPredicate(MyPredicate);

	ESpoutState state = ESpoutState::noEnoR;

	if (sender->FindSenderName(TCHAR_TO_ANSI(*spoutName.ToString()))) {
		//UE_LOG(SpoutUELog, Warning, TEXT("Sender State: --> Exist"));
		if (bIsInListSenders) {
			//UE_LOG(SpoutUELog, Warning, TEXT("Sender State: --> Exist y Registred"));
			state = ESpoutState::ER;
		}
		else {
			//UE_LOG(SpoutUELog, Warning, TEXT("Sender State: --> Exist y No Registred"));
			state = ESpoutState::EnoR;
		}
	}
	else {
		//UE_LOG(SpoutUELog, Warning, TEXT("Sender State: --> No Exist"));
		if (bIsInListSenders) {
			//UE_LOG(SpoutUELog, Warning, TEXT("Sender State: --> No Exist y Registred"));
			state = ESpoutState::noER;
		}
		else {
			//UE_LOG(SpoutUELog, Warning, TEXT("Sender State: --> No Exist y No Registred"));
			state = ESpoutState::noEnoR;
		}
	}

	return state;

}

bool USpoutBPFunctionLibrary::SpoutSender(FName spoutName, ESpoutSendTextureFrom sendTextureFrom, UTextureRenderTarget2D* textureRenderTarget2D, float targetGamma, bool discardWarningMessage)
{
	if (sender == nullptr)
	{
		initSpout();

	};
	if (g_D3D11Device == nullptr || g_pImmediateContext == NULL) {

		GetDevice();
	}

  FTexture2DRHIRef sentTexture;
	FSenderStruct* SenderStruct = 0;
	
	switch (sendTextureFrom)
	{
	case ESpoutSendTextureFrom::GameViewport:
	{
		// add nullptr SmodeTech check
    sentTexture = GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport ? GEngine->GameViewport->Viewport->GetRenderTargetTexture().GetReference() : nullptr; // SModeTech check
		break;
	}
	case ESpoutSendTextureFrom::TextureRenderTarget2D:
		if (textureRenderTarget2D == nullptr) {
			UE_LOG(SpoutUELog, Warning, TEXT("No TextureRenderTarget2D Selected!!"));
			return false;
		}
		textureRenderTarget2D->TargetGamma = targetGamma;
    sentTexture = textureRenderTarget2D->GetResource() && textureRenderTarget2D->GetResource()->TextureRHI.IsValid() ? textureRenderTarget2D->GetResource()->TextureRHI->GetTexture2D() : nullptr;
		break;
	default:
		break;
	}
  
	if (sentTexture == nullptr) {
		if(!discardWarningMessage)
			UE_LOG(SpoutUELog, Warning, TEXT("Texture is not yet ready to be sent"));
		return false;
	}

  unsigned int width, height;
  DXGI_FORMAT format = dx12Interop.getTextureFormat(sentTexture, width, height);

	ESpoutState state = CheckSenderState(spoutName);

	if (state == ESpoutState::noEnoR || state == ESpoutState::noER) {
		UE_LOG(SpoutUELog, Display, TEXT("Creating and registering new Sender..."));
		CreateRegisterSender(spoutName, width, height, format);
		return false;
	}
	if (state == ESpoutState::EnoR) {
		UE_LOG(SpoutUELog, Warning, TEXT("A Sender with the name %s already exists"), *spoutName.GetPlainNameString());
		return false;
	}
	if (state == ESpoutState::ER) {
		GetSpoutRegistred(spoutName, SenderStruct);
		if (SenderStruct->spoutType == ESpoutType::Sender) {
			// Check whether texture size has changed
			if (width != SenderStruct->w || height != SenderStruct->h) {
				UE_LOG(SpoutUELog, Warning, TEXT("Texture Size has changed, Updating registered spout: "), *spoutName.GetPlainNameString());
				UpdateRegisteredSpout(spoutName, width, height, format);
				return false;
			}
		}
		if (SenderStruct->spoutType == ESpoutType::Receiver) {
			UE_LOG(SpoutUELog, Warning, TEXT("A Sender with the name %s already exists, and you have a receiver in Unreal that is receiving"), *spoutName.GetPlainNameString());
			
			return false;
		}	
	}
		
	if (SenderStruct->activeTextures == nullptr)
	{
		UE_LOG(SpoutUELog, Warning, TEXT("activeTextures is null"));
		return false;
	}

	HANDLE targetHandle = SenderStruct->sHandle;

	ID3D11Texture2D * targetTex = SenderStruct->activeTextures;

	if (targetTex == nullptr){
		UE_LOG(SpoutUELog, Warning, TEXT("targetTex is null"));
		return false;
	}
	FString fName = *spoutName.ToString();
	
	ENQUEUE_RENDER_COMMAND(void)(
		[targetTex, sentTexture, SenderStruct, fName](FRHICommandListImmediate& RHICmdList)
		{

			//D3D11_TEXTURE2D_DESC td;
			//baseTexture->GetDesc(&td);
			// Smode Tech frame counter based copy
			if (SenderStruct->frame && SenderStruct->frame->CheckTextureAccess(targetTex))
			{
          //g_pImmediateContext->CopyResource(targetTex, baseTexture);
          //g_pImmediateContext->Flush();
          //sender->UpdateSender(TCHAR_TO_ANSI(*fName), td.Width, td.Height, SenderStruct->sHandle, td.Format /** Smode Tech Fix Bad Texture Format */);

        unsigned int width, height;
        DXGI_FORMAT format = dx12Interop.getTextureFormat(sentTexture, width, height);
        dx12Interop.copyToDx11Texture(sentTexture, targetTex);
        sender->UpdateSender(TCHAR_TO_ANSI(*fName), width, height, SenderStruct->sHandle, format);

        SenderStruct->frame->AllowTextureAccess(targetTex);
        SenderStruct->frame->SetNewFrame();
			}
		});
	return true;
}

bool USpoutBPFunctionLibrary::SpoutReceiver(const FName spoutName, UMaterialInstanceDynamic*& mat, UTexture2D*& texture)
{
	const FString SenderNameString = spoutName.GetPlainNameString();

	if (BaseMaterial == NULL)
	{
		UMaterialInterface* mBase_Material = 0;
		mBase_Material = LoadObject<UMaterial>(NULL, TEXT("/SpoutPlugin/Materials/SpoutMaterial.SpoutMaterial"), NULL, LOAD_None, NULL);
		BaseMaterial = mBase_Material;
	}
	
	if (sender == nullptr)
	{ 
		initSpout();
	};

	if (g_D3D11Device == nullptr || g_pImmediateContext == NULL){
		
		GetDevice();
	}

	ESpoutState state = CheckSenderState(spoutName);

	if (state == ESpoutState::noEnoR) {
		UE_LOG(SpoutUELog, Warning, TEXT("No sender found registered with the name %s"), *spoutName.GetPlainNameString());
		UE_LOG(SpoutUELog, Warning, TEXT("Try to rename it, or resend %s"), *SenderNameString);
		return false;
	}
		
	if(state == ESpoutState::noER) {
		UE_LOG(SpoutUELog, Warning, TEXT("Why are you registered??, unregister, best to close it"));
		//UnregisterSpout(spoutName);
		CloseSender(spoutName);
		return false;
	}

	if (state == ESpoutState::EnoR) {
		UE_LOG(SpoutUELog, Warning, TEXT("Sender %s found, registering, receiving..."), *spoutName.GetPlainNameString());
		RegisterReceiver(spoutName);
		return false;
	}

	if (state == ESpoutState::ER) {
		FSenderStruct* SenderStruct = 0;
		GetSpoutRegistred(spoutName, SenderStruct);
		
		if (SenderStruct->spoutType == ESpoutType::Sender) {
			//UE_LOG(SpoutUELog, Warning, TEXT("Receiving from sender inside ue4 with the name %s"), *spoutName.GetPlainNameString());
		}

		if (SenderStruct->spoutType == ESpoutType::Receiver) {
			//UE_LOG(SpoutUELog, Warning, TEXT("Continue Receiver with the name %s"), *SenderName.GetPlainNameString());

			//communication between the two threads (rendering thread and game thread)
			// copy pixels from shared resource texture to texture temporal and update 
			int32 Stride = SenderStruct->w * 4;

			ENQUEUE_RENDER_COMMAND(void)(
				[SenderStruct, Stride, spoutName](FRHICommandListImmediate& RHICmdList)
				{
					ID3D11Texture2D* t_texTemp = SenderStruct->texTemp;
					ID3D11Texture2D* t_tex = (ID3D11Texture2D*)SenderStruct->sharedResource;

					if (SenderStruct == nullptr) {
						return;
					}
		
					g_pImmediateContext->CopyResource(t_texTemp, t_tex);
					//g_pImmediateContext->Flush(); //<------ No Flush
					
					D3D11_MAPPED_SUBRESOURCE  mapped;
					//Gets a pointer to the data contained in a subresource, and denies the GPU access to that subresource.
					// with CPU read permissions (D3D11_MAP_READ)
					HRESULT hr = g_pImmediateContext->Map(t_texTemp, 0, D3D11_MAP_READ, 0, &mapped);
					if (FAILED(hr))
					{
						t_texTemp->Release();
						return;
					}
					BYTE *pixel = (BYTE *)mapped.pData;
					g_pImmediateContext->Unmap(t_texTemp, 0);

#if ENGINE_MINOR_VERSION < 26 && ENGINE_MAJOR_VERSION ==4 /* Smode hardcoded downcast for old Unreal 4.25 */
					FTexture2DRHIRef texture2D = ((FTexture2DResource*)(SenderStruct->Texture2DResource))->GetTexture2DRHI();
#else
					FTexture2DRHIRef texture2D = SenderStruct->Texture2DResource->GetTexture2DRHI();
#endif
					//Update Texture
					if (texture2D)
					  RHIUpdateTexture2D(
						texture2D,
						0,
						*SenderStruct->UpdateRegions,
						mapped.RowPitch,
						(uint8*)pixel
					);
				});

			texture = SenderStruct->TextureColor;
			mat = SenderStruct->MaterialInstanceColor;
		}
	}
	
	return true;
}

bool USpoutBPFunctionLibrary::SpoutReceiverWithFallback(const FName spoutName, UMaterialInstanceDynamic* fallbackMat, UTexture2D* fallbackTexture, UMaterialInstanceDynamic*& mat, UTexture2D*& texture)
{
	const bool res = SpoutReceiver(spoutName, mat, texture);
	if (!res)
	{
		mat = fallbackMat;
		texture = fallbackTexture;
	}
	return res;
}

void USpoutBPFunctionLibrary::CloseSender(FName spoutName)
{

	UE_LOG(SpoutUELog, Display, TEXT("Closing... sender %s"), *spoutName.GetPlainNameString());

	if (sender == nullptr)
	{
		initSpout();

	};
	if (g_D3D11Device == nullptr || g_pImmediateContext == NULL) {

		GetDevice();
	}

	ESpoutState state = CheckSenderState(spoutName);

	if (state == ESpoutState::noEnoR) {
		UE_LOG(SpoutUELog, Warning, TEXT("%s is already closed; there is nothing to close!! Please check the name."), *spoutName.GetPlainNameString());
		//return;
	}
	if (state == ESpoutState::noER) {
		UE_LOG(SpoutUELog, Warning, TEXT("+++++++++++++++"));
		UnregisterSpout(spoutName);
		//return;
	}
	if (state == ESpoutState::EnoR) {
			UE_LOG(SpoutUELog, Warning, TEXT("A Sender with the name %s already exists. You can not close a sender that is not yours??"), *spoutName.GetPlainNameString());	
		//return;
	}
	if (state == ESpoutState::ER) {

		FSenderStruct* tempSenderStruct = 0;
		GetSpoutRegistred(spoutName, tempSenderStruct);
		
		if (tempSenderStruct->spoutType == ESpoutType::Sender) {
			UE_LOG(SpoutUELog, Display, TEXT("Releasing sender %s"), *spoutName.GetPlainNameString());
			// here really release the sender
			sender->ReleaseSenderName(TCHAR_TO_ANSI(*spoutName.ToString()));
			UE_LOG(SpoutUELog, Display, TEXT("Sender %s released"), *spoutName.GetPlainNameString());
			
			// Smode Tech frame counter cleanup
			if (tempSenderStruct->frame)
			{
				delete tempSenderStruct->frame;
				tempSenderStruct->frame = nullptr;
			}

		}
		else {
			UE_LOG(SpoutUELog, Warning, TEXT("Receiver always listening"));

			FlushRenderingCommands();
			if (tempSenderStruct->sharedResource != nullptr) {
				UE_LOG(SpoutUELog, Warning, TEXT("Release sharedResource"));
				tempSenderStruct->sharedResource->Release();
			}
			if (tempSenderStruct->texTemp != nullptr) {
				UE_LOG(SpoutUELog, Warning, TEXT("Release Temporal texTemp"));
				tempSenderStruct->texTemp->Release();
			}
			if (tempSenderStruct->rView != nullptr) {
				UE_LOG(SpoutUELog, Warning, TEXT("Release rView"));
				tempSenderStruct->rView->Release();
			}
			UE_LOG(SpoutUELog, Warning, TEXT("Released All Temporal Textures"));
			//return;
		}

		UnregisterSpout(spoutName);
	}

	UE_LOG(SpoutUELog, Display, TEXT("There are now %i senders remaining"), FSenders.Num());
	

}


bool USpoutBPFunctionLibrary::UpdateRegisteredSpout(FName spoutName, unsigned int width, unsigned int height, DXGI_FORMAT format)
{
	HANDLE sharedSendingHandle = NULL;
	bool texResult = false;
	bool updateResult = false;
	bool senderResult = false;

	ID3D11Texture2D * sendingTexture = nullptr /* SMode Tech Crash*/;

	UE_LOG(SpoutUELog, Display, TEXT("ID3D11Texture2D Info : ancho_%i, alto_%i"), width, height);
	UE_LOG(SpoutUELog, Display, TEXT("ID3D11Texture2D Info : Format is %i"), int(format));

	//use the pixel format from basetexture (the native texture textureRenderTarget2D)
	DXGI_FORMAT texFormat = format;
	if (format == DXGI_FORMAT_B8G8R8A8_TYPELESS) {
		texFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	}

	texResult = sdx->CreateSharedDX11Texture(g_D3D11Device, width, height, texFormat, &sendingTexture, sharedSendingHandle);
	UE_LOG(SpoutUELog, Display, TEXT("Create shared Texture with SDX : %i"), texResult);

	if (!texResult)
	{
		UE_LOG(SpoutUELog, Error, TEXT("SharedDX11Texture creation failed"));
		return 0;
	}

	// update register
	UE_LOG(SpoutUELog, Warning, TEXT("Updating Sender in Sender list"));

	for (int32 Index = 0; Index != FSenders.Num(); ++Index)
	{
		if (FSenders[Index].sName == spoutName) {
			FSenders[Index].SetW(width);
			FSenders[Index].SetH(height);
			FSenders[Index].SetName(spoutName);
			FSenders[Index].bIsAlive = true;
			FSenders[Index].spoutType = ESpoutType::Sender;
			FSenders[Index].SetHandle(sharedSendingHandle);
			FSenders[Index].activeTextures = sendingTexture;

			FSenders[Index].MaterialInstanceColor = nullptr;
			FSenders[Index].TextureColor = nullptr;

			senderResult = true;
		}
	}

	return senderResult;
}
