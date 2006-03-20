#define NONAMELESSUNION  // Because nameless unions are plentiful in DirectX
                         // and there is also one in objidl.h
#ifndef _stdcall
#define _stdcall STDCALL // Because objidl uses _stdcall
#endif
typedef HANDLE HTASK;    // Because it's missing...

#define GUID_DEFINED     // If it's not there...

// This is missing
#define MAKE_HRESULT(s,f,c) ((HRESULT)(((DWORD)(s)<<31)|((DWORD)(f)<<16)|((DWORD)(c))))

// This lot are just plain missing...
typedef GUID IID;
typedef IID *LPIID;
typedef IID *REFIID;
typedef CLSID *REFCLSID;
typedef GUID *REFGUID;
typedef struct _RPC_VERSION { WORD MajorVersion; WORD MinorVersion; } RPC_VERSION;
typedef struct _RPC_SYNTAX_IDENTIFIER { GUID SyntaxGUID;
	RPC_VERSION SyntaxVersion;
} RPC_SYNTAX_IDENTIFIER, * PRPC_SYNTAX_IDENTIFIER;
typedef struct _RPC_MESSAGE {
	HANDLE Handle;
	unsigned long DataRepresentation;
	void * Buffer;
	unsigned int BufferLength;
	unsigned int ProcNum;
	PRPC_SYNTAX_IDENTIFIER TransferSyntax;
	void * RpcInterfaceInformation;
	void * ReservedForRuntime;
	void * ManagerEpv;
	void * ImportContext;
	unsigned long RpcFlags;
} RPC_MESSAGE, * PRPC_MESSAGE;
