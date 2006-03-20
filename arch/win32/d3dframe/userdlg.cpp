//-----------------------------------------------------------------------------
// File: UserDlg.cpp
//
// Desc: Support functions for the user select driver dialog.

//       Note: Since this code is destined for a static link library, the
//       dialog dox is constructed manually. Otherwise, we would use a resource
//       and save many lines of code. Manually constructing dialog boxes is not
//       trivial and there are many issues (unicode, dword alignment, etc.)
//       involved.
//
//
// Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include <tchar.h>
#include "D3DEnum.h"
#include <stdio.h>
#include "d3dutil.h"



//-----------------------------------------------------------------------------
// Constants and function prototypes for the user select driver dialog
//-----------------------------------------------------------------------------
#define IDC_STATIC       0xffff
#define IDC_DRIVER_COMBO   1000
#define IDC_DEVICE_COMBO   1001
#define IDC_MODE_COMBO     1002




//-----------------------------------------------------------------------------
// External variables (declared in the D3DEnum.cpp file)
//-----------------------------------------------------------------------------
extern D3DEnum_DriverInfo* g_pCurrentDriver;     // The selected DD driver




//-----------------------------------------------------------------------------
// Name:  CopyToWideChar
//-----------------------------------------------------------------------------
static VOID CopyToWideChar( WCHAR** pstrOut, LPTSTR strIn )
{
    DWORD  dwLen  = lstrlen( strIn );
    WCHAR* strOut = *pstrOut;

#ifdef UNICODE // Copy Unicode to Unicode
    _wcsncpy( strOut, strIn, dwLen );
    strOut[dwLen] = L'\0';
#else         // Copy Ansi to Unicode
    dwLen = MultiByteToWideChar( CP_ACP, 0, strIn, dwLen, strOut, dwLen );
    strOut[dwLen++] = L'\0'; // Add the null terminator
#endif
    *pstrOut += dwLen;
}




//-----------------------------------------------------------------------------
// Name: AddDialogControl()
// Desc: Internal function to help build the user select dialog
//-----------------------------------------------------------------------------
static VOID AddDialogControl( WORD** pp, DWORD dwStyle, SHORT x, SHORT y,
                              SHORT cx, SHORT cy, WORD id, 
                              LPTSTR strClassName, LPTSTR strTitle )
{
    // DWORD align the current ptr
    DLGITEMTEMPLATE* p = (DLGITEMTEMPLATE*)(((((ULONG)(*pp))+3)>>2)<<2);

    p->style           = dwStyle | WS_CHILD | WS_VISIBLE;
    p->dwExtendedStyle = 0L;
    p->x               = x;
    p->y               = y;
    p->cx              = cx;
    p->cy              = cy;
    p->id              = id;

    *pp = (WORD*)(++p); // Advance ptr

    CopyToWideChar( (WCHAR**)pp, strClassName ); // Set Class name
    CopyToWideChar( (WCHAR**)pp, strTitle );     // Set Title

    (*pp)++;          // Skip Extra Stuff
}




