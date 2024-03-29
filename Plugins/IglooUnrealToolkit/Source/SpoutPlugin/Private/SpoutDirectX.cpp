//
//
//			spoutDirectX.cpp
//
//		DirectX functions to manage DirectX9 and DirectX11 texture sharing
//
// ====================================================================================
//		Revisions :
//
//		22.07.14	- added option for DX9 or DX11
//		21.09.14	- included keyed mutex texture access lock in CreateSharedDX11Texture
//		23.09.14	- moved general mutex texture access lock to this class
//		23.09.14	- added DX11available() to verify operating system support for DirectX 11
//		15.10.14	- added debugging aid for texture access locks
//		17.10.14	- flush before release immediate context in CloseDX11
//		21.10.14	- removed keyed mutex lock due to reported driver problems
//					  TODO - cleanup all functions using it
//		10.02.15	- removed functions relating to DirectX 11 keyed mutex lock
//		14.02.15	- added UNREFERENCED_PARAMETER(pSharedTexture) to CheckAceess and AllowAccess
//		29.05.15	- Included SetAdapter for multiple adapters - Franz Hildgen.
//		02.06.15	- Added GetAdapter, GetNumAdapters, GetAdapterName
//		08.06.15	- removed dx9 flag from setadapter
//		04.08.15	- cleanup
//		11.08.15	- removed GetAdapterName return if Intel. For use with Intel HD4400/5000 graphics
//		22.10.15	- removed DX11available and shifted it to the interop class
//		19.11.15	- fixed return value in CreateDX9device after caps error (was false instead of NULL)
//		20.11.15	- Registry read/write moved from SpoutGLDXinterop class
//		16.02.16	- IDXGIFactory release - from https://github.com/jossgray/Spout2
//		29.02.16	- cleanup
//		05.04.16	- removed unused texture pointer from mutex access functions
//		16.06.16	- fixed null device release in SetAdapter - https://github.com/leadedge/Spout2/issues/17
//		01.07.16	- restored hFocusWindow in CreateDX9device (was set to NULL for testing)
//		04.09.16	- Add create DX11 staging texture
//		16.01.17	- Add WriteDX9surface
//		23.01.17	- pEventQuery->Release() for writeDX9surface
//		02.06.17	- Registry functions moved to SpoutUtils
//					- Added Spout error log console output
//		17.03.18	- More error log notices
//					- protect against an adapter with no outputs for SetAdapter
//		16.06.18	- change all class variable name prefix from g_ to m_
//					  Add GetImmediateContext();
//					- Add ReleaseDX11Texture and ReleaseDX11Device
//		13.11.18	- Remove staging texture functons
//		16.12.18	- Move FlushWait from interop class
//		03.01.19	- Changed to revised registry functions in SpoutUtils
//		27.06.19	- Restored release of existing texture in CreateSharedDX11Texture
//		03.07.19	- Added pointer checks in OpenDX11shareHandle
//		18.09.19	- Changed initial log from notice to to verbose
//					  for CreateSharedDX9Texture and CreateSharedDX11Texture
//		08.11.19	- removed immediate context check from OpenDX11shareHandle
//		15.03.20	- allow for zero or DX9 format passed in to CreateSharedDX11Texture for utility use
//		16.06.20	- Create separate Wait function
//					- Update comments
//		02.09.20	- CreateSharedDX11Texture - test for null pointer to the shared texture pointer
//		03.09.20	- ReleaseDX11Texture 
//					    add log warnings for null device and texture
//					    add DirectX messages to Output window for debug build
//		06.09.20	- Add output test to SetAdapter
//		08.09.20	- Release all pointers in adapter functions
//					  Remove failures if no adapter output pending testing
//					  Add immediate context test before flush in ReleaseDX11Texture
//					  In case the function is used by a different device.
//		12.09.2020	- Re-introduced Optimus Enablement to enforce NVidia Optimus
//					  Incuding AMD Enduro technology
//					  Credit to https://github.com/Qlex42
//		13.09.20	- Remove Optimus enablement again due to use of extern "C"
//		15.09.20	- Changed all result !=S_OK and !=D3D_OK to FAILED macro for consistency
//					  Correct type cast in CreateDX9device and GetAdapterName
//		16.09.20	- ReleaseDX11Texture - removed log notice for no reference count
//		19.09.20	- Add success notice for SetAdapter
//					  Add DebugLog function
//					  Set passed pointer to null in ReleaseDX11Texture
//					  Clean up comments and logs throughtout
//		21.09.20	- Format specifiers for hex print
//					  SetAdapter - corrected logs
//		23.09.20	- Change warning logs to error in OpenDX11shareHandle
//		24.09.20	- Change all pointer "= NULL to "= nullptr"
//					  Change hex printf to 0x%8.8llX
//					- Introduce try/catch to OpenDX11shareHandle
//					  for the possibility of different graphics adapters
//					- Corrected compare of different enum types in CreateSharedDX11Texture
//		25.09.20	- Made GetAdapterPointer public
//					  Add SetAdapterPointer
//		08.10.20	- Re-introduced CreateDX11StagingTexture
//
// ====================================================================================
/*

	Copyright (c) 2014-2020. Lynn Jarvis. All rights reserved.

	Redistribution and use in source and binary forms, with or without modification,
	are permitted provided that the following conditions are met:

		1. Redistributions of source code must retain the above copyright notice,
		   this list of conditions and the following disclaimer.

		2. Redistributions in binary form must reproduce the above copyright notice,
		   this list of conditions and the following disclaimer in the documentation
		   and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"	AND ANY
	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE	ARE DISCLAIMED.
	IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "SpoutDirectX.h"

spoutDirectX::spoutDirectX() {

	// DX11
	m_pImmediateContext = nullptr;
	m_driverType = D3D_DRIVER_TYPE_NULL;
	m_featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Output graphics adapter
	// Programmer can set for an application
	m_AdapterIndex = 0; // DX9
	m_pAdapterDX11 = nullptr; // DX11
}

spoutDirectX::~spoutDirectX() {

}

//
// =========================== DX9 ================================
//

// Create a DX9 object
IDirect3D9Ex* spoutDirectX::CreateDX9object()
{
	IDirect3D9Ex* pD3D;

	HRESULT res = Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3D);
	if (FAILED(res)) return NULL;

	return pD3D;
}

// Create a DX9 device
IDirect3DDevice9Ex* spoutDirectX::CreateDX9device(IDirect3D9Ex* pD3D, HWND hWnd)
{
	IDirect3DDevice9Ex* pDevice;
	D3DPRESENT_PARAMETERS d3dpp;
	D3DCAPS9 d3dCaps;
	unsigned int AdapterIndex = static_cast<unsigned int>(m_AdapterIndex);

	SpoutLogNotice("spoutDirectX::CreateDX9device - adapter = %u, hWnd = 0x%8.8llX", AdapterIndex, (ULONGLONG)hWnd);

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = true;						// windowed and not full screen
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;	// discard old frames
	d3dpp.hDeviceWindow = hWnd;						// set the window to be used by D3D

	// D3DFMT_UNKNOWN can be specified for the BackBufferFormat while in windowed mode. 
	// This tells the runtime to use the current display-mode format and eliminates
	// the need to call GetDisplayMode. 
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	// Set a dummy resolution - we don't render anything
	d3dpp.BackBufferWidth = 1920;
	d3dpp.BackBufferHeight = 1080;
	d3dpp.EnableAutoDepthStencil = false;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.BackBufferCount = 1;

	// Test for hardware vertex processing capability and set up as needed
	// D3DCREATE_MULTITHREADED required by interop spec
	if (FAILED(pD3D->GetDeviceCaps(AdapterIndex, D3DDEVTYPE_HAL, &d3dCaps))) {
		SpoutLogFatal("spoutDirectX::CreateDX9device - GetDeviceCaps error");
		return NULL;
	}

	// | D3DCREATE_NOWINDOWCHANGES
	DWORD dwBehaviorFlags = D3DCREATE_PUREDEVICE | D3DCREATE_MULTITHREADED;
	if (d3dCaps.VertexProcessingCaps != 0)
		dwBehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		dwBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// Create a DirectX9 device - we use directx only for accessing the handle
	// Note : hwnd seems to have no effect - maybe because we do not render anything.
	// Note here that we are setting up for Windowed mode but it seems not to be affected
	// by fullscreen, probably because we are not rendering to it.
	HRESULT res = pD3D->CreateDeviceEx(AdapterIndex,   // D3DADAPTER_DEFAULT
		D3DDEVTYPE_HAL, // Hardware rasterization. 
		hWnd,			// hFocusWindow (can be NULL)
		dwBehaviorFlags,
		&d3dpp,			// d3dpp.hDeviceWindow should be valid if hFocusWindow is NULL
		NULL,			// pFullscreenDisplayMode must be NULL for windowed mode
		&pDevice);

	if (FAILED(res)) {
		SpoutLogFatal("spoutDirectX::CreateDX9device - CreateDeviceEx returned error %lu (0x%lX)", res, res);
		return NULL;
	}

	SpoutLogNotice("    device (0x%8.8llX)", (ULONGLONG)pDevice);

	return pDevice;

} // end CreateDX9device


// Create a shared DirectX9 texture
// by giving it a sharehandle variable - dxShareHandle
// For a SENDER : the sharehandle is NULL and a new texture is created
// For a RECEIVER : the sharehandle is valid and a handle to the existing shared texture is created
bool spoutDirectX::CreateSharedDX9Texture(IDirect3DDevice9Ex* pDevice, unsigned int width, unsigned int height, D3DFORMAT format, LPDIRECT3DTEXTURE9& dxTexture, HANDLE& dxShareHandle)
{
	if (!pDevice) {
		SpoutLogError("spoutDirectX::CreateSharedDX9Texture - NULL DX9 device");
		return false;
	}

	SpoutLogNotice("spoutDirectX::CreateSharedDX9Texture %dx%d - format %d - handle 0x%X", width, height, (int)format, (ULONGLONG)dxShareHandle);

	if (dxTexture) dxTexture->Release();

	HRESULT res = pDevice->CreateTexture(width,
		height,
		1,
		D3DUSAGE_RENDERTARGET,
		format, // default is D3DFMT_A8R8G8B8 - may be set externally
		D3DPOOL_DEFAULT, // Required by interop spec
		&dxTexture,
		&dxShareHandle);	// local share handle to allow type casting for 64bit

	// USAGE may also be D3DUSAGE_DYNAMIC and pay attention to format and resolution!!!
	// USAGE, format and size for sender and receiver must all match
	if (FAILED(res)) {
		char tmp[256];
		sprintf_s(tmp, 256, "spoutDirectX::CreateSharedDX9Texture error (0x%lX) - ", (LONG)res);
		switch (res) {
		case D3DERR_INVALIDCALL:
			strcat_s(tmp, 256, "D3DERR_INVALIDCALL");
			break;
		case D3DERR_OUTOFVIDEOMEMORY:
			strcat_s(tmp, 256, "D3DERR_OUTOFVIDEOMEMORY");
			break;
		case E_OUTOFMEMORY:
			strcat_s(tmp, 256, "E_OUTOFMEMORY");
			break;
		default:
			strcat_s(tmp, 256, "Unknown error");
			break;
		}
		SpoutLogFatal("%s", tmp);
		return false;
	}

	return true;

} // end CreateSharedDX9Texture


bool spoutDirectX::WriteDX9memory(IDirect3DDevice9Ex* pDevice, LPDIRECT3DSURFACE9 source_surface, LPDIRECT3DTEXTURE9 dxTexture)
{
	IDirect3DSurface9* texture_surface = nullptr;
	IDirect3DQuery9* pEventQuery = nullptr;
	HRESULT hr = 0;
	hr = dxTexture->GetSurfaceLevel(0, &texture_surface); // shared texture surface
	if (SUCCEEDED(hr)) {
		// UpdateSurface
		// https://msdn.microsoft.com/en-us/library/windows/desktop/bb205857%28v=vs.85%29.aspx
		//    The source surface must have been created with D3DPOOL_SYSTEMMEM.
		//    The destination surface must have been created with D3DPOOL_DEFAULT.
		//    Neither surface can be locked or holding an outstanding device context.
		hr = pDevice->UpdateSurface(source_surface, NULL, texture_surface, NULL);
		if (SUCCEEDED(hr)) {
			// It is necessary to flush the command queue 
			// or the data is not ready for the receiver to read.
			// Adapted from : https://msdn.microsoft.com/en-us/library/windows/desktop/bb172234%28v=vs.85%29.aspx
			// Also see : http://www.ogre3d.org/forums/viewtopic.php?f=5&t=50486
			pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);
			if (pEventQuery) {
				pEventQuery->Issue(D3DISSUE_END);
				while (S_FALSE == pEventQuery->GetData(NULL, 0, D3DGETDATA_FLUSH));
				pEventQuery->Release(); // Must be released or causes a leak and reference count increment
			}
			return true;
		}
	}

	SpoutLogError("spoutDirectX::WriteDX9surface((0x%8.8llX), (0x%8.8llX), (0x%8.8llX) failed", (ULONGLONG)pDevice, (ULONGLONG)dxTexture, (ULONGLONG)source_surface);

	return false;
} // end WriteDX9surface

//
// COPY FROM A GPU DX9 SURFACE TO THE SHARED DX9 TEXTURE
//
//    The source surface must have been created using the same device as the texture
//
bool spoutDirectX::WriteDX9surface(IDirect3DDevice9Ex* pDevice, LPDIRECT3DSURFACE9 surface, LPDIRECT3DTEXTURE9 dxTexture)
{
	IDirect3DSurface9* texture_surface = nullptr;
	HRESULT hr = dxTexture->GetSurfaceLevel(0, &texture_surface); // destination texture surface
	if (SUCCEEDED(hr)) {
		// StretchRect is a GPU copy
		hr = pDevice->StretchRect(surface, NULL, texture_surface, NULL, D3DTEXF_NONE);
		if (SUCCEEDED(hr)) {
			// It is necessary to flush the command queue
			// or the data is not ready for the receiver to read.
			// Adapted from : https://msdn.microsoft.com/en-us/library/windows/desktop/bb172234%28v=vs.85%29.aspx
			// Also see : http://www.ogre3d.org/forums/viewtopic.php?f=5&t=50486
			IDirect3DQuery9* pEventQuery = nullptr;
			pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);
			if (pEventQuery) {
				pEventQuery->Issue(D3DISSUE_END);
				while (S_FALSE == pEventQuery->GetData(NULL, 0, D3DGETDATA_FLUSH));
				pEventQuery->Release(); // Must be released or causes a leak and reference count increment
			}
			return true;
		}
	}

	SpoutLogError("spoutDirectX::WriteDX9surface(0x%8.8llX, 0x%8.8llX) failed", (ULONGLONG)surface, (ULONGLONG)dxTexture);

	return false;
}


// =========================== end DX9 =============================


//
// =========================== DX11 ================================
//

//
// Notes for DX11 : https://www.opengl.org/registry/specs/NV/DX_interop2.txt
//
// Valid device types for the <dxDevice> parameter of wglDXOpenDeviceNV and associated restrictions
// DirectX device type ID3D11Device - can only be used on WDDM operating systems; XXX Must be multithreaded
// TEXTURE_2D - ID3D11Texture2D - Usage flags must be D3D11_USAGE_DEFAULT
// wglDXSetResourceShareHandle does not need to be called for DirectX version
// 10 and 11 resources. Calling this function for DirectX 10 and 11 resources
// is not an error but has no effect.

// Create DX11 device
ID3D11Device* spoutDirectX::CreateDX11device()
{
	ID3D11Device* pd3dDevice = nullptr;
	HRESULT hr = S_OK;
	UINT createDeviceFlags = 0;
	IDXGIAdapter* pAdapterDX11 = m_pAdapterDX11;

	SpoutLogNotice("spoutDirectX::CreateDX11device - pAdapterDX11 (0x%8.8llX)", (ULONGLONG)m_pAdapterDX11);

	//
	// If the project is in a debug build, enable debugging via SDK Layers with this flag.
	// https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_create_device_flag
	// To use this flag, you must have D3D11_1SDKLayers.dll installed or device creation fails.
	// To resolve this you can install the Windows 10 SDK.
	// 
	// Due to this dependency problem, you have to manually remove the comments below to enable it.
	// See also : void spoutDirectX::DebugLog
	//

/*
#if defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
*/

