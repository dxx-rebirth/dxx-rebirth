/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/


#pragma off (unreferenced)
static char rcsid[] = "$Id: winregs.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)

#define WIN95
#define _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

#include "error.h"
#include "mono.h"
#include "winregs.h"


static HKEY hRootKey=HKEY_LOCAL_MACHINE;


void registry_setpath(HKEY hKey)
{
	hRootKey = HKEY_LOCAL_MACHINE;
}
		


registry_handle *registry_open(const char *path)
{
	char *regtoken;
	int keys;
	int i;
	char *regpath;
	long lres;
	registry_handle *handle;

//	Find number of keys 
	regpath = (char *)malloc(strlen(path)+1);
	if (!regpath) return NULL;

	strcpy(regpath, path);

	i = 0;
	regtoken = strtok(regpath, "\\");
	if (!regtoken) { 
		free(regpath);	
		return NULL;
	}

	while (regtoken)
	{
		i++;
		regtoken = strtok(NULL, "\\");
	}

	keys = i;
	handle = (registry_handle*)malloc(sizeof(registry_handle));
	handle->keys = keys;
	handle->hKey = (HKEY *)malloc(sizeof(HKEY)*keys);
	if (!handle->hKey) {
		free(handle);
		return NULL;
	}

	memset(handle->hKey, 0, sizeof(HKEY)*keys);

//	Create keys
	i = 0;
	strcpy(regpath, path);
	regtoken = strtok(regpath, "\\");

	if (regtoken) mprintf((0, "%s\\", regtoken));

	lres = RegOpenKeyEx(hRootKey, regtoken, 0, KEY_EXECUTE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &handle->hKey[i]);
	if (lres != ERROR_SUCCESS) 
		goto RegistryOpen_Cleanup;
	
	regtoken = strtok(NULL, "\\");
	i++;
	if (regtoken) mprintf((0, "%s\\", regtoken));


	while (regtoken)
	{
		lres = RegOpenKeyEx(handle->hKey[i-1], regtoken, 0, KEY_EXECUTE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &handle->hKey[i]);
		if (lres != ERROR_SUCCESS) 
			goto RegistryOpen_Cleanup;

		regtoken = strtok(NULL, "\\");
		if (regtoken) mprintf((0, "%s\\", regtoken));
		i++;
	}

	free(regpath);

	mprintf((0, "\n"));

	return handle;

RegistryOpen_Cleanup:
	for (i = handle->keys-1; i >=0; i--)
		if (handle->hKey[i])  {
			RegCloseKey(handle->hKey[i]);
			handle->hKey[i]=0;
		}
	
	free(handle->hKey);
	free(handle);
	free(regpath);
	return NULL;
}	


int registry_getint(registry_handle *handle, const char *label, int *val)
{
	long lres;
	DWORD type, size;

	*val = 0;
	size = sizeof(int);
	lres = RegQueryValueEx(handle->hKey[handle->keys-1], label, NULL, 
				&type, (LPBYTE)val, &size);
	if (lres != ERROR_SUCCESS) return 0;
	if (type != REG_DWORD) return 0;
		
	return 1;
}


int registry_getstring(registry_handle *handle, const char *label, char *str, int bufsize)
{
	long lres;
	DWORD type, size;

	size = bufsize;
	lres = RegQueryValueEx(handle->hKey[handle->keys-1], label, NULL, &type, 
				(LPBYTE)str, &size);

	if (lres != ERROR_SUCCESS) return 0;
	if (type != REG_SZ && type != REG_EXPAND_SZ) return 0;
	
	return 1;
}		

		
	
int registry_close(registry_handle *handle)
{
	int i;

	for (i = handle->keys-1; i >=0; i--)
		if (handle->hKey[i])  {
			RegCloseKey(handle->hKey[i]);
			handle->hKey[i]=0;
		}
	
	free(handle->hKey);
	free(handle);

	return 1;
}





/*
int registry_create(const char *name)
{
	HANDLE hSubKey, hGroupKey, hOurKey;
	DWORD disp;
	int result;

	result = RegOpenKeyEx(HKEY_CURRENT_USER,  REG_DIR_PARENT, 0, KEY_ALL_ACCESS,
								&hSubKey);
	if (result!=ERROR_SUCCESS) return 0;
	result = RegOpenKeyEx(hSubKey, REG_DIR_GROUP, 0, KEY_ALL_ACCESS, &hGroupKey);
	if (result!=ERROR_SUCCESS) {
		result = RegCreateKeyEx(hSubKey, REG_DIR_GROUP, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
							&hGroupKey, &disp);
		if (result!=ERROR_SUCCESS) {
			RegCloseKey(hSubKey);
			return 0;
		}
		result = RegCreateKeyEx(hGroupKey, name, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
							&hOurKey, &disp);
		if (result !=ERROR_SUCCESS) {
			RegCloseKey(hGroupKey);
			RegCloseKey(hSubKey);
			return 0;
		}
	}
	RegCloseKey(hOurKey);
	RegCloseKey(hGroupKey);
	RegCloseKey(hSubKey);
  	return 1;
}
	

int registry_delete(const char *name)
{
	Int3();
	return 0;
}


int registry_open(const char *name)
{
	HANDLE hKey;
	int result;	
	char path[256];

	Assert(hMyRegKey == NULL);

	strcpy(path, REG_DIR_PATH);
	strcat(path, name);

	result = RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, 0, &hKey);
	if (result != ERROR_SUCCESS) return 0;

	hMyRegKey = hKey;
	
	return 1;
}

	
int registry_close()
{
	Assert(hMyRegKey != NULL);
	RegCloseKey(hMyRegKey);
	hMyRegKey = NULL;
	return 1;
}



int registry_addint(const char *valname, int val)
{
	int result;

	result = RegSetValueEx(hMyRegKey, valname, 0, REG_DWORD, &val, sizeof(val));

	if (result != ERROR_SUCCESS) return 0;
	return 1;
}


int registry_addstring(const char *valname, const char *str)
{
	int result;

	result = RegSetValueEx(hMyRegKey, valname, 0, REG_SZ, str, lstrlen(str));

	if (result != ERROR_SUCCESS) return 0;
	return 1;
}
*/
