#ifndef DEBUG_H
#define DEBUG_H
/*
**-----------------------------------------------------------------------------
**  File:       Debug.h
**  Purpose:    Sample Debug code
**  Notes:
**
**  Copyright (c) 1995 - 1997 by Microsoft, all rights reserved
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
**  Include files
**-----------------------------------------------------------------------------
*/



/*
**-----------------------------------------------------------------------------
**  Defines
**-----------------------------------------------------------------------------
*/

#ifndef NDEBUG
// Note:  Define DEBUG_PROMPTME if you want MessageBox Error prompting
//		  This can get annoying quickly...
// #define DEBUG_PROMPTME

	// Pre and Post debug string info
//	#define START_STR	TEXT ("D3DDESCENT: ")
	#define START_STR	TEXT ("")
	#define END_STR		TEXT ("\r\n")
#endif // NDEBUG

// Debug Levels
#define DEBUG_ALWAYS	0L
#define DEBUG_CRITICAL	1L
#define DEBUG_ERROR		2L
#define DEBUG_MINOR		3L
#define DEBUG_WARN		4L
#define DEBUG_DETAILS	5L


// Sample Errors
#define APPERR_GENERIC			MAKE_DDHRESULT (10001)
#define	APPERR_INVALIDPARAMS	MAKE_DDHRESULT (10002)
#define APPERR_NOTINITIALIZED	MAKE_DDHRESULT (10003)
#define APPERR_OUTOFMEMORY		MAKE_DDHRESULT (10004)
#define APPERR_NOTFOUND			MAKE_DDHRESULT (10005)



/*
**-----------------------------------------------------------------------------
**  Macros
**-----------------------------------------------------------------------------
*/

#ifndef NDEBUG
    #define DPF dprintf
/*
    #define ASSERT(x) \
        if (! (x)) \
        { \
            DPF (DEBUG_ALWAYS, TEXT("Assertion violated: %s, File = %s, Line = #%ld\n"), \
                 TEXT(#x), TEXT(__FILE__), (DWORD)__LINE__ ); \
            abort (); \
        }
*/

   #define REPORTERR(x) \
       ReportDDError ((x), TEXT("%s(%ld)"), \
                      TEXT(__FILE__), (DWORD)__LINE__ );

   #define FATALERR(x) \
       ReportDDError ((x), TEXT("%s(%ld)"), \
                      TEXT(__FILE__), (DWORD)__LINE__ ); \
       OnPause (TRUE); \
       DestroyWindow (g_hWnd);
#else
   #define REPORTERR(x)
   #define DPF 1 ? (void)0 : (void)
/*
   #define ASSERT(x)
*/
   #define FATALERR(x) \
       OnPause (TRUE); \
       DestroyWindow (g_hWnd);
#endif // NDEBUG



/*
**-----------------------------------------------------------------------------
**  Global Variables
**-----------------------------------------------------------------------------
*/

// Debug Variables
#ifndef NDEBUG
	extern DWORD g_dwDebugLevel;
#endif

extern BOOL  g_fDebug;



/*
**-----------------------------------------------------------------------------
**  Function Prototypes
**-----------------------------------------------------------------------------
*/

// Debug Routines
#ifndef NDEBUG
	void __cdecl dprintf (DWORD dwDebugLevel, LPCTSTR szFormat, ...);
#endif //NDEBUG

void _cdecl ReportDDError (HRESULT hResult, LPCTSTR szFormat, ...);

#ifndef NDEBUG
struct CDebugTracker
{
	CDebugTracker (LPCSTR lpsz);
	~CDebugTracker ();

	void __cdecl Return (LPCSTR szFormat, ...);

protected:
	LPCSTR m_psz;
	BOOL m_bReturned;
	static ULONG s_cLevels;
	static char szsp [80];
};

#define TRACKER_ENTER(str)\
	CDebugTracker __tracker (str);
#else	// NDEBUG
#define TRACKER_ENTER(str) while(0);
#endif	// NDEBUG

/*
**-----------------------------------------------------------------------------
**  End of File
**-----------------------------------------------------------------------------
*/
#endif // End DEBUG_H


