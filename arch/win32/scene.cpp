//-----------------------------------------------------------------------------
// File: VBuffer.cpp
//
// Desc: Example code showing how to use DirectX 6 vertex buffers.
//
//       Note: This code uses the D3D Framework helper library.
//
//
// Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "pch.h"
#include "scene.h"
#include <stack>

#include "D3DTextr.h"
#include "D3DUtil.h"
#include "D3DMath.h"
#include "D3DFrame.h"

#include "texture.h"

extern CD3DFramework* g_pFramework;
extern BOOL           g_bWindowed;

extern "C" {
#include "d3dhelp.h"
}

//-----------------------------------------------------------------------------
// Declare the application globals for use in WinMain.cpp
//-----------------------------------------------------------------------------
void   Win32_SetupColor(void);
BOOL   g_TrueColor;
int    g_RShift, g_GShift, g_BShift;
int    g_RBits6, g_GBits6, g_BBits6;

TCHAR* g_strAppTitle       = TEXT( "Direct3D Descent" );
BOOL   g_bAppUseZBuffer    = FALSE;
BOOL   g_bAppUseBackBuffer = TRUE;

LPDIRECT3DTEXTURE2 g_pd3dtLast;
BOOL g_bTransparentLast;

std::stack <D3DMATRIX> g_stkWorlds;

float g_fPSURed = 0.0f;
float g_fPSUGreen = 0.0f;
float g_fPSUBlue = 0.0f;

D3DLVERTEX g_rgVerts [16];

CTextureSet g_setTextures;

//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Function prototypes and global (or static) variables
//-----------------------------------------------------------------------------
HRESULT App_InitDeviceObjects( LPDIRECT3DDEVICE3, LPDIRECT3DVIEWPORT3 );
VOID    App_DeleteDeviceObjects( LPDIRECT3DDEVICE3, LPDIRECT3DVIEWPORT3 );