// GL/DX interop Spec
// ID3D11Device can only be used on WDDM operating systems : Must be multithreaded
// D3D11_CREATE_DEVICE_FLAG createDeviceFlags
	D3D_DRIVER_TYPE driverTypes[] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};

	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	// These are the feature levels that we will accept.
	// m_featureLevel is the feature level used
	// 11.0 is the highest level currently supported for Spout
	// because 11.1 limits compatibility
	// Note from D3D11 Walbourn examples :
	//	DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1
	// Note from NVIDIA forums :
	//  Not all DirectX 11.1 features are software features.
	//  Target Independent Rasterization requires hardware support
	//  so we can not make DX11 GPUs fully DX11.1 complaint.
	D3D_FEATURE_LEVEL featureLevels[] = {
		// D3D_FEATURE_LEVEL_11_1, // 0xb001
		D3D_FEATURE_LEVEL_11_0, // 0xb000
		D3D_FEATURE_LEVEL_10_1, // 0xa100
		D3D_FEATURE_LEVEL_10_0, // 0xa000
	};

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	// To allow for multiple graphics cards we will use m_pAdapterDX11
	// Which is set by SetAdapter before initializing DirectX
	if (pAdapterDX11) {
		hr = D3D11CreateDevice(pAdapterDX11,
			D3D_DRIVER_TYPE_UNKNOWN,
			NULL,
			createDeviceFlags,
			featureLevels,
			numFeatureLevels,
			D3D11_SDK_VERSION,
			&pd3dDevice,
			&m_featureLevel,
			&m_pImmediateContext);

	} // endif adapter set
	else {

		// Possible Optimus problem : is the default adapter (NULL) always Intel ?
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476082%28v=vs.85%29.aspx
		// pAdapter : a pointer to the video adapter to use when creating a device. 
		// Pass NULL to use the default adapter, which is the first adapter that is
		// enumerated by IDXGIFactory1::EnumAdapters. 
		// http://www.gamedev.net/topic/645920-d3d11createdevice-returns-wrong-feature-level/

		for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++) {

			m_driverType = driverTypes[driverTypeIndex];

			hr = D3D11CreateDevice(NULL,
				m_driverType,
				NULL,
				createDeviceFlags,
				featureLevels,
				numFeatureLevels,
				D3D11_SDK_VERSION,
				&pd3dDevice,
				&m_featureLevel,
				&m_pImmediateContext);

			// Break as soon as something passes
			if (SUCCEEDED(hr))
				break;
		}

	} // endif no adapter set

	// Quit if nothing worked
	if (FAILED(hr)) {
		SpoutLogFatal("spoutDirectX::CreateDX11device NULL device");
		return NULL;
	}

	// All OK - return the device pointer to the caller
	SpoutLogNotice("    device (0x%8.8llX)", (ULONGLONG)pd3dDevice);

	return pd3dDevice;

} // end CreateDX11device


