//-----------------------------------------------------------------------------
// File: D3DTextr.cpp
//
// Desc: This file contains the member functions for the
//       CD3DTextureEngine class. The class is responsible for loading and
//       creating textures from files, as well as retrieving the
//       corresponding ptrs and surfaces for the textures, given a
//       texture's name.
//
//
// Copyright (c) 1996-1998 Microsoft Corporation. All rights reserved
//-----------------------------------------------------------------------------

#include <tchar.h>
#include <stdio.h>
#include "D3DTextr.h"
#include "D3DUtil.h"



//-----------------------------------------------------------------------------
// Name: TextureContainer
// Desc: Linked list tructure to hold info per texture
//-----------------------------------------------------------------------------
struct TextureContainer
{
    HBITMAP              hbmBitmap;      // Bitmap containing texture image
    LPDIRECTDRAWSURFACE4 pddsSurface;    // Surface of the texture
    LPDIRECT3DTEXTURE2   ptexTexture;    // Direct3D texture for the texture
    DWORD                dwStage;        // Texture stage (for multi-textures)
    TCHAR                strName[80];    // Filename of assoc'd bitmap
	BOOL                 bHasAlpha;
	DWORD                dwFlags;
	
	BYTE*                pSurfaceBits[10];
	DDSURFACEDESC2       ddsd;
    
	TextureContainer     *pPrev, *pNext; // Ptr to next texture in linked list

    ~TextureContainer();
};




//-----------------------------------------------------------------------------
// Macros, function prototypes and static variable
//-----------------------------------------------------------------------------
#define FOREACHTEXTURE(ptc) for( TextureContainer* ptc=g_ptcTextureList; \
                            NULL!=ptc; ptc=ptc->pNext )
inline BOOL FileExists( TCHAR* n )
{ FILE* fp=_tfopen(n,TEXT("rb")); return fp?(0==fclose(fp)):FALSE; }

static TextureContainer* g_ptcTextureList = NULL;     // Textures list
static TCHAR  g_strTexturePath[512] = TEXT("MEDIA\\"); // Path for files
static TCHAR* g_strRegValueName     = TEXT("DX6SDK Samples Path");




struct TEXTURESEARCHINFO
{
	DWORD dwDesiredBPP; // Input for texture format search
	BOOL  bUseAlpha;
	BOOL  bUsePalette;
	BOOL  bUseFourCC;
	BOOL  bFoundGoodFormat;

	DDPIXELFORMAT* pddpf; // Result of texture format search
};




//-----------------------------------------------------------------------------
// Name: TextureSearchCallback()
// Desc: Enumeration callback routine to find a best-matching texture format. 
//       The param data is the DDPIXELFORMAT of the best-so-far matching
//       texture. Note: the desired BPP is passed in the dwSize field, and the
//       default BPP is passed in the dwFlags field.
//-----------------------------------------------------------------------------
static HRESULT CALLBACK TextureSearchCallback( DDPIXELFORMAT* pddpf,
											   VOID* param )
{
    if( NULL==pddpf || NULL==param )
        return DDENUMRET_OK;

	TEXTURESEARCHINFO* ptsi = (TEXTURESEARCHINFO*)param;

    // Skip any funky modes
    if( pddpf->dwFlags & (DDPF_LUMINANCE|DDPF_BUMPLUMINANCE|DDPF_BUMPDUDV) )
        return DDENUMRET_OK;

	// Check for palettized formats
	if( ptsi->bUsePalette )
	{
		if( !( pddpf->dwFlags & DDPF_PALETTEINDEXED8 ) )
			return DDENUMRET_OK;

		// Accept the first 8-bit palettized format we get
        memcpy( ptsi->pddpf, pddpf, sizeof(DDPIXELFORMAT) );
		ptsi->bFoundGoodFormat = TRUE;
        return DDENUMRET_CANCEL;
    }

	// Else, skip any paletized formats (all modes under 16bpp)
	if( pddpf->dwRGBBitCount < 16 )
		return DDENUMRET_OK;

	// Check for FourCC formats
    if( ptsi->bUseFourCC )
	{
		if( pddpf->dwFourCC == 0 )
		    return DDENUMRET_OK;

		return DDENUMRET_CANCEL;
	}

	// Else, skip any FourCC formats
	if( pddpf->dwFourCC != 0 )
		return DDENUMRET_OK;

	// Make sure current alpha format agrees with requested format type
	if( (ptsi->bUseAlpha==TRUE) && !(pddpf->dwFlags&DDPF_ALPHAPIXELS) )
		return DDENUMRET_OK;
	if( (ptsi->bUseAlpha==FALSE) && (pddpf->dwFlags&DDPF_ALPHAPIXELS) )
		return DDENUMRET_OK;

    // Check if we found a good match
    if( pddpf->dwRGBBitCount == ptsi->dwDesiredBPP )
    {
        memcpy( ptsi->pddpf, pddpf, sizeof(DDPIXELFORMAT) );
		ptsi->bFoundGoodFormat = TRUE;
        return DDENUMRET_CANCEL;
    }

    return DDENUMRET_OK;
}