//-----------------------------------------------------------------------------
// Name: App_OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT App_OneTimeSceneInit( HWND hWnd )
{
    // Create some textures
    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: App_InitDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT App_InitDeviceObjects( LPDIRECT3DDEVICE3 pd3dDevice,
                               LPDIRECT3DVIEWPORT3 pvViewport )
{
    // Check parameters
    if( NULL==pd3dDevice || NULL==pvViewport )
        return E_INVALIDARG;


	D3DVIEWPORT2 vdData;
    ZeroMemory( &vdData, sizeof(D3DVIEWPORT2) );
    vdData.dwSize		= sizeof(vdData);
	vdData.dwX			= 0;
	vdData.dwY			= 0;
    vdData.dwWidth		= g_pFramework->m_dwRenderWidth;
    vdData.dwHeight		= 147 * g_pFramework->m_dwRenderHeight / 200;
    vdData.dvMaxZ		= 1.0f;

    vdData.dvClipX		= -1.0f;
    vdData.dvClipY		= (FLOAT) g_pFramework->m_dwRenderHeight / (FLOAT) g_pFramework->m_dwRenderWidth;
    vdData.dvClipWidth	= 2.0f;
    vdData.dvClipHeight	= 2.0f * vdData.dvClipY;
    vdData.dvMinZ		= 0.0f;
    vdData.dvMaxZ		= 1.0f;

    // Set the parameters to the new viewport
    if (FAILED (pvViewport->SetViewport2 (&vdData)))
    {
        DEBUG_MSG( TEXT("Error: Couldn't set the viewport data") );
        return D3DFWERR_NOVIEWPORT;
    }


	// Get a ptr to the ID3D object to create VB's, materials and/or lights.
    // Note: the Release() call just serves to decrease the ref count.
    LPDIRECT3D3 pD3D;
    if( FAILED( pd3dDevice->GetDirect3D( &pD3D ) ) )
        return E_FAIL;
    pD3D->Release();

	// Get the device's caps bits
	D3DDEVICEDESC ddHwDesc, ddSwDesc;
	D3DUtil_InitDeviceDesc( ddHwDesc );
	D3DUtil_InitDeviceDesc( ddSwDesc );
	if( FAILED( pd3dDevice->GetCaps( &ddHwDesc, &ddSwDesc ) ) )
		return E_FAIL;

	D3DMATRIX matProj;
	float fAspect = 1.0f;
    D3DUtil_SetProjectionMatrix( matProj, g_PI/3, fAspect, 0.01f, 1000.0f );
    pd3dDevice->SetTransform( D3DTRANSFORMSTATE_PROJECTION, &matProj );
    
    pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	//pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFN_LINEAR );
	//pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR );

	pd3dDevice->SetRenderState (D3DRENDERSTATE_SPECULARENABLE, FALSE);
	pd3dDevice->SetRenderState (D3DRENDERSTATE_DITHERENABLE, TRUE);
	pd3dDevice->SetRenderState (D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
	pd3dDevice->SetRenderState (D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
	pd3dDevice->SetRenderState (D3DRENDERSTATE_ZENABLE, 0);
	pd3dDevice->SetRenderState (D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
	pd3dDevice->SetRenderState (D3DRENDERSTATE_COLORKEYENABLE, FALSE);

	D3DMATRIX matID;
	D3DUtil_SetIdentityMatrix (matID);

	while (!g_stkWorlds.empty ())
		g_stkWorlds.pop ();

	g_stkWorlds.push (matID);
    pd3dDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, &matID );

	g_setTextures.Initialize ();

	g_bTransparentLast = FALSE;
	g_pd3dtLast = NULL;

	Win32_SetupColor();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: App_FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT App_FinalCleanup( LPDIRECT3DDEVICE3 pd3dDevice, 
                          LPDIRECT3DVIEWPORT3 pvViewport)
{
    App_DeleteDeviceObjects( pd3dDevice, pvViewport );
    
	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: App_DeleteDeviceObjects()
// Desc: Called when the app is exitting, or the device is being changed,
//       this function deletes any device dependant objects.
//-----------------------------------------------------------------------------
VOID App_DeleteDeviceObjects( LPDIRECT3DDEVICE3 pd3dDevice, 
                              LPDIRECT3DVIEWPORT3 pvViewport)
{
    D3DTextr_InvalidateAllTextures();

	g_setTextures.Uninitialize ();

	Win32_InvalidatePages();

	Win32_SetupColor();
}




//----------------------------------------------------------------------------
// Name: App_RestoreSurfaces
// Desc: Restores any previously lost surfaces. Must do this for all surfaces
//       (including textures) that the app created.
//----------------------------------------------------------------------------
HRESULT App_RestoreSurfaces()
{
	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: App_ConfirmDevice()
// Desc: Called during device intialization, this code checks the device
//       for some minimum set of capabilities
//-----------------------------------------------------------------------------
HRESULT App_ConfirmDevice( DDCAPS* pddDriverCaps,
						   D3DDEVICEDESC* pd3dDeviceDesc )
{
	// Don't allow 3d Devices that don't support textures(Matrox Millenium)
	if (!pd3dDeviceDesc->dwMaxTextureWidth)
		return DDERR_INVALIDOBJECT;

    return S_OK;

}




#define byte __BOGUS1
#define bool __BOGUS2

extern "C"
{
#include "types.h"
#include "grdef.h"
#include "..\main\segment.h"
#include "..\main\segpoint.h"
#include "..\main\object.h"
#include "..\3d\globvars.h"
#include "3d.h"
#include "palette.h"
}

#undef __BOGUS1
#undef __BOGUS2

LPDIRECT3DDEVICE3 g_pd3dDevice;

HRESULT App_StartFrame( LPDIRECT3DDEVICE3 pd3dDevice, LPDIRECT3DVIEWPORT3 pvViewport, D3DRECT* prcViewRect )
{
	g_pd3dDevice = pd3dDevice;

	HRESULT hResult;

	hResult = pvViewport->Clear2 (1UL, prcViewRect, D3DCLEAR_TARGET, 0, 1.0f, 0);

    // Begin the scene 
    hResult = pd3dDevice->BeginScene();

	ASSERT (g_stkWorlds.size () == 1);

	return hResult;
}

HRESULT App_EndFrame (void)
{
	HRESULT hResult;
	
	hResult = g_pd3dDevice->EndScene ();

	ASSERT (g_stkWorlds.size () == 1);

	return hResult;
}

extern "C" RGBQUAD w32lastrgb[256];

extern "C" void Win32_DoSetPalette (PALETTEENTRY *rgpe)
{
	g_setTextures.SetPaletteEntries (rgpe);
	for (int i = 0; i < 256; i++) {
		w32lastrgb[i].rgbBlue = rgpe[i].peBlue;
		w32lastrgb[i].rgbGreen = rgpe[i].peGreen;
		w32lastrgb[i].rgbRed = rgpe[i].peRed;
	}
}

extern "C" void Win32_DoGetPalette (PALETTEENTRY *rgpe)
{
	g_setTextures.GetPaletteEntries (rgpe);
}

extern "C" int Win32_PaletteStepUp (int r, int g, int b)
{
	if (g_setTextures.HasPalette ())
	{
		g_fPSURed = g_fPSUGreen = g_fPSUBlue = 0.0f;
		return FALSE;
	}
	else
	{
		g_fPSURed = (float) r / 32.0f;
		g_fPSUGreen = (float) g / 32.0f;
		g_fPSUBlue = (float) b / 32.0f;
		return TRUE;
	}
}

D3DVECTOR g_vecRight, g_vecUp;

extern "C" void Win32_set_view_matrix ()
{
	vms_vector *pPos = &View_position;
	vms_matrix *pOrient = &View_matrix;

	D3DVECTOR vecView;
	vecView.x = (D3DVALUE) f2fl (pPos->x) / 10;
	vecView.y = (D3DVALUE) f2fl (pPos->y) / 10;
	vecView.z = (D3DVALUE) f2fl (pPos->z) / 10;

	D3DMATRIX matView;
	D3DVECTOR vecDir;
	matView(0, 0) = g_vecRight.x = (D3DVALUE) f2fl (pOrient->rvec.x) / 10;
	matView(1, 0) = g_vecRight.y = (D3DVALUE) f2fl (pOrient->rvec.y) / 10;
	matView(2, 0) = g_vecRight.z = (D3DVALUE) f2fl (pOrient->rvec.z) / 10;

	matView(0, 1) = g_vecUp.x = (D3DVALUE) f2fl (pOrient->uvec.x) / 10;
	matView(1, 1) = g_vecUp.y = (D3DVALUE) f2fl (pOrient->uvec.y) / 10;
	matView(2, 1) = g_vecUp.z = (D3DVALUE) f2fl (pOrient->uvec.z) / 10;

	matView(0, 2) = vecDir.x = (D3DVALUE) f2fl (pOrient->fvec.x) / 10;
	matView(1, 2) = vecDir.y = (D3DVALUE) f2fl (pOrient->fvec.y) / 10;
	matView(2, 2) = vecDir.z = (D3DVALUE) f2fl (pOrient->fvec.z) / 10;

	matView(3, 0) = -DotProduct (g_vecRight, vecView);
	matView(3, 1) = -DotProduct (g_vecUp, vecView);
	matView(3, 2) = -DotProduct (vecDir, vecView);

	matView(0, 3) = matView(1, 3) = matView(2, 3) = 0.0f;
	matView(3, 3) = 1.0f;

    g_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, &matView );
}

HRESULT SetTexture (CTexture *pTexture)
{
	HRESULT hResult = S_OK;
	LPDIRECT3DTEXTURE2 pd3dt;
	BOOL bTransparent;

	if (pTexture != NULL)
	{
		pd3dt = pTexture->GetTexture ();
		bTransparent = pTexture->IsTransparent ();
	}
	else
	{
		pd3dt = NULL;
		bTransparent = FALSE;
	}

	if (pd3dt != g_pd3dtLast)
	{
		g_pd3dtLast = pd3dt;

		hResult = g_pd3dDevice->SetTexture (0, pd3dt);
		ASSERT (SUCCEEDED (hResult));
	}

	if (bTransparent != g_bTransparentLast)
	{
		g_bTransparentLast = bTransparent;

		hResult = g_pd3dDevice->SetRenderState (D3DRENDERSTATE_COLORKEYENABLE, bTransparent);
		ASSERT (SUCCEEDED (hResult));
	}
	return hResult;
}

extern "C" bool g3_draw_tmap (int cPoints, g3s_point **rgpPoints, g3s_uvl *rgpUvls, grs_bitmap *pbm)
{
	ASSERT (cPoints <= sizeof (g_rgVerts) / sizeof (g_rgVerts[0]));

	for (ULONG iPoint = 0; iPoint < cPoints; iPoint ++)
	{
		D3DLVERTEX *pvert = &g_rgVerts [iPoint];
		const g3s_point *pPoint = rgpPoints [iPoint];
		const g3s_uvl *pUvl = &rgpUvls [iPoint];
		double dCol = f2fl (pUvl->l);
		double dColR = min (1, max (0, dCol + g_fPSURed));
		double dColG = min (1, max (0, dCol + g_fPSUGreen));
		double dColB = min (1, max (0, dCol + g_fPSUBlue));

		pvert->x = (D3DVALUE) f2fl (pPoint->p3_orig.x) / 10;
		pvert->y = (D3DVALUE) f2fl (pPoint->p3_orig.y) / 10;
		pvert->z = (D3DVALUE) f2fl (pPoint->p3_orig.z) / 10;
		pvert->tu = (D3DVALUE) f2fl (pUvl->u);
		pvert->tv = (D3DVALUE) f2fl (pUvl->v);
		pvert->color = D3DRGBA (dColR, dColG, dColB, 0);
		pvert->specular = 0;
	}

	HRESULT hResult;
	hResult = SetTexture ((CTexture *) pbm->pvSurface);
	ASSERT (SUCCEEDED (hResult));

	hResult = g_pd3dDevice->DrawPrimitive (D3DPT_TRIANGLEFAN, D3DFVF_LVERTEX, g_rgVerts, cPoints, D3DDP_WAIT);
	ASSERT (SUCCEEDED (hResult));

	return hResult == S_OK;
}

extern "C" bool g3_draw_poly (int cPoints, g3s_point **rgpPoints)
{
	ASSERT (cPoints <= sizeof (g_rgVerts) / sizeof (g_rgVerts[0]));
	PALETTEENTRY pe = g_setTextures.ReadPalette (grd_curcanv->cv_color);
	BYTE bR = pe.peRed;
	BYTE bG = pe.peGreen;
	BYTE bB = pe.peBlue;
	double dColR = min (1, max (0, (float) bR / 256 + g_fPSURed));
	double dColG = min (1, max (0, (float) bG / 256 + g_fPSUGreen));
	double dColB = min (1, max (0, (float) bB / 256 + g_fPSUBlue));

	for (ULONG iPoint = 0; iPoint < cPoints; iPoint ++)
	{
		D3DLVERTEX *pvert = &g_rgVerts [iPoint];
		const g3s_point *pPoint = rgpPoints [iPoint];

		pvert->x = (D3DVALUE) f2fl (pPoint->p3_orig.x) / 10;
		pvert->y = (D3DVALUE) f2fl (pPoint->p3_orig.y) / 10;
		pvert->z = (D3DVALUE) f2fl (pPoint->p3_orig.z) / 10;
		pvert->tu = 0;
		pvert->tv = 0;
		pvert->color = D3DRGBA (dColR, dColG, dColB, 0);
		pvert->specular = 0;
	}

	HRESULT hResult;
	hResult = SetTexture (NULL);
	ASSERT (SUCCEEDED (hResult));

	hResult = g_pd3dDevice->DrawPrimitive (D3DPT_TRIANGLEFAN, D3DFVF_LVERTEX, g_rgVerts, cPoints, D3DDP_WAIT);
	ASSERT (SUCCEEDED (hResult));

	return S_OK;
}



extern "C" bool g3_draw_bitmap (vms_vector *pos, fix width, fix height,grs_bitmap *pbm)
{
	ULONG cPoints = 4;

	D3DVECTOR vecRight = g_vecRight * f2fl (width);
	D3DVECTOR vecUp = g_vecUp * f2fl (height);
	D3DVECTOR vecPos (
		(D3DVALUE) f2fl (pos->x) / 10,
		(D3DVALUE) f2fl (pos->y) / 10,
		(D3DVALUE) f2fl (pos->z) / 10);

	double dCol = 0.5;
	double dColR = min (1, max (0, dCol + g_fPSURed));
	double dColG = min (1, max (0, dCol + g_fPSUGreen));
	double dColB = min (1, max (0, dCol + g_fPSUBlue));
	D3DCOLOR col = D3DRGBA (dColR, dColG, dColB, 0);

	D3DVALUE flAdjX, flAdjY;
	CTexture *pTexture = (CTexture *) pbm->pvSurface;
	pTexture->GetBitmapAdj (&flAdjX, &flAdjY);

	D3DLVERTEX rgVerts [4] =
	{
		D3DLVERTEX (vecPos - vecRight - vecUp, col, 0, 0,      flAdjY),
		D3DLVERTEX (vecPos - vecRight + vecUp, col, 0, 0,      0),
		D3DLVERTEX (vecPos + vecRight + vecUp, col, 0, flAdjX, 0),
		D3DLVERTEX (vecPos + vecRight - vecUp, col, 0, flAdjX, flAdjY),
	};

	HRESULT hResult;
	hResult = SetTexture (pTexture);
	ASSERT (SUCCEEDED (hResult));

	hResult = g_pd3dDevice->DrawPrimitive (D3DPT_TRIANGLEFAN, D3DFVF_LVERTEX, rgVerts, cPoints, D3DDP_WAIT);
	ASSERT (SUCCEEDED (hResult));

	return S_OK;
}

extern "C" void BlitToPrimary(HDC hSrcDC)
{
	RECT rectDest;
	rectDest.left	= 0;
	rectDest.top	= 0;
	rectDest.right	= g_pFramework->m_dwRenderWidth;
	rectDest.bottom	= g_pFramework->m_dwRenderHeight;

	if (g_bWindowed)
	{
		rectDest.left	+= g_pFramework->m_rcScreenRect.left;
		rectDest.right	+= g_pFramework->m_rcScreenRect.left;
		rectDest.top	+= g_pFramework->m_rcScreenRect.top;
		rectDest.bottom	+= g_pFramework->m_rcScreenRect.top;
	}

	HDC hDstDC;
	if (g_pFramework->GetFrontBuffer()->GetDC(&hDstDC) == DD_OK)
	{
		StretchBlt(hDstDC, rectDest.left, rectDest.top, 
			rectDest.right - rectDest.left,	rectDest.bottom - rectDest.top,
			hSrcDC, 0, 0, 320, 200, SRCCOPY);
		g_pFramework->GetFrontBuffer()->ReleaseDC(hDstDC);
	}
	return;
}

extern "C" void BlitToPrimaryRect(HDC hSrcDC, int x, int y, int w, int h, 
	unsigned char *dst)
{
	RECT rectDest;
	int destWidth = g_pFramework->m_dwRenderWidth;
	int destHeight = g_pFramework->m_dwRenderHeight;

	rectDest.left	= (x * destWidth) / 320;
	rectDest.top	= (y * destHeight) / 200;
	rectDest.right	= ((x + w) * destWidth) / 320;
	rectDest.bottom	= ((y + h) * destHeight) / 200;

	if (g_bWindowed && (int)dst == BM_D3D_DISPLAY)
	{
		rectDest.left	+= g_pFramework->m_rcScreenRect.left;
		rectDest.right	+= g_pFramework->m_rcScreenRect.left;
		rectDest.top	+= g_pFramework->m_rcScreenRect.top;
		rectDest.bottom	+= g_pFramework->m_rcScreenRect.top;
	}

	HDC hDstDC;
	IDirectDrawSurface4 *pddsDest = ((int)dst == BM_D3D_DISPLAY) ?
		g_pFramework->GetFrontBuffer() :
		g_pFramework->GetRenderSurface();
	if (pddsDest->GetDC(&hDstDC) == DD_OK)
	{
		StretchBlt(hDstDC, rectDest.left, rectDest.top, 
			rectDest.right - rectDest.left,	rectDest.bottom - rectDest.top,
			hSrcDC, x, y, w, h, SRCCOPY);
		pddsDest->ReleaseDC(hDstDC);
	}
	return;
}

HRESULT Blit (
	RECT &rectDest,
	LPDIRECTDRAWSURFACE4 pddsSrc,
	RECT &rectSrc,
	DWORD dwFlags,
	DDBLTFX *pddbltfx,
	BOOL bPrimary)
{
	rectDest.left	= rectDest.left * g_pFramework->m_dwRenderWidth / 320;
	rectDest.top	= rectDest.top  * g_pFramework->m_dwRenderHeight / 200;
	rectDest.right	= (rectDest.right  + 1) * g_pFramework->m_dwRenderWidth  / 320;
	rectDest.bottom	= (rectDest.bottom + 1) * g_pFramework->m_dwRenderHeight / 200;

	if (g_bWindowed && bPrimary)
	{
		rectDest.left	+= g_pFramework->m_rcScreenRect.left;
		rectDest.right	+= g_pFramework->m_rcScreenRect.left;
		rectDest.top	+= g_pFramework->m_rcScreenRect.top;
		rectDest.bottom	+= g_pFramework->m_rcScreenRect.top;
	}

	IDirectDrawSurface4 *pddsDest;
	if (bPrimary)
	{
		pddsDest = g_pFramework->GetFrontBuffer ();
	}
	else
	{
		pddsDest = g_pFramework->GetRenderSurface();
	}

	return pddsDest->Blt (
		&rectDest,
		pddsSrc,
		&rectSrc,
		dwFlags,
		pddbltfx);
}

static void findmask6(int in_mask, int *shift, int *bits6)
{
	int i;

	if (!in_mask) {
		*shift = 0;
		*bits6 = 6;
		return;
	}
	i = 0;
	while (!(in_mask & 1))
	{
		in_mask >>= 1;
		i++;
	}
	*shift = i;
	i = 0;
	while (in_mask & 1)
	{
		in_mask >>= 1;
		i++;
	}
	*bits6 = 6 - i;
}

void Win32_SetupColor(void)
{
	DDPIXELFORMAT pf; 
	pf.dwSize = sizeof(pf);
	g_pFramework->GetFrontBuffer ()->GetPixelFormat(&pf);
	g_TrueColor = pf.dwRGBBitCount == 24 || pf.dwRGBBitCount == 32;
	if (!g_TrueColor)
	{
		findmask6(pf.dwRBitMask, &g_RShift, &g_RBits6);
		findmask6(pf.dwGBitMask, &g_GShift, &g_GBits6);
		findmask6(pf.dwBBitMask, &g_BShift, &g_BBits6);
	}
}

extern "C" void Win32_Rect (
	int left, int top, int right, int bot,
	int iSurf,
	int iCol)
{
	RECT rectDest = {left, top, right, bot};

	DDBLTFX ddbltfx;
	ddbltfx.dwSize = sizeof (ddbltfx);

	#if 0	
	if (g_setTextures.HasPalette ())
	{
		ddbltfx.dwFillColor = iCol;
	}
	else
	{
		PALETTEENTRY pe = g_setTextures.ReadPalette (iCol);
		ddbltfx.dwFillColor = *(DWORD*) &pe;
	}
	#else
	unsigned char *p = gr_current_pal + iCol * 3;
	if (g_TrueColor)
		ddbltfx.dwFillColor = (255 << 24) | (p[0] << 18) | 
			(p[1] << 10) | (p[2] << 2);
	else
		ddbltfx.dwFillColor = (p[0] >> g_RBits6) << g_RShift |
			(p[1] >> g_GBits6) << g_GShift |
			(p[2] >> g_BBits6) << g_BShift;
	#endif

	HRESULT hr = Blit (
		rectDest,
		NULL,
		rectDest,
		DDBLT_WAIT | DDBLT_COLORFILL,
		&ddbltfx,
		iSurf == BM_D3D_DISPLAY);
}


extern "C" void Win32_BlitLinearToDirectX (
	int w, int h,
	int dx, int dy,
	int sx, int sy,
	void *pvSurface,
	int iSurf,
	BOOL bTransparent)
{
	CTexture *pTexture = (CTexture *) pvSurface;

	if (pTexture == NULL)
	{
		return;
	}

	if (!(w <= pTexture->m_ulWidthSource && h <= pTexture->m_ulHeightSource))
	{
		ASSERT (FALSE);
	}

	if (pTexture->isDirtyMemory ())
	{
		pTexture->CleanMemory ();
	}

	RECT rectDest = {dx, dy, dx + w, dy + h};
	RECT rectSrc = {0, 0, w, h};

	HRESULT hr = Blit (
		rectDest,
		pTexture->GetSurface (),
		rectSrc,
		DDBLT_WAIT,
		NULL,
		iSurf == BM_D3D_DISPLAY);
}

extern "C" void Win32_start_instance_matrix (vms_vector *pPos, vms_matrix *pOrient)
{
	D3DMATRIX matOrientInv;
	if (pOrient != NULL)
	{
		D3DMATRIX matOrient;

		matOrient(0, 0) = (D3DVALUE) f2fl (pOrient->rvec.x);
		matOrient(1, 0) = (D3DVALUE) f2fl (pOrient->rvec.y);
		matOrient(2, 0) = (D3DVALUE) f2fl (pOrient->rvec.z);
		matOrient(3, 0) = 0;
		matOrient(0, 1) = (D3DVALUE) f2fl (pOrient->uvec.x);
		matOrient(1, 1) = (D3DVALUE) f2fl (pOrient->uvec.y);
		matOrient(2, 1) = (D3DVALUE) f2fl (pOrient->uvec.z);
		matOrient(3, 1) = 0;
		matOrient(0, 2) = (D3DVALUE) f2fl (pOrient->fvec.x);
		matOrient(1, 2) = (D3DVALUE) f2fl (pOrient->fvec.y);
		matOrient(2, 2) = (D3DVALUE) f2fl (pOrient->fvec.z);
		matOrient(3, 2) = 0;
		matOrient(0, 3) = 0;
		matOrient(1, 3) = 0;
		matOrient(2, 3) = 0;
		matOrient(3, 3) = 1;

		D3DMath_MatrixInvert (matOrientInv, matOrient);
	}
	else
	{
		D3DUtil_SetIdentityMatrix (matOrientInv);
	}

	D3DMATRIX matTranslate;
	D3DUtil_SetTranslateMatrix (
		matTranslate,
		(D3DVALUE) f2fl (pPos->x) / 10,
		(D3DVALUE) f2fl (pPos->y) / 10,
		(D3DVALUE) f2fl (pPos->z) / 10);

	D3DMATRIX matTmp;
	D3DMath_MatrixMultiply (matTmp, g_stkWorlds.top (), matTranslate);
	D3DMATRIX matWorld;
	D3DMath_MatrixMultiply (matWorld, matTmp, matOrientInv);

	g_stkWorlds.push (matWorld);

    g_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, &matWorld );
}

extern "C" void Win32_done_instance (void)
{
	g_stkWorlds.pop ();

	D3DMATRIX matWorld = g_stkWorlds.top ();
    g_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, &matWorld );
}


extern "C" void Win32_SetTextureBits (grs_bitmap *bm, unsigned char *data, int bRle)
{
	CTexture *pTexture = (CTexture *) bm->pvSurface;
	if (pTexture != NULL)
	{
		pTexture->SetBitmapData (data, bRle);
		pTexture->SetTransparent (bm->bm_flags & BM_FLAG_TRANSPARENT);
	}
}

extern "C" void Win32_CreateTexture (grs_bitmap *bm)
{
	bm->pvSurface = (void *) g_setTextures.CreateTexture (
		bm->bm_data,
		bm->bm_w,
		bm->bm_h,
		bm->bm_rowsize);
}

extern "C" void Win32_FreeTexture (grs_bitmap *bm)
{
	CTexture *pTexture = (CTexture *) bm->pvSurface;
	if (pTexture != NULL)
	{
		g_setTextures.FreeTexture (pTexture);
		bm->pvSurface = NULL;
	}
}

extern "C" void Win32_SetTransparent (void *pvTexture, BOOL bTransparent)
{
	CTexture *pTexture = (CTexture *) pvTexture;
	if (pTexture != NULL)
	{
		pTexture->SetTransparent (bTransparent);
	}
}