bool spoutDirectX::CreateSharedDX11Texture(ID3D11Device* pd3dDevice,
	unsigned int width,
	unsigned int height,
	DXGI_FORMAT format,
	ID3D11Texture2D** ppSharedTexture,
	HANDLE& dxShareHandle)
{
	if (!pd3dDevice) {
		SpoutLogFatal("spoutDirectX::CreateSharedDX11Texture NULL device");
		return false;
	}

	if (!ppSharedTexture) {
		SpoutLogWarning("spoutDirectX::CreateSharedDX11Texture NULL ppSharedTexture");
		return false;
	}
	SpoutLogNotice("spoutDirectX::CreateSharedDX11Texture");

	//
	// Create a new shared DX11 texture
	//

	// Release the texture if it already exists
	if (*ppSharedTexture) {
		ReleaseDX11Texture(pd3dDevice, *ppSharedTexture);
	}

	ID3D11Texture2D* pTexture = nullptr; // The new texture pointer

	// SpoutLogVerbose("spoutDirectX::CreateSharedDX11Texture");
	// SpoutLogVerbose("    pDevice = 0x%8.8llX, width = %d, height = %d, format = %d", (ULONGLONG)pd3dDevice, width, height, format);

	// Use the format passed in
	// If that is zero or DX9 format, use the default format
	DXGI_FORMAT texformat = DXGI_FORMAT_B8G8R8A8_UNORM;
	if (format != 0 && format != (DXGI_FORMAT)D3DFMT_A8R8G8B8)
		texformat = format;

	// Textures being shared from D3D9 to D3D11 have the following restrictions (LJ - D3D11 to D3D9 ?).
	//		Textures must be 2D
	//		Only 1 mip level is allowed
	//		Texture must have default usage
	//		Texture must be write only	- ?? LJ ??
	//		MSAA textures are not allowed
	//		Bind flags must have SHADER_RESOURCE and RENDER_TARGET set
	//		Only R10G10B10A2_UNORM, R16G16B16A16_FLOAT and R8G8B8A8_UNORM formats are allowed - ?? LJ ??
	//		If a shared texture is updated on one device ID3D11DeviceContext::Flush must be called on that device **

	// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476903%28v=vs.85%29.aspx
	// To share a resource between two Direct3D 11 devices the resource must have been created
	// with the D3D11_RESOURCE_MISC_SHARED flag, if it was created using the ID3D11Device interface.
	//
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED; // This texture will be shared
	// desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
	// Note that a DirectX 11 texture with D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX is not
	// compatible with DirectX 9 so a general named mutex is used for all texture types
	desc.CPUAccessFlags = 0;
	desc.Format = texformat;
	desc.Usage = D3D11_USAGE_DEFAULT;
	// Multisampling quality and count
	// The default sampler mode, with no anti-aliasing, has a count of 1 and a quality level of 0.
	desc.SampleDesc.Quality = 0;
	desc.SampleDesc.Count = 1;
	desc.MipLevels = 1;
	desc.ArraySize = 1;

	HRESULT res = pd3dDevice->CreateTexture2D(&desc, NULL, &pTexture);

	if (FAILED(res)) {
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476174%28v=vs.85%29.aspx
		char tmp[256];
		// TODO : check for compiler warning with "l" prefix
		sprintf_s(tmp, 256, "spoutDirectX::CreateSharedDX11Texture ERROR - [0x%lX] : ", res);
		switch (res) {
		case D3DERR_INVALIDCALL:
			strcat_s(tmp, 256, "D3DERR_INVALIDCALL");
			break;
		case E_INVALIDARG:
			strcat_s(tmp, 256, "E_INVALIDARG");
			break;
		case E_OUTOFMEMORY:
			strcat_s(tmp, 256, "E_OUTOFMEMORY");
			break;
		default:
			strcat_s(tmp, 256, "Unlisted error");
			break;
		}
		SpoutLogFatal("%s", tmp);
		return false;
	}

	// The DX11 texture is created OK
	// Get the texture share handle so it can be saved in shared memory for receivers to pick up.
	// When sharing a resource between two Direct3D 10/11 devices the unique handle 
	// of the resource can be obtained by querying the resource for the IDXGIResource 
	// interface and then calling GetSharedHandle.
	IDXGIResource* pOtherResource(NULL);
	if (!pTexture /* SMode Tech */ || FAILED(pTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&pOtherResource))) {
		SpoutLogFatal("spoutDirectX::CreateSharedDX11Texture - QueryInterface error");
		return false;
	}

	// Return the shared texture handle
	pOtherResource->GetSharedHandle(&dxShareHandle);
	pOtherResource->Release();

	*ppSharedTexture = pTexture;

	SpoutLogNotice("    pTexture = 0x%8.8llX : dxShareHandle = 0x%X", (ULONGLONG)pTexture, (ULONGLONG)dxShareHandle);

	return true;

}