//-----------------------------------------------------------------------------
// Name: ~TextureContainer()
// Desc: Destructs the contents of the texture container
//-----------------------------------------------------------------------------
TextureContainer::~TextureContainer()
{
    SAFE_DELETE( pNext );
    SAFE_RELEASE( ptexTexture );
    SAFE_RELEASE( pddsSurface );
    DeleteObject( hbmBitmap );
}




//-----------------------------------------------------------------------------
// Name: CD3DStaticTextureEngine
// Desc: Class used to automatically construct and destruct the static
//       texture engine class.
//-----------------------------------------------------------------------------
class CD3DTextureEngine
{
public:
    CD3DTextureEngine();
    ~CD3DTextureEngine();
} g_StaticTextureEngine;




//-----------------------------------------------------------------------------
// Name: CD3DTextureEngine()
// Desc: Constructs the texture engine. Creates a DDraw object
//-----------------------------------------------------------------------------
CD3DTextureEngine::CD3DTextureEngine()
{
}




//-----------------------------------------------------------------------------
// Name: ~CD3DTextureEngine()
// Desc: Deletes the internal list of textures
//-----------------------------------------------------------------------------
CD3DTextureEngine::~CD3DTextureEngine()
{
    // Delete the list of textures
    SAFE_DELETE( g_ptcTextureList );
}




//-----------------------------------------------------------------------------
// Name: FindTexture()
// Desc: Searches the internal list of textures for a texture specified by
//       its name. Returns the structure associated with that texture.
//-----------------------------------------------------------------------------
static TextureContainer* FindTexture( TCHAR* strTextureName )
{
    FOREACHTEXTURE( ptcTexture )
    {
        if( !lstrcmpi( strTextureName, ptcTexture->strName ) )
            return ptcTexture;
    }

    return NULL;
}




//-----------------------------------------------------------------------------
// Name: FindTextureFile()
// Desc: Looks for the specified file in the current directory, in the path
//       specified in the environment variables, or in the path specified in
//       the registry. After finding it, the function returns the full path.
//-----------------------------------------------------------------------------
static HRESULT FindTextureFile( TCHAR* strFilename, TCHAR* strTexturePath,
                         TCHAR* strFullPath )
{
    // First, check the current directory
    _tcscpy( strFullPath, strFilename );
    if( FileExists( strFullPath ) )
        return DD_OK;

    // Next, check to see if an environment variable specifies the path
    TCHAR* strPath = _tgetenv( TEXT("D3DPATH") );
    if( NULL != strPath )
    {
        _stprintf( strFullPath, TEXT("%s\\%s"), strPath, strFilename );
        if( FileExists( strFullPath ) )
            return DD_OK;
    }

    // Finally, check the system registry for a path
    HKEY key;
    LONG result = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                TEXT("Software\\Microsoft\\DirectX"),
                                0, KEY_READ, &key );
    if( ERROR_SUCCESS == result )
    {
        TCHAR  strPath[512];
        DWORD type, size = 512;
        result = RegQueryValueEx( key, g_strRegValueName, NULL, &type,
                                  (BYTE*)strPath, &size );

        RegCloseKey( key );

        if( ERROR_SUCCESS == result )
        {
            _stprintf( strFullPath, TEXT("%s\\D3DIM\\Media\\%s"), strPath,
                       strFilename );
            if( FileExists( strFullPath ) )
                return DD_OK;

            _stprintf( strFullPath, TEXT("%s\\%s%s"), strPath,
                       strTexturePath, strFilename );
            if( FileExists( strFullPath ) )
                return DD_OK;
        }
    }

    return DDERR_NOTFOUND;
}




