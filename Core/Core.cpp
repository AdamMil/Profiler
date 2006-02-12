#include "stdafx.h"
#include "ProfilerCallback.h"
#include <new>

// constants
static const CLSID CLSID_ProfilerCallback = // {80340274-A1F5-478d-AD0D-CB3A6B2FF0C5}
{ 0x80340274, 0xa1f5, 0x478d, { 0xad, 0xd, 0xcb, 0x3a, 0x6b, 0x2f, 0xf0, 0xc5 }
};
static const char * const VerIndProgID = "AdamMil.Profiler";
static const char * const ProgID = "AdamMil.Profiler.1";
static const int CLSID_STRING_SIZE = 39;

// module data
static HINSTANCE g_hModuleInst;
static LONG g_cServerLocks;

// helpers
static void CLSIDtoString(REFCLSID clsid, char *szCLSID, int length);
static LONG SetKeyAndValue(const char *szKey, const char *szSubkey, const char *szValue);
static LONG SetValueInKey(const char *szKey, const char *szNamedValue, const char *szValue);
static HRESULT RegisterServer(HMODULE hModule, REFCLSID clsid, const char *szFriendlyName, const char *szVerIndProgID,
                              const char *szProgID, const char* szThreadingModel);
static HRESULT UnregisterServer(REFCLSID clsid, const char* szVerIndProgID, const char* szProgID);
static void LockModule()	{ InterlockedIncrement(&g_cServerLocks); }
static void UnlockModule()	{ InterlockedDecrement(&g_cServerLocks); }

/*** BEGIN ClassFactory ***/
class ClassFactory : public IClassFactory
{ public:
  ClassFactory() { m_nRefCnt = 1; }

	// IUnknown
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();
  STDMETHOD(QueryInterface)(REFIID riid, void **ppv);

	// IClassFactory
	STDMETHOD(CreateInstance)(IUnknown *pUnknownOuter, REFIID iid, void **ppv);
	STDMETHOD(LockServer)(BOOL bLock);

  private:
	LONG m_nRefCnt;
};

ULONG ClassFactory::AddRef() { return InterlockedIncrement(&m_nRefCnt); }

ULONG ClassFactory::Release()
{ LONG ret = InterlockedDecrement(&m_nRefCnt);
  if(ret==0) delete this;
  return ret;
}

HRESULT ClassFactory::QueryInterface(REFIID iid, void **ppv)
{ if(ppv==NULL) return E_POINTER;
  if(iid==IID_IClassFactory) *ppv = static_cast<IClassFactory*>(this);
  else if(iid==IID_IUnknown) *ppv = static_cast<IUnknown*>(this);
  else
  { *ppv = NULL;
    return E_NOINTERFACE;
  }
  AddRef();
  return S_OK;
}

HRESULT ClassFactory::CreateInstance(IUnknown *pUnknownOuter, REFIID iid, void **ppv)
{ if(pUnknownOuter) return CLASS_E_NOAGGREGATION;
 
  ProfilerCallback *pProf = new (std::nothrow) ProfilerCallback();
  if(pProf==NULL) return E_OUTOFMEMORY;

  HRESULT hr = pProf->QueryInterface(iid, ppv);
  pProf->Release();
  return hr;
}

HRESULT ClassFactory::LockServer(BOOL bLock)
{ if(bLock) LockModule();
  else UnlockModule();
  return S_OK;
}
/*** END ClassFactory ***/


BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{ switch(dwReason)
  { case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hInstance);
      g_hModuleInst = hInstance;
      break;
  }

  return TRUE;
} // DllMain

STDAPI DllRegisterServer()
{ return RegisterServer(g_hModuleInst, CLSID_ProfilerCallback, "AdamMil's Profiler", VerIndProgID, ProgID, "Both");
}

STDAPI DllUnregisterServer() { return UnregisterServer(CLSID_ProfilerCallback, VerIndProgID, ProgID); }

STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv)
{ if(clsid!=CLSID_ProfilerCallback) return CLASS_E_CLASSNOTAVAILABLE;

  ClassFactory *factory = new (std::nothrow) ClassFactory();
  if(factory==NULL) return E_OUTOFMEMORY;

  HRESULT hr = factory->QueryInterface(iid, ppv);
  factory->Release();
  return hr;
}

STDAPI DllCanUnloadNow()
{ return S_OK;
}

// Convert a CLSID to a char string.
static void CLSIDtoString(REFCLSID clsid, char *szCLSID, int length)
{ assert(length>=CLSID_STRING_SIZE);

	OLECHAR wszCLSID[CLSID_STRING_SIZE];
	wszCLSID[0] = 0; // guard against the assertion below failing in a release build
	int ret = StringFromGUID2(clsid, wszCLSID, CLSID_STRING_SIZE);
	assert(ret!=0);

	wcstombs(szCLSID, wszCLSID, length); // Covert from wide characters to non-wide.
} // CLSIDtoString