// Create a DirectX 11 staging texture for read and write
bool spoutDirectX::CreateDX11StagingTexture(ID3D11Device* pd3dDevice,
	unsigned int width, unsigned int height, DXGI_FORMAT format, ID3D11Texture2D** ppStagingTexture)
{
	if (pd3dDevice == NULL) return false;

	SpoutLogNotice("spoutDirectX::CreateDX11StagingTexture");

	// Release the texture if it already exists
	if (*ppStagingTexture) {
		ReleaseDX11Texture(pd3dDevice, *ppStagingTexture);
	}

	ID3D11Texture2D* pTexture = nullptr; // The new texture pointer

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;

	HRESULT res = pd3dDevice->CreateTexture2D(&desc, NULL, &pTexture);

	if (res != S_OK) {
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476174%28v=vs.85%29.aspx
		char tmp[256];
		sprintf_s(tmp, 256, "spoutDirectX::CreateDX11StagingTexture ERROR : [0x%lx] : ", res);
		switch (res) {
		case D3DERR_INVALIDCALL:
			strcat_s(tmp, 256, "D3DERR_INVALIDCALL");
			break;
		case E_INVALIDARG:
			strcat_s(tmp, 256, "E_INVALIDARG");
			break;
		case E_OUTOFMEMORY:
			strcat_s(tmp, 256, "E_OUTOFMEMORY");
			break;
		default:
			strcat_s(tmp, 256, "Unlisted error");
			break;
		}
		SpoutLogFatal("%s", tmp);
		return false;
	}

	*ppStagingTexture = pTexture;

	SpoutLogNotice("    pTexture = 0x%8.8llX", (ULONGLONG)pTexture);

	return true;

}