//-----------------------------------------------------------------------------
// Name: LoadTextureImage()
// Desc: Loads a texture map file into a BITMAP surface.
//-----------------------------------------------------------------------------
static HRESULT LoadTextureImage( TextureContainer* ptcTexture )
{
    TCHAR* strFilename = ptcTexture->strName;
    TCHAR* strExtension;
    TCHAR  strPathname[256];

    // Get the filename extension
    if( NULL == ( strExtension = _tcsrchr( strFilename, TEXT('.') ) ) )
        return DDERR_UNSUPPORTED;

    // Check the executable's resource. If it's there, we're done!
    if( NULL != ( ptcTexture->hbmBitmap = (HBITMAP)LoadImage(
		                                    GetModuleHandle(NULL),
                                            strFilename, IMAGE_BITMAP,
                                            0, 0, LR_CREATEDIBSECTION) ) )
        return S_OK;

    // Check the current path and system registry path for the file
    if( FAILED( FindTextureFile( strFilename, g_strTexturePath, strPathname ) ) )
        return DDERR_NOTFOUND;

    if( !lstrcmpi( strExtension, ".bmp" ) )
    {
        // Try to load the bitmap as a resource.
        ptcTexture->hbmBitmap = (HBITMAP)LoadImage( GetModuleHandle(NULL),
			                            strPathname, IMAGE_BITMAP, 0, 0, 
								        LR_CREATEDIBSECTION );

        // If the bitmap wasn't a resource, try it as a file.
        if( NULL == ptcTexture->hbmBitmap )
            ptcTexture->hbmBitmap = (HBITMAP)LoadImage( NULL, strPathname,
			                            IMAGE_BITMAP, 0, 0, 
										LR_LOADFROMFILE|LR_CREATEDIBSECTION );

        return (ptcTexture->hbmBitmap) ? DD_OK : DDERR_NOTFOUND;
    }

	// Can check for other file formats here

    return DDERR_UNSUPPORTED;
}




