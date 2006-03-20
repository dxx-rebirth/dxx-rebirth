#define D3D_OVERLOADS
#define WIN32_MEAN_AND_LEAN

#if 0
class _Lockit
{
public:
	_Lockit () {}
	~_Lockit () {}
};
#define _Lockit ___Lockit
#include <yvals.h>
#undef _Lockit
#endif


#include <windows.h>
#include <windowsx.h>
#include <ddraw.h>
#include <dinput.h>
#include <d3d.h>
#include <d3dtypes.h>
#include <dsound.h>

 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <tchar.h>
#include <assert.h>


#include <list>
#include <queue>
#include <set>
#include <map>

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include "resource.h"

#define ASSERT(x) _ASSERTE(x)

#pragma warning (disable: 4786)