bool spoutDirectX::OpenDX11shareHandle(ID3D11Device* pDevice, ID3D11Texture2D** ppSharedTexture, HANDLE dxShareHandle)
{

	if (!pDevice || !ppSharedTexture || !dxShareHandle) {
		SpoutLogError("spoutDirectX::OpenDX11shareHandle - null sources");
		return false;
	}

	// printf("OpenDX11shareHandle - pDevice [0x%llX] 0x%X, 0x%llX\n", (ULONGLONG)pDevice, (UINT)dxShareHandle, (ULONGLONG)ppSharedTexture);

	// To share a resource between a Direct3D 9 device and a Direct3D 11 device 
	// the texture must have been created using the pSharedHandle argument of CreateTexture.
	// The shared Direct3D 9 handle is then passed to OpenSharedResource in the hResource argument.
	//
	// Note that the resource created for use on this device must be eventually freed or there is a leak.
	//
	// This can crash if the share handle has been created using a different graphics adapter
	//try {
		HRESULT hr = pDevice->OpenSharedResource(dxShareHandle, __uuidof(ID3D11Resource), (void**)(ppSharedTexture));
		if (FAILED(hr)) {
			SpoutLogError("spoutDirectX::OpenDX11shareHandle failed");
			return false;
		}
	//}
	//catch (...) {
	//	// Catch any exception
	//	SpoutLogError("spoutDirectX::OpenDX11shareHandle - exception opening share handle");
	//	return false;
	//}


	/*
	// Can get sender format here
	ID3D11Texture2D * texturePointer = *ppSharedTexture;
	D3D11_TEXTURE2D_DESC td;
	texturePointer->GetDesc(&td);
	printf("td.Format = %d\n", td.Format); // 87
	printf("td.Width = %d\n", td.Width);
	printf("td.Height = %d\n", td.Height);
	printf("td.MipLevels = %d\n", td.MipLevels);
	printf("td.Usage = %d\n", td.Usage);
	printf("td.ArraySize = %d\n", td.ArraySize);
	printf("td.SampleDesc = %d\n", td.SampleDesc);
	printf("td.BindFlags = %d\n", td.BindFlags);
	printf("td.MiscFlags = %d\n", td.MiscFlags); // D3D11_RESOURCE_MISC_SHARED
	*/


	return true;

}

// Get context
ID3D11DeviceContext* spoutDirectX::GetImmediateContext()
{
	return m_pImmediateContext;
}