//-----------------------------------------------------------------------------
// Name: CopyBitmapToSurface()
// Desc: Copies the image of a bitmap into a surface
//-----------------------------------------------------------------------------
static HRESULT CopyBitmapToSurface( LPDIRECTDRAWSURFACE4 pddsTarget,
                                    HBITMAP hbmBitmap, DWORD dwFlags )
{
    // Get a DDraw object to create a temporary surface
    LPDIRECTDRAW4 pDD;
    pddsTarget->GetDDInterface( (VOID**)&pDD );
    pDD->Release();

    // Get the bitmap structure (to extract width, height, and bpp)
    BITMAP bm;
    GetObject( hbmBitmap, sizeof(BITMAP), &bm );

    // Setup the new surface desc
    DDSURFACEDESC2 ddsd;
    D3DUtil_InitSurfaceDesc( ddsd );
    pddsTarget->GetSurfaceDesc( &ddsd );
    ddsd.dwFlags          = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|
		                    DDSD_TEXTURESTAGE;
    ddsd.ddsCaps.dwCaps   = DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    ddsd.ddsCaps.dwCaps2  = 0L;
    ddsd.dwWidth          = bm.bmWidth;
    ddsd.dwHeight         = bm.bmHeight;

    // Create a new surface for the texture
    LPDIRECTDRAWSURFACE4 pddsTempSurface;
	HRESULT hr;
    if( FAILED( hr = pDD->CreateSurface( &ddsd, &pddsTempSurface, NULL ) ) )
        return NULL;

    // Get a DC for the bitmap
    HDC hdcBitmap = CreateCompatibleDC( NULL );
    if( NULL == hdcBitmap )
    {
        pddsTempSurface->Release();
        return NULL;
    }
    SelectObject( hdcBitmap, hbmBitmap );

    // Handle palettized textures. Need to attach a palette
    if( ddsd.ddpfPixelFormat.dwRGBBitCount == 8 )
    {
        // Create the color table and parse in the palette
        DWORD pe[256];
        WORD  wNumColors = GetDIBColorTable( hdcBitmap, 0, 256,
                                             (RGBQUAD*)pe );
        for( WORD i=0; i<wNumColors; i++ )
        {
            pe[i] = 0xff000000 + RGB( GetBValue(pe[i]), GetGValue(pe[i]),
                                      GetRValue(pe[i]) );

			// Set alpha for transparent pixels
			if( dwFlags & D3DTEXTR_TRANSPARENTBLACK )
            {
                if( (pe[i]&0x00ffffff) == 0x00000000 )
                    pe[i] &= 0x00ffffff;
            }
            else if( dwFlags & D3DTEXTR_TRANSPARENTWHITE )
            {
                if( (pe[i]&0x00ffffff) == 0x00ffffff )
                    pe[i] &= 0x00ffffff;
            }
		}
        // Create & attach a palette with the bitmap's colors
        LPDIRECTDRAWPALETTE  pPalette;
		if( dwFlags & (D3DTEXTR_TRANSPARENTWHITE|D3DTEXTR_TRANSPARENTBLACK) )
            pDD->CreatePalette( DDPCAPS_8BIT|DDPCAPS_ALPHA, (PALETTEENTRY*)pe, &pPalette, NULL );
        else
            pDD->CreatePalette( DDPCAPS_8BIT, (PALETTEENTRY*)pe, &pPalette, NULL );
        pddsTempSurface->SetPalette( pPalette );
        pddsTarget->SetPalette( pPalette );
        SAFE_RELEASE( pPalette );
    }

    // Copy the bitmap image to the surface.
    HDC hdcSurface;
    if( SUCCEEDED( pddsTempSurface->GetDC( &hdcSurface ) ) )
    {
        BitBlt( hdcSurface, 0, 0, bm.bmWidth, bm.bmHeight, hdcBitmap, 0, 0,
                SRCCOPY );
        pddsTempSurface->ReleaseDC( hdcSurface );
    }
    DeleteDC( hdcBitmap );

    // Copy the temp surface to the real texture surface
    pddsTarget->Blt( NULL, pddsTempSurface, NULL, DDBLT_WAIT, NULL );

    if( ddsd.ddpfPixelFormat.dwRGBBitCount == 8 )
	{
        LPDIRECTDRAWPALETTE  pPalette;
		DWORD pe[256];
        pddsTempSurface->GetPalette( &pPalette );
		pPalette->GetEntries( 0, 0, 256, (PALETTEENTRY*)&pe );
		pPalette->Release();

		pddsTarget->GetPalette( &pPalette );
		pPalette->GetEntries( 0, 0, 256, (PALETTEENTRY*)&pe );
		pPalette->Release();
	}

    pddsTempSurface->Release();

	// For textures with real alpha (not palettized), set transparent bits
    if( ddsd.ddpfPixelFormat.dwRGBAlphaBitMask )
	{
		if( dwFlags & (D3DTEXTR_TRANSPARENTWHITE|D3DTEXTR_TRANSPARENTBLACK) )
		{
			// Lock the texture surface
			DDSURFACEDESC2 ddsd;
			D3DUtil_InitSurfaceDesc( ddsd );
			while( pddsTarget->Lock( NULL, &ddsd, 0, NULL ) ==
				   DDERR_WASSTILLDRAWING );
			 
		    DWORD dwAlphaMask = ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
			DWORD dwRGBMask   = ( ddsd.ddpfPixelFormat.dwRBitMask |
							      ddsd.ddpfPixelFormat.dwGBitMask |
							      ddsd.ddpfPixelFormat.dwBBitMask );
			DWORD dwColorkey  = 0x00000000; // Colorkey on black
			if( dwFlags & D3DTEXTR_TRANSPARENTWHITE ) 
				dwColorkey = dwRGBMask;     // Colorkey on white

			// Add an opaque alpha value to each non-colorkeyed pixel
			for( DWORD y=0; y<ddsd.dwHeight; y++ )
			{
				WORD*  p16 =  (WORD*)((BYTE*)ddsd.lpSurface + y*ddsd.lPitch);
				DWORD* p32 = (DWORD*)((BYTE*)ddsd.lpSurface + y*ddsd.lPitch);
        
				for( DWORD x=0; x<ddsd.dwWidth; x++ )
				{
					if( ddsd.ddpfPixelFormat.dwRGBBitCount == 16 )
					{
						if( ( *p16 &= dwRGBMask ) != dwColorkey )
							*p16 |= dwAlphaMask;
						p16++;
					}
					if( ddsd.ddpfPixelFormat.dwRGBBitCount == 32 )
					{
						if( ( *p32 &= dwRGBMask ) != dwColorkey )
							*p32 |= dwAlphaMask;
						p32++;
					}
				}
			}
			pddsTarget->Unlock( NULL );
		}
	}

    return S_OK;;
}