//-----------------------------------------------------------------------------
// Name: _BuildUserSelectDialog()
// Desc: Internal function to build the user select dialog
//-----------------------------------------------------------------------------
DLGTEMPLATE* _BuildDriverSelectTemplate()
{
    // Allocate ample memory for building the template
    DLGTEMPLATE* pDlgTemplate = new DLGTEMPLATE[50];
    if( NULL == pDlgTemplate )
        return NULL;
    ZeroMemory( pDlgTemplate, 50*sizeof(DLGTEMPLATE) );
    
    // Fill in the DLGTEMPLATE info
    DLGTEMPLATE* pdt     = pDlgTemplate;
    pdt->style           = DS_MODALFRAME | DS_NOIDLEMSG | DS_SETFOREGROUND |
                           DS_3DLOOK | DS_CENTER | WS_POPUP | WS_VISIBLE |
                           WS_CAPTION | WS_SYSMENU | DS_SETFONT;
    pdt->dwExtendedStyle = 0L;
    pdt->cdit            = 8;
    pdt->x               = 0;
    pdt->y               = 0;
    pdt->cx              = 179;
    pdt->cy              = 84;

    // Add menu array, class array, dlg title, font size and font name
    WORD* pw = (WORD*)(++pdt);
    *pw++ = L'\0';                               // Set Menu array to nothing
    *pw++ = L'\0';                               // Set Class array to nothing
    CopyToWideChar( (WCHAR**)&pw, TEXT( "Select New Direct3D Driver" ) ); // Dlg title
    *pw++ = 8;                                   // Font Size
    CopyToWideChar( (WCHAR**)&pw, TEXT("Arial") );         // Font Name
 
    // Add the okay button
    AddDialogControl( &pw, BS_PUSHBUTTON | WS_TABSTOP, 65, 65, 51, 14, 
                      IDOK, TEXT("BUTTON"), TEXT("OK") );

    // Add the cancel button
    AddDialogControl( &pw, BS_PUSHBUTTON | WS_TABSTOP, 123, 65, 51, 14, 
                      IDCANCEL, TEXT("BUTTON"), TEXT("Cancel") );

    // Add the driver select combobox
    AddDialogControl( &pw, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 
                      36, 5, 138, 45, 
                      IDC_DRIVER_COMBO, TEXT("COMBOBOX"), TEXT("") );

    // Add the device select combobox
    AddDialogControl( &pw, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 
                      36, 24, 138, 45,
                      IDC_DEVICE_COMBO, TEXT("COMBOBOX"), TEXT("") );

    // Add the mode select combobox
    AddDialogControl( &pw, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                      36, 44, 138, 45, 
                      IDC_MODE_COMBO, TEXT("COMBOBOX"), TEXT("") );

    // Add the "Driver:" text
    AddDialogControl( &pw, SS_LEFT, 5, 5, 22, 13, 
                      IDC_STATIC, TEXT("STATIC"), TEXT("Driver:") );

    // Add the "Device:" text
    AddDialogControl( &pw, SS_LEFT, 5, 24, 25, 13,
                      IDC_STATIC, TEXT("STATIC"), TEXT("Device:") );

    // Add the "Mode:" text
    AddDialogControl( &pw, SS_LEFT, 5, 44, 22, 13, 
                      IDC_STATIC, TEXT("STATIC"), TEXT("Mode:") );

    return pDlgTemplate;
}