// Set required graphics adapter for output
bool spoutDirectX::SetAdapter(int index)
{
	char adaptername[128];
	IDXGIAdapter* pAdapter = nullptr;

	// Reset to default
	if (index == -1) {
		index = 0;
	}

	// Is the requested adapter available
	int n = GetNumAdapters();
	if (index > n - 1) {
		SpoutLogError("spoutDirectX::SetAdapter - index %d greater than number of adapters %d", index, n);
		return false;
	}

	// Must be able to get the name
	if (!GetAdapterName(index, adaptername, 128)) {
		SpoutLogError("spoutDirectX::SetAdapter - could not get name for adapter %d", index);
		return false;
	}

	SpoutLogNotice("spoutDirectX::SetAdapter(%d) [%s]", index, adaptername);

	// Get the global adapter pointer for DX11 CreateDevice to use
	if (m_pAdapterDX11) m_pAdapterDX11->Release();
	pAdapter = GetAdapterPointer(index);
	if (!pAdapter) {
		SpoutLogError("spoutDirectX::SetAdapter - could not get pointer for adapter %d", index);
		return false;
	}

	/*
	// Removed pending testing
	// Test for an output
	IDXGIOutput* p_output = nullptr;
	if (pAdapter->EnumOutputs(0, &p_output) == DXGI_ERROR_NOT_FOUND) {
		SpoutLogError("spoutDirectX::SetAdapter(%d) :  No output", index);
		pAdapter->Release();
		return false;
	}
	p_output->Release();
	*/

	// Set the global adapter index (used for DX9 and to retrieve the index)
	m_AdapterIndex = index;

	// Set the adapter pointer for DX11
	m_pAdapterDX11 = pAdapter;

	// In case of remaining incompatibility with the selected adapter, test everything here

	// For >= 2.005 check the directX sharing mode
	SpoutLogNotice("    creating test device");
	DWORD dwDX9 = 0;
	ReadDwordFromRegistry(HKEY_CURRENT_USER, "Software\\Leading Edge\\Spout", "DX9", &dwDX9);
	if (dwDX9 == 1) {
		// Try to create a DX9 object and device
		IDirect3D9Ex* pD3D = CreateDX9object();
		if (!pD3D) {
			SpoutLogError("spoutDirectX::SetAdapter - could not create DX9 object for adapter %d", index);
			pAdapter->Release();
			// Reset to default adapter
			m_AdapterIndex = 0; // DX9
			m_pAdapterDX11 = nullptr; // DX11
			return false;
		}
		IDirect3DDevice9Ex* pDevice = CreateDX9device(pD3D, NULL);
		if (!pDevice) {
			SpoutLogError("spoutDirectX::SetAdapter - could not create DX9 device for adapter %d", index);
			pD3D->Release();
			pAdapter->Release();
			m_AdapterIndex = 0;
			m_pAdapterDX11 = nullptr;
			return false;
		}
		// DX9 OK
		pD3D->Release();
		pDevice->Release();
	}
	else {
		// Try to create a DirectX 11 device
		ID3D11Device* pd3dDevice = CreateDX11device();
		if (!pd3dDevice) {
			SpoutLogError("spoutDirectX::SetAdapter - could not create DX11 device for adapter %d", index);
			pAdapter->Release();
			m_AdapterIndex = 0;
			m_pAdapterDX11 = nullptr;
			return false;
		}
		// Close the device because this is just a test
		// See : https://github.com/leadedge/Spout2/issues/17
		ReleaseDX11Device(pd3dDevice);
	}

	// Selected adapter OK
	SpoutLogNotice("    successfully set adapter %d", index);

	return true;

}

// Get the global adapter index
int spoutDirectX::GetAdapter()
{
	return m_AdapterIndex;
}


// FOR DEBUGGING 
bool spoutDirectX::FindNVIDIA(int& nAdapter)
{
	IDXGIFactory1* _dxgi_factory1;
	IDXGIAdapter* adapter1_ptr = nullptr;
	DXGI_ADAPTER_DESC desc;
	UINT32 i;
	bool bFound = false;

	if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&_dxgi_factory1)))
		return false;

	for (i = 0; _dxgi_factory1->EnumAdapters(i, &adapter1_ptr) != DXGI_ERROR_NOT_FOUND; i++) {
		adapter1_ptr->GetDesc(&desc);
		// printf("spoutDirectX::FindNVIDIA - Adapter(%d) : %S\n", i, desc.Description );
		DXGI_OUTPUT_DESC desc_out;
		IDXGIOutput* p_output = nullptr;
		if (adapter1_ptr->EnumOutputs(0, &p_output) == DXGI_ERROR_NOT_FOUND) {
			// printf("  No outputs\n");
			continue;
		}

		for (UINT32 j = 0; adapter1_ptr->EnumOutputs(j, &p_output) != DXGI_ERROR_NOT_FOUND; j++) {
			if (p_output) {
				p_output->GetDesc(&desc_out);
				// printf( "  Output : %d\n", j );
				// printf( "    Name %S\n", desc_out.DeviceName );
				// printf( "    Attached to desktop : (%d) %s\n", desc_out.AttachedToDesktop, desc_out.AttachedToDesktop ? "yes" : "no" );
				// printf( "    Rotation : %d\n", desc_out.Rotation );
				// printf( "    Left     : %d\n", desc_out.DesktopCoordinates.left );
				// printf( "    Top      : %d\n", desc_out.DesktopCoordinates.top );
				// printf( "    Right    : %d\n", desc_out.DesktopCoordinates.right );
				// printf( "    Bottom   : %d\n", desc_out.DesktopCoordinates.bottom );
				p_output->Release();
			}
		}
		adapter1_ptr->Release();

		if (wcsstr(desc.Description, L"NVIDIA")) {
			// printf("spoutDirectX::FindNVIDIA - Found NVIDIA adapter %d (%S)\n", i, desc.Description);
			bFound = true;
			break;
		}

	}

	_dxgi_factory1->Release();

	if (bFound) {
		// printf// ("spoutDirectX::FindNVIDIA - Found NVIDIA adapter %d (%S)\n", i, desc.Description);
		nAdapter = static_cast<int>(i);
		//	0x10DE	NVIDIA
		//	0x163C	intel
		//	0x8086  Intel
		//	0x8087  Intel
		// printf("Vendor    = %d [0x%X]\n", desc.VendorId, desc.VendorId);
		// printf("Revision  = %d [0x%X]\n", desc.Revision, desc.Revision);
		// printf("Device ID = %d [0x%X]\n", desc.DeviceId, desc.DeviceId);
		// printf("SubSys ID = %d [0x%X]\n", desc.SubSysId, desc.SubSysId);
		return true;
	}

	return false;

}