//-----------------------------------------------------------------------------
// Name: D3DTextr_GetSurface()
// Desc: Returns a pointer to a d3dSurface from the name of the texture
//-----------------------------------------------------------------------------
LPDIRECTDRAWSURFACE4 D3DTextr_GetSurface( TCHAR* strName )
{
    TextureContainer* ptcTexture = FindTexture( strName );

    return ptcTexture ? ptcTexture->pddsSurface : NULL;
}




//-----------------------------------------------------------------------------
// Name: D3DTextr_GetTexture()
// Desc: Returns a pointer to a d3dTexture from the name of the texture
//-----------------------------------------------------------------------------
LPDIRECT3DTEXTURE2 D3DTextr_GetTexture( TCHAR* strName )
{
    TextureContainer* ptcTexture = FindTexture( strName );

    return ptcTexture ? ptcTexture->ptexTexture: NULL;
}




//-----------------------------------------------------------------------------
// Name: D3DTextr_SetTexturePath()
// Desc: Enumeration callback routine to find a best-matching texture format.
//-----------------------------------------------------------------------------
VOID D3DTextr_SetTexturePath( TCHAR* strTexturePath )
{
    if( NULL==strTexturePath )
        strTexturePath = TEXT("");
    _tcscpy( g_strTexturePath, strTexturePath );
}