//-----------------------------------------------------------------------------
// Name: UpdateComboBoxesContent()
// Desc: Builds the list of drivers, devices and modes for the combo boxes
//       in the driver select dialog box.
//-----------------------------------------------------------------------------
static VOID UpdateComboBoxesContent( HWND hDlg, 
							         D3DEnum_DriverInfo** ppCurrentDriver, 
                                     D3DEnum_DeviceInfo** ppCurrentDevice, 
                                     D3DEnum_ModeInfo** ppCurrentMode,
									 BOOL bWindowed )
{
    // Check the parameters
    if( (NULL==ppCurrentDriver) || (NULL==ppCurrentDevice) || 
        (NULL==ppCurrentMode) )
        return;

    // If the specified driver is NULL, use the first one in the list
    if( NULL == *ppCurrentDriver) 
        (*ppCurrentDriver) = D3DEnum_GetFirstDriver();

    // If the specified device is NULL, use the first one in the list
    if( NULL == *ppCurrentDevice) 
        (*ppCurrentDevice) = (*ppCurrentDriver)->pFirstDevice;
    
    // Reset the content in each of the combo boxes
    SendDlgItemMessage( hDlg, IDC_DRIVER_COMBO, CB_RESETCONTENT, 0, 0 );
    SendDlgItemMessage( hDlg, IDC_DEVICE_COMBO, CB_RESETCONTENT, 0, 0 );
    SendDlgItemMessage( hDlg, IDC_MODE_COMBO,   CB_RESETCONTENT, 0, 0 );

    // Build the list of drivers for the combo box
    for( D3DEnum_DriverInfo* pDriver = D3DEnum_GetFirstDriver(); pDriver; 
         pDriver = pDriver->pNext )
    {
        // Add driver desc to the combo box
        DWORD dwItem = SendDlgItemMessage( hDlg, IDC_DRIVER_COMBO, 
                           CB_ADDSTRING, 0, (LPARAM)pDriver->strDesc );

        // Associate DriverInfo ptr with the item in the combo box
        SendDlgItemMessage( hDlg, IDC_DRIVER_COMBO, CB_SETITEMDATA, 
                            (WPARAM)dwItem, (LPARAM)pDriver );

        // If this is the current driver, set is as the current selection
        if( pDriver == (*ppCurrentDriver) )
            SendDlgItemMessage( hDlg, IDC_DRIVER_COMBO, CB_SETCURSEL,
                                (WPARAM)dwItem, 0L );
    }

    // Build the list of devices for the combo box
    for( D3DEnum_DeviceInfo* pDevice = (*ppCurrentDriver)->pFirstDevice; pDevice;
         pDevice = pDevice->pNext )
    {
        // Add device name to the combo box
        DWORD dwItem = SendDlgItemMessage( hDlg, IDC_DEVICE_COMBO, 
                           CB_ADDSTRING, 0, (LPARAM)pDevice->strName );

        // Associate DeviceInfo ptr with the item in the combo box
        SendDlgItemMessage( hDlg, IDC_DEVICE_COMBO, CB_SETITEMDATA,
                            (WPARAM)dwItem, (LPARAM)pDevice );

        // If this is the current device, set this as the current selection
        if( pDevice == (*ppCurrentDevice) )
            SendDlgItemMessage( hDlg, IDC_DEVICE_COMBO, CB_SETCURSEL, 
                                (WPARAM)dwItem, 0L );
    }

	if( (*ppCurrentDriver)->ddDriverCaps.dwCaps2 & DDCAPS2_CANRENDERWINDOWED 
        && ( NULL == (*ppCurrentDriver)->pGUID )
		&& (*ppCurrentDevice)->bCompatbileWithDesktop )
	{
		// Add windowed mode to the combo box of available modes
        DWORD dwItem = SendDlgItemMessage( hDlg, IDC_MODE_COMBO, 
                           CB_ADDSTRING, 0, (LPARAM)TEXT("Windowed mode") );
        
		// Associate ModeInfo ptr with the item in the combo box
        SendDlgItemMessage( hDlg, IDC_MODE_COMBO, CB_SETITEMDATA, 
                            (WPARAM)dwItem, (LPARAM)NULL );

        // If the current device is windowed, set this as current
        if( bWindowed )
            SendDlgItemMessage( hDlg, IDC_MODE_COMBO, CB_SETCURSEL, 
                                (WPARAM)dwItem, 0L );
	}

    // Build the list of modes for the combo box
    for( D3DEnum_ModeInfo* pMode = (*ppCurrentDevice)->pFirstMode; pMode; 
         pMode = pMode->pNext )
    {
        // Add mode desc to the combo box
        DWORD dwItem = SendDlgItemMessage( hDlg, IDC_MODE_COMBO, 
                                           CB_ADDSTRING, 0,
                                           (LPARAM)pMode->strDesc );

        // Associate ModeInfo ptr with the item in the combo box
        SendDlgItemMessage( hDlg, IDC_MODE_COMBO, CB_SETITEMDATA, 
                            (WPARAM)dwItem, (LPARAM)pMode );

        // If this is the current mode, set is as the current selection
        if( !bWindowed && ( pMode == (*ppCurrentMode) ) )
				SendDlgItemMessage( hDlg, IDC_MODE_COMBO, CB_SETCURSEL, 
									(WPARAM)dwItem, 0L );
    }   
}




