#ifndef __TEXTURE_H__
#define __TEXTURE_H__

struct CPaletteInfo
{
	HRESULT Initialize ();
	HRESULT Uninitialize ();

	LONG GetShift (DWORD dwMask)
	{
		ULONG cshft = 0;
		while (dwMask)
		{
			cshft += 1;
			dwMask >>= 1;
		}
		return cshft;
	}

	DWORD ShiftLeft (DWORD dwVal, LONG lShift)
	{
		if (lShift > 0)
			return dwVal << lShift;
		else
			return dwVal >> (-lShift);
	}

	WORD Read16 (BYTE b)
	{
		DWORD dwPal = *(DWORD*) &m_rgpe [b];
		WORD w;
		
		w  = ShiftLeft (dwPal, m_cshftR -  8) & m_mskR;
		w |= ShiftLeft (dwPal, m_cshftG - 16) & m_mskG;
		w |= ShiftLeft (dwPal, m_cshftB - 24) & m_mskB;

		return w;
	}

	BOOL IsPow2 () const { return m_bPow2; }
	BOOL IsSquare () const { return m_bSquare; }

	void GetPixelFormat (DDPIXELFORMAT *pddpf) { *pddpf = m_ddpfPixelFormat; }
	BOOL IsIndexed () const { return m_ddpfPixelFormat.dwRGBBitCount == 8; }
	void SetPaletteEntries (PALETTEENTRY rgpe [256]);
	void GetPaletteEntries (PALETTEENTRY rgpe [256]);

	LPDIRECTDRAWPALETTE GetPalette () { return m_spddpPalette; }
	PALETTEENTRY ReadPalette (BYTE b) const { return m_rgpe [b]; }

	DWORD m_mskR, m_mskG, m_mskB;
	ULONG m_cshftR, m_cshftG, m_cshftB;
	DDPIXELFORMAT m_ddpfPixelFormat;
	BOOL m_bPow2, m_bSquare;
	PALETTEENTRY m_rgpe [256];
	CComPtr <IDirectDrawPalette> m_spddpPalette;
};

struct CTexture
{
	HRESULT Initialize (BYTE *rgbSource, ULONG ulWidth, ULONG ulHeight, ULONG ulPitch);

	HRESULT Allocate (CPaletteInfo *ppi);
	HRESULT Free (void);

	HRESULT Lock (void);
	HRESULT Unlock (void);
	void PlotPixel (ULONG ulX, ULONG ulY, BYTE b);
	void SetBitmapData (BYTE *rgb, BOOL bRle);
	void CopyFromSource (void);

	void DirtyMemory (void) { m_bDirtyMemory = TRUE; }
	BOOL isDirtyMemory (void) const { return m_bDirtyMemory; }
	HRESULT CleanMemory (void);

	void SetTransparent (BOOL bTransparent) { m_bTransparent = bTransparent; }
	BOOL IsTransparent (void) const { return m_bTransparent; }

	ULONG GetPixelCount () const { return m_ulHeight * m_ulWidth; }
	void GetBitmapAdj (D3DVALUE *pflAdjX, D3DVALUE *pflAdjY)
	{
		*pflAdjX = m_flAdjX;
		*pflAdjY = m_flAdjY;
	}

	IDirectDrawSurface4 *GetSurface () { return m_spddsMemory; }
	IDirect3DTexture2 *GetTexture ();

	ULONG m_ulWidth;
	ULONG m_ulHeight;
	ULONG m_ulWidthSource;
	ULONG m_ulHeightSource;

protected:
	CComPtr <IDirectDrawSurface4> m_spddsMemory;
	CComQIPtr <IDirect3DTexture2, &IID_IDirect3DTexture2> m_spddtMemory;
	ULONG m_ulPitchSource;
	ULONG m_ulAge;
	BYTE *m_rgbSource;

	BOOL m_bDirtyMemory;
	BOOL m_bTransparent;
	BOOL m_bRle;
	D3DVALUE m_flAdjX, m_flAdjY;

public:
	static CPaletteInfo *m_ppi;
	static DDSURFACEDESC2 s_ddsd;
	static BOOL s_bLocked;
};

struct CTextureSet
{
	HRESULT Initialize ();
	HRESULT Uninitialize ();

	CTexture *CreateTexture (BYTE *rgbSource, ULONG dx, ULONG dy, ULONG ulPitch);
	void FreeTexture (CTexture *pTexture);
	void SetPaletteEntries (PALETTEENTRY rgpe [256]);
	void GetPaletteEntries (PALETTEENTRY rgpe [256]);
	void DirtyTextures (void);

	PALETTEENTRY ReadPalette (BYTE b) { return m_pi.ReadPalette (b); }
	BOOL HasPalette () { return m_pi.IsIndexed (); }

protected:
	typedef std::set <CTexture *> TEXTURE_SET;
	TEXTURE_SET m_setTextures;
	CPaletteInfo m_pi;
};



#endif	// __TEXTURE_H__