//-----------------------------------------------------------------------------
// Name: D3DTextr_CreateTexture()
// Desc: Is passed a filename and creates a local Bitmap from that file.
//       The texture can not be used until it is restored, however.
//-----------------------------------------------------------------------------
HRESULT D3DTextr_CreateTexture( TCHAR* strName, DWORD dwStage, DWORD dwFlags )
{
    // Check first to see if the texture is already loaded
    if( NULL != FindTexture( strName ) )
        return S_OK;

    // Allocate and add the texture to the linked list of textures;
    TextureContainer* ptcTexture = new TextureContainer();
    if( NULL == ptcTexture )
        return E_OUTOFMEMORY;
    ZeroMemory( ptcTexture, sizeof(TextureContainer) );
    lstrcpy( ptcTexture->strName, strName );
    ptcTexture->dwStage = dwStage;
    ptcTexture->dwFlags = dwFlags;

    // Create a bitmap and load the texture file into it,
    if( FAILED( LoadTextureImage( ptcTexture ) ) )
	{
		delete ptcTexture;
        return E_FAIL;
	}

    // Add the texture to the global linked list
    if( g_ptcTextureList )
        g_ptcTextureList->pPrev = ptcTexture;
    ptcTexture->pNext = g_ptcTextureList;
    g_ptcTextureList  = ptcTexture;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreFromBitmap()
// Desc: Invalidates the current texture objects and rebuilds new ones
//       using the new device.
//-----------------------------------------------------------------------------
static HRESULT RestoreFromBitmap( TextureContainer* ptcTexture, 
						          LPDIRECT3DDEVICE3 pd3dDevice )
{
    // Get the DirectDraw interface for creating surfaces
    LPDIRECTDRAW4 pDD;
    if( NULL == ( pDD = D3DUtil_GetDirectDrawFromDevice( pd3dDevice ) ) )
        return E_FAIL;
    pDD->Release();

    // Get the device caps
    D3DDEVICEDESC ddHwDesc, ddSwDesc;
    DWORD         dwDeviceCaps;
    ddHwDesc.dwSize = sizeof(D3DDEVICEDESC);
    ddSwDesc.dwSize = sizeof(D3DDEVICEDESC);
    if( FAILED( pd3dDevice->GetCaps( &ddHwDesc, &ddSwDesc ) ) )
        return E_FAIL;
    if( ddHwDesc.dwFlags ) dwDeviceCaps = ddHwDesc.dpcTriCaps.dwTextureCaps;
    else                   dwDeviceCaps = ddSwDesc.dpcTriCaps.dwTextureCaps;

	// Get the bitmap structure (to extract width, height, and bpp)
    BITMAP bm;
    HBITMAP hbmBitmap = ptcTexture->hbmBitmap;
    GetObject( hbmBitmap, sizeof(BITMAP), &bm );
    DWORD dwWidth  = (DWORD)bm.bmWidth;
    DWORD dwHeight = (DWORD)bm.bmHeight;

    // Setup the new surface desc
    DDSURFACEDESC2 ddsd;
    D3DUtil_InitSurfaceDesc( ddsd );
    ddsd.dwFlags         = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|
                           DDSD_PIXELFORMAT|DDSD_TEXTURESTAGE;
    ddsd.ddsCaps.dwCaps  = DDSCAPS_TEXTURE;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    ddsd.dwTextureStage  = ptcTexture->dwStage;
    ddsd.dwWidth         = dwWidth;
    ddsd.dwHeight        = dwHeight;

    // Adjust width and height, if the driver requires it
    if( dwDeviceCaps & D3DPTEXTURECAPS_POW2 )
    {
        for( ddsd.dwWidth=1;  dwWidth>ddsd.dwWidth;   ddsd.dwWidth<<=1 );
        for( ddsd.dwHeight=1; dwHeight>ddsd.dwHeight; ddsd.dwHeight<<=1 );
    }
    if( dwDeviceCaps & D3DPTEXTURECAPS_SQUAREONLY )
    {
        if( ddsd.dwWidth > ddsd.dwHeight ) ddsd.dwHeight = ddsd.dwWidth;
        else                               ddsd.dwWidth  = ddsd.dwHeight;
    }

	BOOL bUsePalette = ( bm.bmBitsPixel <= 8 );
	BOOL bUseAlpha   = FALSE;

	if( ptcTexture->bHasAlpha )
		bUseAlpha = TRUE;
	
	if( ptcTexture->dwFlags & (D3DTEXTR_TRANSPARENTWHITE|D3DTEXTR_TRANSPARENTBLACK) )
	{
		if( bUsePalette )
		{
			if( dwDeviceCaps & D3DPTEXTURECAPS_ALPHAPALETTE )
			{
				bUseAlpha   = TRUE;
				bUsePalette = TRUE;
			}
			else
			{
				bUseAlpha   = TRUE;
				bUsePalette = FALSE;
			}
		}
	}

	// Setup the structure to be used for texture enumration.
	TEXTURESEARCHINFO tsi;
	tsi.pddpf            = &ddsd.ddpfPixelFormat;
	tsi.bUseAlpha        = bUseAlpha;
	tsi.bUsePalette      = bUsePalette;
	tsi.bUseFourCC       = ( ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC );
	tsi.dwDesiredBPP     = 16;
	tsi.bFoundGoodFormat = FALSE;
	if( ptcTexture->dwFlags & D3DTEXTR_32BITSPERPIXEL )
		tsi.dwDesiredBPP = 32;

	// Enumerate the texture formats, and find the closest device-supported
	// texture pixel format
    pd3dDevice->EnumTextureFormats( TextureSearchCallback, &tsi );

	// If a palettized format was requested, but not found, default to a
	// 16-bit texture format
	if( FALSE == tsi.bFoundGoodFormat && bUsePalette )
	{
		tsi.bUsePalette  = FALSE;
		tsi.dwDesiredBPP = 16;
	    pd3dDevice->EnumTextureFormats( TextureSearchCallback, &tsi );
		if( FALSE == tsi.bFoundGoodFormat )
			return E_FAIL;
	}
	
    // Create a new surface for the texture
    HRESULT hr;
    if( FAILED( hr = pDD->CreateSurface( &ddsd, &ptcTexture->pddsSurface, NULL ) ) )
        return E_FAIL;

    // Create the texture
    if( FAILED( ptcTexture->pddsSurface->QueryInterface( IID_IDirect3DTexture2,
                                         (VOID**)&ptcTexture->ptexTexture ) ) )
        return E_FAIL;


	// Copy the bitmap to the texture surface
    return CopyBitmapToSurface( ptcTexture->pddsSurface, hbmBitmap,
		                        ptcTexture->dwFlags );

}




//-----------------------------------------------------------------------------
// Name: D3DTextr_Restore()
// Desc: Invalidates the current texture objects and rebuilds new ones
//       using the new device.
//-----------------------------------------------------------------------------
HRESULT D3DTextr_Restore( TCHAR* strName, LPDIRECT3DDEVICE3 pd3dDevice )
{
    // Check params
    if( NULL == pd3dDevice )
        return DDERR_INVALIDPARAMS;

    TextureContainer* ptcTexture = FindTexture( strName );
    if( NULL == ptcTexture )
        return DDERR_NOTFOUND;

    // Release any previously created objects
    SAFE_RELEASE( ptcTexture->ptexTexture );
    SAFE_RELEASE( ptcTexture->pddsSurface );

	// Restore the texture surface from the bitmap image. At this point, code
	// can be added to handle other texture formats, such as those created from
	// .dds files, .jpg files, or whatever else.
	return RestoreFromBitmap( ptcTexture, pd3dDevice );
}	
	



//-----------------------------------------------------------------------------
// Name: D3DTextr_RestoreAllTextures()
// Desc: This function is called when a mode is changed. It updates all
//       texture objects to be valid with the new device.
//-----------------------------------------------------------------------------
HRESULT D3DTextr_RestoreAllTextures( LPDIRECT3DDEVICE3 pd3dDevice )
{
    FOREACHTEXTURE( ptcTexture )
    {
        D3DTextr_Restore( ptcTexture->strName, pd3dDevice );
    }

    return DD_OK;
}




//-----------------------------------------------------------------------------
// Name: D3DTextr_Invalidate()
// Desc: Used to bump a texture out of (video) memory, this function
//       actually destroys the d3dtexture and ddsurface of the texture
//-----------------------------------------------------------------------------
HRESULT D3DTextr_Invalidate( TCHAR* strName )
{
    TextureContainer* ptcTexture = FindTexture( strName );
    if( NULL == ptcTexture )
        return DDERR_NOTFOUND;

    SAFE_RELEASE( ptcTexture->ptexTexture );
    SAFE_RELEASE( ptcTexture->pddsSurface );

    return DD_OK;
}




//-----------------------------------------------------------------------------
// Name: D3DTextr_InvalidateAllTextures()
// Desc: This function is called when a mode is changed. It invalidates
//       all texture objects so their device can be safely released.
//-----------------------------------------------------------------------------
HRESULT D3DTextr_InvalidateAllTextures()
{
    FOREACHTEXTURE( ptcTexture )
    {
        SAFE_RELEASE( ptcTexture->ptexTexture );
        SAFE_RELEASE( ptcTexture->pddsSurface );
    }

    return DD_OK;
}




//-----------------------------------------------------------------------------
// Name: D3DTextr_DestroyTexture()
// Desc: Frees the resources for the specified texture container
//-----------------------------------------------------------------------------
HRESULT D3DTextr_DestroyTexture( TCHAR* strName )
{
    TextureContainer* ptcTexture = FindTexture( strName );
    if( NULL == ptcTexture )
        return DDERR_NOTFOUND;

    // Remove the texture container from the global list
    if( ptcTexture->pPrev )
        ptcTexture->pPrev->pNext = ptcTexture->pNext;
    else
        g_ptcTextureList = ptcTexture->pNext;
    ptcTexture->pNext = NULL;

    SAFE_DELETE( ptcTexture );

    return DD_OK;
}

