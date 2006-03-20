#include "pch.h"
#include "texture.h"

#include "d3dframe.h"
extern CD3DFramework* g_pFramework;

////////////////////////////////////////////////////////////////////////////

BOOL CTexture::s_bLocked = FALSE;
DDSURFACEDESC2 CTexture::s_ddsd;


HRESULT CTexture::Initialize (BYTE *rgbSource, ULONG ulWidth, ULONG ulHeight, ULONG ulPitch)
{
	m_rgbSource = rgbSource;
	m_ulWidthSource = ulWidth;
	m_ulHeightSource = ulHeight;
	m_ulPitchSource = ulPitch;

	return S_OK;
}

HRESULT CTexture::Allocate (CPaletteInfo *ppi)
{
	HRESULT hr;

	ASSERT (m_spddsMemory == NULL);

	if (ppi->IsPow2 ())
	{
		m_ulWidth = 1;
		while (m_ulWidth < m_ulWidthSource)
			m_ulWidth *= 2;
		
		m_ulHeight = 1;
		while (m_ulHeight < m_ulHeightSource)
			m_ulHeight *= 2;
	}
	else
	{
		m_ulWidth = m_ulWidthSource;
		m_ulHeight = m_ulHeightSource;
	}

	if (ppi->IsSquare ())
	{
		if (m_ulHeight > m_ulWidth)
		{
			m_ulWidth = m_ulHeight;
		}
		else
		{
			m_ulHeight = m_ulWidth;
		}
	}

	m_flAdjX = (D3DVALUE) m_ulWidthSource / (D3DVALUE) m_ulWidth;
	m_flAdjY = (D3DVALUE) m_ulHeightSource / (D3DVALUE) m_ulHeight;

	DDSURFACEDESC2 ddsd;
	ZeroMemory (&ddsd, sizeof (ddsd));
	ddsd.dwSize = sizeof (ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd.dwWidth = m_ulWidth;
	ddsd.dwHeight = m_ulHeight;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
	ppi->GetPixelFormat (&ddsd.ddpfPixelFormat);

	hr = g_pFramework->GetDirectDraw ()->CreateSurface(&ddsd, &m_spddsMemory, NULL);
	ASSERT (SUCCEEDED (hr));
	if (FAILED (hr))
		return hr;

	ASSERT (m_spddsMemory != NULL);

	m_spddtMemory = m_spddsMemory;

	DirtyMemory ();

	DDCOLORKEY ddck;
	if (ppi->IsIndexed ())
	{
		ddck.dwColorSpaceLowValue = 255;
		ddck.dwColorSpaceHighValue = 255;
	}
	else
	{
		ddck.dwColorSpaceLowValue = 0;
		ddck.dwColorSpaceHighValue = 0;
	}
	hr = m_spddsMemory->SetColorKey (DDCKEY_SRCBLT, &ddck);
	ASSERT (SUCCEEDED (hr));

	if (ppi->GetPalette () != NULL)
	{
		hr = m_spddsMemory->SetPalette (ppi->GetPalette ());
		ASSERT (SUCCEEDED (hr));
	}

	return hr;
}

HRESULT CTexture::Free (void)
{
	HRESULT hr = S_OK;

	m_spddtMemory = m_spddsMemory = NULL;

	m_ulWidth = 0;
	m_ulHeight = 0;

	return hr;
}

HRESULT CTexture::Lock (void)
{
	HRESULT hr;
	if (!s_bLocked)
	{
		s_bLocked = TRUE;

		s_ddsd.dwSize = sizeof (s_ddsd);
		hr = m_spddsMemory->Lock (
			NULL,
			&s_ddsd,
#ifndef NDEBUG
			DDLOCK_NOSYSLOCK |
#endif
			DDLOCK_WAIT,
			NULL);
		ASSERT (SUCCEEDED (hr));
	}
	else
	{
		// already locked
		hr = E_FAIL;
		ASSERT (FALSE);
	}
	return hr;
}

HRESULT CTexture::Unlock (void)
{
	HRESULT hr;
	if (s_bLocked)
	{
		hr = m_spddsMemory->Unlock (NULL);
		ASSERT (SUCCEEDED (hr));

		s_bLocked = FALSE;
	}
	else
	{
		// not locked
		hr = E_FAIL;
		ASSERT (FALSE);
	}
	return hr;
}

void CTexture::PlotPixel (ULONG ulX, ULONG ulY, BYTE b)
{
	ASSERT (ulX <= m_ulWidth);
	ASSERT (ulY <= m_ulHeight);

	ASSERT (s_bLocked);
	ASSERT (s_ddsd.lpSurface != NULL);

	switch (s_ddsd.ddpfPixelFormat.dwRGBBitCount)
	{
		case 8:
		{
			* (((LPBYTE) s_ddsd.lpSurface) + ulX + ulY * s_ddsd.lPitch) = b;

			break;
		}
		case 16:
		{
			WORD w;
			if (!(m_bTransparent && b == 255))
			{
				w = m_ppi->Read16 (b);
			}
			else
			{
				w = 0;
			}
			* (WORD *) (((LPBYTE) s_ddsd.lpSurface) + ulX * 2 + ulY * s_ddsd.lPitch) = w;
			break;
		}
		
		default:
		{
			ASSERT (FALSE);
			break;
		}
	}
}

void CTexture::SetBitmapData (BYTE *rgb, BOOL bRle)
{
	m_rgbSource = rgb;
	m_bRle = bRle;
	DirtyMemory ();
}

void CTexture::CopyFromSource (void)
{
	if (SUCCEEDED (Lock ()))
	{
		if (m_bRle)
		{
			PBYTE pbLine = m_rgbSource + m_ulHeightSource + 4;;
			for (ULONG y = 0; y < m_ulHeightSource; y ++)
			{
				ULONG x = 0;
				PBYTE pbSrc = pbLine;

				while (1)
				{
					const BYTE RLE_CODE = 0xE0;
					BYTE b = *pbSrc++;
					if ((b & RLE_CODE) != RLE_CODE)
					{
						PlotPixel (x++, y, b);
					}
					else
					{
						BYTE cb = b & (~RLE_CODE);
						if (cb == 0)
							break;

						b = *pbSrc++;
						while (cb--)
							PlotPixel (x++, y, b);
					}
				}
				ASSERT (x <= m_ulWidthSource);

				pbLine += m_rgbSource [y + 4];
			}
		}
		else
		{
			LPBYTE lpbDest = (LPBYTE) s_ddsd.lpSurface;
			LPBYTE lpbSrc = m_rgbSource;

			switch (s_ddsd.ddpfPixelFormat.dwRGBBitCount)
			{
				case 8:
				{
					for (ULONG y = m_ulHeightSource; y != 0; y--)
					{
						memcpy (lpbDest, lpbSrc, m_ulPitchSource);
						lpbDest += s_ddsd.lPitch;
						lpbSrc += m_ulPitchSource;
					}
					break;
				}
				case 16:
				{
					for (ULONG y = m_ulHeightSource; y != 0; y--)
					{
						LPBYTE lpbLineSrc = lpbSrc;
						LPWORD lpwLineDest = (LPWORD) lpbDest;

						if (m_bTransparent)
						{
							for (ULONG x = m_ulWidthSource; x != 0; x--)
							{
								BYTE b = *lpbLineSrc++;
								*lpwLineDest++ = (b == 255) ? 0 : m_ppi->Read16 (b);
							}
						}
						else
						{
							for (ULONG x = m_ulWidthSource; x != 0; x--)
							{
								*lpwLineDest++ = m_ppi->Read16 (*lpbLineSrc++);
							}
						}
						lpbDest += s_ddsd.lPitch;
						lpbSrc += m_ulPitchSource;
					}
					break;
				}
				
				default:
				{
					ASSERT (FALSE);
					break;
				}
			}
		}
		Unlock ();
	}
	else
	{
		ASSERT (FALSE);
	}
}

HRESULT CTexture::CleanMemory (void)
{
	CopyFromSource ();
	m_bDirtyMemory = FALSE;
	return S_OK;
}

IDirect3DTexture2 *CTexture::GetTexture ()
{
	if (m_bDirtyMemory)
	{
		CleanMemory ();
	}

	return m_spddtMemory;
}

////////////////////////////////////////////////////////////////////////////

struct FindTextureData
{
    DWORD           bpp;        // we want a texture format of this bpp
    DDPIXELFORMAT   ddpf;       // place the format here
};


HRESULT CALLBACK FindTextureCallback(LPDDPIXELFORMAT pddpf, LPVOID lParam)
{
    FindTextureData * FindData = (FindTextureData *)lParam;
	
    //
    // we use GetDC/BitBlt to init textures so we only
    // want to use formats that GetDC will support.
    //
    if (pddpf->dwFlags & (DDPF_ALPHA|DDPF_ALPHAPIXELS))
        return DDENUMRET_OK;

/*
    if (pddpf->dwRGBBitCount == 16)
    {
        FindData->ddpf = ddpf;
    }
	
    return DDENUMRET_OK;
*/

	
//	if (!(pddpf->dwFlags & DDPF_ALPHAPIXELS))
//		return DDENUMRET_OK;

    if (pddpf->dwRGBBitCount < 8)
        return DDENUMRET_OK;

    if (pddpf->dwRGBBitCount == 8 && !(pddpf->dwFlags & DDPF_PALETTEINDEXED8))
        return DDENUMRET_OK;
	
    if (pddpf->dwRGBBitCount > 8 && !(pddpf->dwFlags & DDPF_RGB))
        return DDENUMRET_OK;

    //
    // keep the texture format that is nearest to the bitmap we have
    //
    if (FindData->ddpf.dwRGBBitCount == 0 ||
		(pddpf->dwRGBBitCount >= FindData->bpp && pddpf->dwRGBBitCount <= FindData->ddpf.dwRGBBitCount) //&&
		//(pddpf->dwAlphaBitDepth != 0 && pddpf->dwAlphaBitDepth > FindData->ddpf.dwAlphaBitDepth) // ALPHA BIT DEPTHS ARE REVERSED!!!!
		)
    {
        FindData->ddpf = *pddpf;
    }
	
    return DDENUMRET_OK;
}

void ChooseTextureFormat(LPDIRECT3DDEVICE3 Device, DWORD bpp, DDPIXELFORMAT *pddpf)
{
	HRESULT hr;
    FindTextureData FindData;
	ZeroMemory(&FindData, sizeof(FindData));
    FindData.bpp = bpp;
	hr = Device->EnumTextureFormats(FindTextureCallback, (LPVOID)&FindData);
    ASSERT(hr == S_OK);
    *pddpf = FindData.ddpf;
}

HRESULT CPaletteInfo::Initialize ()
{
	m_spddpPalette = NULL;

	//
	// find the best texture format to use.
	//
	ChooseTextureFormat (g_pFramework->GetD3DDevice (), 8, &m_ddpfPixelFormat);
	
	m_cshftR = GetShift (m_mskR = m_ddpfPixelFormat.dwRBitMask);
	m_cshftG = GetShift (m_mskG = m_ddpfPixelFormat.dwGBitMask);
	m_cshftB = GetShift (m_mskB = m_ddpfPixelFormat.dwBBitMask);

	HRESULT hr;

	D3DDEVICEDESC descHAL, descHEL;
	descHAL.dwSize = descHEL.dwSize = sizeof (descHAL);
	hr = g_pFramework->GetD3DDevice ()->GetCaps (&descHAL, &descHEL);
	ASSERT (SUCCEEDED (hr));

	if (!descHAL.dwFlags) {
		m_bPow2 = (descHEL.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) != 0;
		m_bSquare = (descHEL.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0;
	} else {
		m_bPow2 = (descHAL.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) != 0;
		m_bSquare = (descHAL.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0;
	}

	if (IsIndexed ())
	{
		hr = g_pFramework->GetDirectDraw ()->CreatePalette (DDPCAPS_8BIT | DDPCAPS_ALLOW256, m_rgpe, &m_spddpPalette, NULL);
		ASSERT (SUCCEEDED (hr));
	}

	return S_OK;
}

HRESULT CPaletteInfo::Uninitialize ()
{
	m_spddpPalette = NULL;	// smart pointer release

	return S_OK;
}

void CPaletteInfo::SetPaletteEntries (PALETTEENTRY rgpe [256])
{
	// okay, we don't really need to cache the palette values
	// we could implement ReadPalette by individual Getentries calls...
	memcpy (m_rgpe, rgpe, sizeof (m_rgpe));

	if (m_spddpPalette != NULL)
	{
		HRESULT hr = m_spddpPalette->SetEntries (0, 0, 256,	m_rgpe);
		ASSERT (SUCCEEDED (hr));
	}
}

void CPaletteInfo::GetPaletteEntries (PALETTEENTRY rgpe [256])
{
	if (m_spddpPalette != NULL)
	{
		HRESULT hr = m_spddpPalette->GetEntries (0, 0, 256,	m_rgpe);
		ASSERT (SUCCEEDED (hr));
	}
}

////////////////////////////////////////////////////////////////
CPaletteInfo *CTexture::m_ppi;

HRESULT CTextureSet::Initialize ()
{
	HRESULT hr;
		
	hr = m_pi.Initialize ();
	ASSERT (SUCCEEDED (hr));

	CTexture::m_ppi = &m_pi;

	for (TEXTURE_SET::iterator iter = m_setTextures.begin ();
		iter != m_setTextures.end ();
		iter ++)
	{
		(*iter)->Allocate (&m_pi);
	}

	return hr;
}

HRESULT CTextureSet::Uninitialize ()
{
	for (TEXTURE_SET::iterator iter = m_setTextures.begin ();
		iter != m_setTextures.end ();
		iter ++)
	{
		(*iter)->Free ();
	}

	m_pi.Uninitialize ();

	return S_OK;
}

CTexture *CTextureSet::CreateTexture (BYTE *rgbSource, ULONG dx, ULONG dy, ULONG ulPitch)
{
	CTexture *pTexture = new CTexture;

	pTexture->Initialize (rgbSource, dx, dy, ulPitch);
	pTexture->Allocate (&m_pi);

	m_setTextures.insert (pTexture);

	return pTexture;
}

void CTextureSet::FreeTexture (CTexture *pTexture)
{
	m_setTextures.erase (pTexture);

	delete pTexture;
}

void CTextureSet::DirtyTextures (void)
{
	for (TEXTURE_SET::iterator iter = m_setTextures.begin ();
		iter != m_setTextures.end ();
		iter ++)
	{
		(*iter)->DirtyMemory ();
	}
}


void CTextureSet::SetPaletteEntries (PALETTEENTRY rgpe [256])
{
	m_pi.SetPaletteEntries (rgpe);

	if (!m_pi.IsIndexed ())
	{
		DirtyTextures ();
	}
}

void CTextureSet::GetPaletteEntries (PALETTEENTRY rgpe [256])
{
	m_pi.GetPaletteEntries (rgpe);
}