//-----------------------------------------------------------------------------
// Name: _DriverSelectProc()
// Desc: Windows message handling function for the driver select dialog
//-----------------------------------------------------------------------------
BOOL CALLBACK _DriverSelectProc( HWND hDlg, UINT uiMsg, WPARAM wParam, 
                                 LPARAM lParam )
{
    static D3DEnum_DriverInfo *pOldDriver,  *pNewDriver;
    static D3DEnum_DeviceInfo *pOldDevice,  *pNewDevice;
    static D3DEnum_ModeInfo   *pOldMode,    *pNewMode;
    static BOOL                bOldWindowed, bNewWindowed;

    // Handle the initialization message
    if( WM_INITDIALOG == uiMsg )
    {
        // Setup temp storage pointers for dialog
        pNewDriver   = pOldDriver   = g_pCurrentDriver;
        pNewDevice   = pOldDevice   = pOldDriver->pCurrentDevice;
        pNewMode     = pOldMode     = pOldDevice->pCurrentMode;
		bNewWindowed = bOldWindowed = pOldDevice->bWindowed;

        UpdateComboBoxesContent( hDlg, &pOldDriver, &pOldDevice, 
                                 &pOldMode, bOldWindowed );
        return TRUE;
    }
    
    if( WM_COMMAND == uiMsg )
    {
        // Handle the case when the user hits the OK button
        if( IDOK == LOWORD(wParam) )
        {
            // Check if any of the options were changed
            if( ( pOldDriver != pNewDriver ) || ( pOldDevice != pNewDevice ) ||
                ( pOldMode != pNewMode ) || ( bOldWindowed != bNewWindowed ) )
            {
				// Set actual ptrs from the temp ptrs used for the dialog
                g_pCurrentDriver = pNewDriver;
                g_pCurrentDriver->pCurrentDevice = pNewDevice;
                g_pCurrentDriver->pCurrentDevice->pCurrentMode = pNewMode;
                g_pCurrentDriver->pCurrentDevice->bWindowed    = bNewWindowed;
 
                EndDialog( hDlg, IDOK );
                return TRUE;
            }
            else
            {
                EndDialog( hDlg, IDCANCEL );
                return TRUE;
            }
        }

        // Handle the case when the user hits the Cancel button
        else if( IDCANCEL == LOWORD(wParam) )
        {
            EndDialog( hDlg, IDCANCEL );
            return TRUE;
        }

        // Handle the case when the user chooses an item in the combo boxes.
        else if( CBN_SELENDOK == HIWORD(wParam) )
        {
            DWORD dwIndex    = SendMessage( (HWND)lParam, CB_GETCURSEL, 
                                            0, 0 );
            LONG  pNewObject = SendMessage( (HWND)lParam, CB_GETITEMDATA,
                                            dwIndex, 0 );
            
            if( (CB_ERR==dwIndex) )
                return TRUE;

            // Handle the case where one of these may have changed. The
            // combo boxes will need to be updated to reflect the changes.
            switch( LOWORD( wParam ) )
            {
                case IDC_DRIVER_COMBO:
                    if( pNewObject && pNewObject != (LONG)pNewDriver )
                    {
                        pNewDriver   = (D3DEnum_DriverInfo*)pNewObject;
                        pNewDevice   = pNewDriver->pCurrentDevice;;
                        pNewMode     = pNewDevice->pCurrentMode;
						bNewWindowed = pNewDevice->bWindowed;
                    }
                    break;
                case IDC_DEVICE_COMBO:
                    if( pNewObject && pNewObject != (LONG)pNewDevice )
					{
                        pNewDevice   = (D3DEnum_DeviceInfo*)pNewObject;
                        pNewMode     = pNewDevice->pCurrentMode;
						bNewWindowed = pNewDevice->bWindowed;
					}
                    break;
                case IDC_MODE_COMBO:
					if( pNewObject )
					{
						pNewMode = (D3DEnum_ModeInfo*)pNewObject;
						bNewWindowed = FALSE;
					}
					else
						bNewWindowed = TRUE;
                    break;
            }

            UpdateComboBoxesContent( hDlg, &pNewDriver, &pNewDevice,
                                     &pNewMode, bNewWindowed );

            return TRUE;
        }
    }
    return FALSE;
}