// Get the number of graphics adapters in the system
int spoutDirectX::GetNumAdapters()
{
	IDXGIFactory1* _dxgi_factory1;
	IDXGIAdapter* adapter1_ptr = nullptr;
	UINT32 i;

	// Enum Adapters first : multiple video cards
	if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&_dxgi_factory1))) {
		SpoutLogError("spoutDirectX::GetNumAdapters - No adapters found");
		return 0;
	}

	for (i = 0; _dxgi_factory1->EnumAdapters(i, &adapter1_ptr) != DXGI_ERROR_NOT_FOUND; i++) {

		DXGI_ADAPTER_DESC desc;
		adapter1_ptr->GetDesc(&desc);
		// printf("Adapter(%d) : %S\n", i, desc.Description );
		// printf("  Vendor Id : %d\n", desc.VendorId );
		// printf("  Dedicated System Memory : %.0f MiB\n", (float)desc.DedicatedSystemMemory / (1024.f * 1024.f) );
		// printf("  Dedicated Video Memory : %.0f MiB\n", (float)desc.DedicatedVideoMemory / (1024.f * 1024.f) );
		// printf("  Shared System Memory : %.0f MiB\n", (float)desc.SharedSystemMemory / (1024.f * 1024.f) );

		IDXGIOutput* p_output = nullptr;
		// 24-10-18 change from error to warning
		// 
		if (adapter1_ptr->EnumOutputs(0, &p_output) == DXGI_ERROR_NOT_FOUND) {
			// SpoutLogWarning("spoutDirectX::GetNumAdapters Adapter(%d) :  No outputs", i);
		}

		DXGI_OUTPUT_DESC desc_out;
		for (UINT32 j = 0; adapter1_ptr->EnumOutputs(j, &p_output) != DXGI_ERROR_NOT_FOUND; j++) {
			if (p_output) {
				p_output->GetDesc(&desc_out);
				// printf("  Output : %d\n", j );
				// printf("    Name %S\n", desc_out.DeviceName );
				// printf("    Attached to desktop : (%d) %s\n", desc_out.AttachedToDesktop, desc_out.AttachedToDesktop ? "yes" : "no" );
				// printf("    Rotation : %d\n", desc_out.Rotation );
				// printf("    Left     : %d\n", desc_out.DesktopCoordinates.left );
				// printf("    Top      : %d\n", desc_out.DesktopCoordinates.top );
				// printf("    Right    : %d\n", desc_out.DesktopCoordinates.right );
				// printf("    Bottom   : %d\n", desc_out.DesktopCoordinates.bottom );
				p_output->Release();
			}
		}
		adapter1_ptr->Release();
	}

	_dxgi_factory1->Release();

	return (int)i;

}

// Get an adapter name
bool spoutDirectX::GetAdapterName(int index, char* adaptername, int maxchars)
{
	IDXGIFactory1* _dxgi_factory1;
	IDXGIAdapter* adapter1_ptr = nullptr;

	if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&_dxgi_factory1))) {
		SpoutLogError("spoutDirectX::GetAdapterName - Could not create CreateDXGIFactory1");
		return false;
	}

	for (UINT32 i = 0; _dxgi_factory1->EnumAdapters(i, &adapter1_ptr) != DXGI_ERROR_NOT_FOUND; i++) {
		if ((int)i == index) {
			DXGI_ADAPTER_DESC desc;
			adapter1_ptr->GetDesc(&desc);
			size_t charsConverted = 0;
			size_t maxBytes = static_cast<size_t>(maxchars);
			wcstombs_s(&charsConverted, adaptername, maxBytes, desc.Description, maxBytes - 1);
			adapter1_ptr->Release();
			_dxgi_factory1->Release();
			return true;
		}
		adapter1_ptr->Release();
	}

	_dxgi_factory1->Release();

	return false;
}

IDXGIAdapter* spoutDirectX::GetAdapterPointer(int index)
{
	// Return the current pointer for default
	if (index < 0)
		return m_pAdapterDX11;

	// Enum Adapters first : multiple video cards
	IDXGIFactory1* _dxgi_factory1;
	if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&_dxgi_factory1))) {
		SpoutLogError("spoutDirectX::GetAdapterPointer - Could not create CreateDXGIFactory1");
		return nullptr;
	}

	IDXGIAdapter* adapter1_ptr = nullptr;
	for (UINT32 i = 0; _dxgi_factory1->EnumAdapters(i, &adapter1_ptr) != DXGI_ERROR_NOT_FOUND; i++) {
		if (index == (int)i) {

			/*
			// Removed pending testing
			// Now we have the requested adapter (17-03-18) test for an output on the adapter
			IDXGIOutput* p_output = nullptr;
			if (adapter1_ptr->EnumOutputs(0, &p_output) == DXGI_ERROR_NOT_FOUND) {
				SpoutLogError("spoutDirectX::GetAdapterPointer(%d) :  No outputs", i);
				adapter1_ptr->Release();
				_dxgi_factory1->Release();
				return nullptr;
			}
			*/

			_dxgi_factory1->Release();
			return adapter1_ptr;
		}
		adapter1_ptr->Release();
	}
	_dxgi_factory1->Release();

	return nullptr;
}

bool spoutDirectX::GetAdapterInfo(char* adapter, char* display, int maxchars)
{
	IDXGIFactory1* _dxgi_factory1;
	IDXGIAdapter* adapter1_ptr = nullptr;
	UINT32 i;
	size_t charsConverted = 0;
	size_t maxBytes = static_cast<size_t>(maxchars);

	// Enum Adapters first : multiple video cards
	if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&_dxgi_factory1))) {
		SpoutLogError("spoutDirectX::GetAdapterInfo - Could not create CreateDXGIFactory1");
		return false;
	}

	DXGI_ADAPTER_DESC desc;
	for (i = 0; _dxgi_factory1->EnumAdapters(i, &adapter1_ptr) != DXGI_ERROR_NOT_FOUND; i++) {
		adapter1_ptr->GetDesc(&desc);
		// Return the current adapter - max of 2 assumed
		wcstombs_s(&charsConverted, adapter, maxBytes, desc.Description, maxBytes - 1);
		IDXGIOutput* p_output = nullptr;
		for (UINT32 j = 0; adapter1_ptr->EnumOutputs(j, &p_output) != DXGI_ERROR_NOT_FOUND; j++) {
			DXGI_OUTPUT_DESC desc_out;
			if (p_output) {
				p_output->GetDesc(&desc_out);
				if (desc_out.AttachedToDesktop)
					wcstombs_s(&charsConverted, display, maxBytes, desc.Description, maxBytes - 1);
				p_output->Release();
			}
		}
	}
	_dxgi_factory1->Release();
	return true;
}