// Create a key and set its value.
static LONG SetKeyAndValue(const char *szKey, const char *szSubkey, const char *szValue)
{ assert(szKey && szValue);
	assert(strlen(szKey) + (szSubkey ? strlen(szSubkey) : 0) < 1000);

  HKEY hKey;
	char szKeyBuf[1024];

	strcpy_s(szKeyBuf, sizeof(szKeyBuf), szKey);
	if(szSubkey)
	{ strcat_s(szKeyBuf, sizeof(szKeyBuf), "\\");
		strcat_s(szKeyBuf, sizeof(szKeyBuf), szSubkey);
	}

	LONG status = RegCreateKeyEx(HKEY_CLASSES_ROOT, szKeyBuf, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
	                             &hKey, NULL);
	if(status!=ERROR_SUCCESS) return status;

	status = RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE*)szValue, (DWORD)strlen(szValue)+1);
	RegCloseKey(hKey);
	return status;
} // SetKeyAndValue

// Open a key and set a value.
static LONG SetValueInKey(const char *szKey, const char *szNamedValue, const char *szValue)
{ assert(szKey && szNamedValue && szValue);

  HKEY hKey;
	LONG status = RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_SET_VALUE, &hKey);
	if(status!=ERROR_SUCCESS) return status;

	status = RegSetValueEx(hKey, szNamedValue, 0, REG_SZ, (BYTE*)szValue, (DWORD)strlen(szValue)+1);
	RegCloseKey(hKey);
	return status;
} // SetKeyInValue

// Delete a key and all of its descendents.
static void RecursiveDeleteKey(HKEY hKeyParent, const char *lpszKeyChild)
{ HKEY hKeyChild;
	if(RegOpenKeyEx(hKeyParent, lpszKeyChild, 0, KEY_ALL_ACCESS, &hKeyChild) != ERROR_SUCCESS) return;

	// Enumerate all of the decendents of this child.
	FILETIME time;
	char szBuffer[256];
	DWORD dwSize = sizeof(szBuffer);
	while(RegEnumKeyEx(hKeyChild, 0, szBuffer, &dwSize, NULL, NULL, NULL, &time)==ERROR_SUCCESS)
	  RecursiveDeleteKey(hKeyChild, szBuffer); // Delete recursively

	RegCloseKey(hKeyChild);
	RegDeleteKey(hKeyParent, lpszKeyChild);
} // RecursiveDeleteKey

static HRESULT RegisterServer(HMODULE hModule, REFCLSID clsid, const char *szFriendlyName, const char *szVerIndProgID,
                              const char *szProgID, const char* szThreadingModel)
{
  // Get server location.
  char szModule[512];
  if(GetModuleFileName(hModule, szModule, sizeof(szModule))==0) return E_FAIL;

  char szCLSID[CLSID_STRING_SIZE];
  CLSIDtoString(clsid, szCLSID, sizeof(szCLSID));

  char szKey[CLSID_STRING_SIZE+7];
  strcpy_s(szKey, sizeof(szKey), "CLSID\\"); // Build the key CLSID\\{...}
  strcat_s(szKey, sizeof(szKey), szCLSID);
  LONG status = SetKeyAndValue(szKey, NULL, szFriendlyName); // Add the CLSID to the registry.
  if(status!=ERROR_SUCCESS) return status==ERROR_ACCESS_DENIED ? E_ACCESSDENIED : E_FAIL;

  // Add the server filename subkey under the CLSID key.
  SetKeyAndValue(szKey, "InprocServer32", szModule);
  char szInproc[sizeof(szKey)+16];
  strcpy_s(szInproc, sizeof(szInproc), szKey);
  strcat_s(szInproc, sizeof(szInproc), "\\InprocServer32");
  SetValueInKey(szInproc, "ThreadingModel", szThreadingModel);

  SetKeyAndValue(szKey, "ProgID", szProgID); // Add the ProgID subkey under the CLSID key.

  // Add the version-independent ProgID subkey under CLSID key.
  SetKeyAndValue(szKey, "VersionIndependentProgID", szVerIndProgID);

  // Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.
  SetKeyAndValue(szVerIndProgID, NULL, szFriendlyName);
  SetKeyAndValue(szVerIndProgID, "CLSID", szCLSID);
  SetKeyAndValue(szVerIndProgID, "CurVer", szProgID);

  // Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.
  SetKeyAndValue(szProgID, NULL, szFriendlyName); 
  SetKeyAndValue(szProgID, "CLSID", szCLSID);

  return S_OK;
} // RegisterServer

static HRESULT UnregisterServer(REFCLSID clsid, const char* szVerIndProgID, const char* szProgID)
{ char szCLSID[CLSID_STRING_SIZE];
	CLSIDtoString(clsid, szCLSID, sizeof(szCLSID));

	char szKey[CLSID_STRING_SIZE+7];
	strcpy_s(szKey, sizeof(szKey), "CLSID\\"); // Build the key CLSID\\{...}
	strcat_s(szKey, sizeof(szKey), szCLSID);

	RecursiveDeleteKey(HKEY_CLASSES_ROOT, szKey); // Delete the CLSID Key - CLSID\{...}
	RecursiveDeleteKey(HKEY_CLASSES_ROOT, szVerIndProgID);
	RecursiveDeleteKey(HKEY_CLASSES_ROOT, szProgID);

	return S_OK;
} // UnregisterServer