void spoutDirectX::SetAdapterPointer(IDXGIAdapter* pAdapter)
{
	m_pAdapterDX11 = pAdapter;
}

unsigned long spoutDirectX::ReleaseDX11Texture(ID3D11Device* pd3dDevice, ID3D11Texture2D* pTexture)
{

	if (!pd3dDevice || !pTexture) {
		if (!pd3dDevice)
			SpoutLogWarning("spoutDirectX::ReleaseDX11Texture - no device");
		if (!pTexture)
			SpoutLogWarning("spoutDirectX::ReleaseDX11Texture - no texture");
		return 0;
	}

	SpoutLogNotice("spoutDirectX::ReleaseDX11Texture (0x%8.8llX)", (ULONGLONG)pd3dDevice);

	unsigned long refcount = pTexture->Release();
	pTexture = nullptr;

	// Flush to put the release at the top of the queue
	if (m_pImmediateContext)
		m_pImmediateContext->Flush();

	// The device will be live, so warn if refcount > 1
	if (refcount > 1) {
		SpoutLogWarning("spoutDirectX::ReleaseDX11Texture - refcount = %lu", refcount);
		DebugLog(pd3dDevice, "spoutDirectX::ReleaseDX11Texture - refcount = %lu\n", refcount);
	}

	// Note that if the texture is registered and linked to OpenGL using the 
	// GL/DX interop, the interop must be unregistered or the texture is not
	// released even though the reference count reported here does not increase.

	return refcount;
}


unsigned long spoutDirectX::ReleaseDX11Device(ID3D11Device* pd3dDevice)
{
	if (!pd3dDevice)
		return 0;

	SpoutLogNotice("spoutDirectX::ReleaseDX11Device (0x%8.8llX)", (ULONGLONG)pd3dDevice);


	// Release the global context or there is an outstanding ref count
	// when the device is released
	if (m_pImmediateContext) {
		// Clear state and flush context to prevent deferred device release
		// From Microsoft docs :
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476425(v=vs.85).aspx
		// Microsoft Direct3D 11 defers the destruction of objects. 
		// Therefore, an application can't rely upon objects immediately being destroyed.
		// By calling Flush, you destroy any objects whose destruction was deferred.
		// If an application requires synchronous destruction of an object, we recommend
		// that the application release all its references, call 
		// ID3D11DeviceContext::ClearState, and then call Flush.
		m_pImmediateContext->ClearState();
		m_pImmediateContext->Flush();
		m_pImmediateContext->Release();
		m_pImmediateContext = nullptr;
	}

	// TODO : Release adapter pointer if there is one
	// if (m_pAdapterDX11)
		// m_pAdapterDX11->Release();

	unsigned long refcount = pd3dDevice->Release();
	pd3dDevice = nullptr;

	if (refcount > 0) {
		SpoutLogWarning("spoutDirectX::ReleaseDX11Device - refcount = %lu", refcount);
		DebugLog(pd3dDevice, "spoutDirectX::ReleaseDX11Device - refcount = %lu\n", refcount);
	}

	return refcount;
}

// 
// For a DX11 sender :
//   If a shared texture is updated on one device ID3D11DeviceContext::Flush must be called on that device. 
//   https://docs.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-opensharedresource
//   Only the sender updates the shared texture. It is not required for the receiver.
//   The application can either call Flush alone or combine a flush and Wait using this function.
//
// For an OpenGL sender :
//   This function is not necessary, the GL/DX interop performs the necessary flushing.
//
void spoutDirectX::FlushWait(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pImmediateContext)
{
	if (!pd3dDevice || !pImmediateContext)
		return;

	// (Approx 250 microseconds 0.25 msec)
	pImmediateContext->Flush();

	// CopyResource and Flush are both asynchronous.
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb205132%28v=vs.85%29.aspx#Performance_Considerations
	// Here we can wait for the copy and flush to finish before accessing the texture
	// (Approx 550 microseconds 0.55 msec)
	// Practical testing recommended
	Wait(pd3dDevice, pImmediateContext);
}


void spoutDirectX::Wait(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pImmediateContext)
{
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476578%28v=vs.85%29.aspx
	// When the GPU is finished, ID3D11DeviceContext::GetData will return S_OK.
	// When using this type of query, ID3D11DeviceContext::Begin is disabled.
	D3D11_QUERY_DESC queryDesc;
	ID3D11Query* pQuery = nullptr;
	ZeroMemory(&queryDesc, sizeof(queryDesc));
	queryDesc.Query = D3D11_QUERY_EVENT;
	pd3dDevice->CreateQuery(&queryDesc, &pQuery);
	if (pQuery) {
		pImmediateContext->End(pQuery);
		while (S_OK != pImmediateContext->GetData(pQuery, NULL, 0, 0));
		pQuery->Release();
	}
}


void spoutDirectX::DebugLog(ID3D11Device* pd3dDevice, const char* format, ...)
{

	char dlog[128];
	va_list args;

	// Construct the log
	va_start(args, format);
	vsprintf_s(dlog, 128, format, args);
	va_end(args);

	OutputDebugStringA("\n");
	OutputDebugStringA(dlog);

	//
	// Output for debug build
	// See comments in : ID3D11Device* spoutDirectX::CreateDX11device()
	//
/*
#ifdef _DEBUG

	if (!pd3dDevice)
		return;

	ID3D11Debug* DebugDevice = nullptr;
	if (pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&DebugDevice) == S_OK) {
		ID3D11InfoQueue *d3dInfoQueue = nullptr;
		if (SUCCEEDED(DebugDevice->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue))) {
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
			D3D11_MESSAGE_ID hide[] =
			{
				D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
				// Add more message IDs here as needed
			};

			D3D11_INFO_QUEUE_FILTER filter;
			memset(&filter, 0, sizeof(filter));
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
		}

		// Print live objects to the debug Output window
		DebugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);

		DebugDevice->Release();
	}
#else
*/

	UNREFERENCED_PARAMETER(pd3dDevice);

	// #endif

}
