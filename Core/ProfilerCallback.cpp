#include "stdafx.h"
#include "ProfilerCallback.h"
#include "ThreadInfo.h"
#include "Hashtable.h"

#define CHKRET3(hr,c,label) { (hr) = (c); if(FAILED(hr)) goto label; }
#define CHKRET2(c,label) CHKRET3(hr,c,label)
#define CHKRET(c) CHKRET3(hr,c,error)

ProfilerCallback *g_pProfiler;

// TODO: note, use of rdtsc is not safe on machines with multiple processors. switch to a different system on multiproc
// machines

__declspec(naked) static void RawEnterFunction(FunctionID funcID, UINT_PTR clientData, COR_PRF_FRAME_INFO func,
                                               COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
{ __asm
  { 
    push eax
    push ecx
    push edx
    push [DWORD PTR esp+16]
    call ProfilerCallback::EnterFunction
    pop edx
    pop ecx
    pop eax
    ret 16
  }
}

__declspec(naked) static void RawLeaveFunction(FunctionID funcID, UINT_PTR clientData, COR_PRF_FRAME_INFO func,
                                               COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange)
{ __asm
  { 
    push eax
    push edx
    rdtsc
    push ecx
    push edx
    push eax
    call ProfilerCallback::LeaveFunction
    pop ecx
    pop edx
    pop eax
    ret 16
  }
}

__declspec(naked) static void RawTailcallFunction(FunctionID funcID, UINT_PTR clientData, COR_PRF_FRAME_INFO func)
{ __asm
  { 
    push eax
    push edx
    rdtsc
    push ecx
    push edx
    push eax
    push [DWORD PTR esp+24]
    call ProfilerCallback::TailcallFunction
    pop ecx
    pop edx
    pop eax
    ret 12
  }
}

ProfilerCallback::ProfilerCallback()
{ m_nRefCnt    = 1;
  m_pProfInfo  = NULL;
  m_pThreadMap = NULL;
}

ProfilerCallback::~ProfilerCallback()
{ // if we have an ICorProfilerInfo2 pointer, release it.
  if(m_pProfInfo!=NULL) { m_pProfInfo->Release(); m_pProfInfo = NULL; }

  if(m_pThreadMap)
  { int threadCount = m_pThreadMap->Count();
    if(threadCount)
    { ThreadInfo **threads = new ThreadInfo*[threadCount];
      m_pThreadMap->GetValues(threads, threadCount);
      for(int i=0; i<threadCount; i++) delete threads[i];
      delete[] threads;
    }

    delete m_pThreadMap;
  }
}

ULONG ProfilerCallback::AddRef() { return InterlockedIncrement(&m_nRefCnt); }

ULONG ProfilerCallback::Release()
{ ULONG ret = (ULONG)InterlockedDecrement(&m_nRefCnt);
  if(ret==0) delete this;
  return ret;
}

HRESULT ProfilerCallback::QueryInterface(REFIID riid, void **ppv)
{ if(ppv==NULL) return E_POINTER;
  if(riid==IID_ICorProfilerCallback2) *ppv = static_cast<ICorProfilerCallback2*>(this);
  else if(riid==IID_ICorProfilerCallback) *ppv = static_cast<ICorProfilerCallback*>(this);
  else if(riid==IID_IUnknown) *ppv = static_cast<IUnknown*>(this);
  else
  { *ppv = NULL;
    return E_NOINTERFACE;
  }
  
  AddRef();
  return S_OK;
}

HRESULT ProfilerCallback::Initialize(IUnknown *pICorProfilerInfo)
{ if(pICorProfilerInfo==NULL) return E_POINTER; // verify that the passed pointer is valid
  if(g_pProfiler) return E_ABORT;
  if(m_pProfInfo!=NULL) { m_pProfInfo->Release(); m_pProfInfo = NULL; } // if so, release our own pointer
  // then, query for the ICorProfilerInfo2 interface on the given object
  if(FAILED(pICorProfilerInfo->QueryInterface(IID_ICorProfilerInfo2, (void**)&m_pProfInfo))) return E_INVALIDARG;
  assert(m_pProfInfo!=NULL);
  // register for JIT compilation and function enter/leave/unload events
  HRESULT hr;
  CHKRET(m_pProfInfo->SetEventMask(COR_PRF_MONITOR_JIT_COMPILATION | COR_PRF_MONITOR_ENTERLEAVE |
                                   COR_PRF_MONITOR_FUNCTION_UNLOADS));
  // set the callbacks for function call events
  CHKRET(m_pProfInfo->SetEnterLeaveFunctionHooks2(RawEnterFunction, RawLeaveFunction, RawTailcallFunction));
  
  // allocate private members
  m_pThreadMap = new Hashtable<ThreadID,ThreadInfo*>();
  // set the one and only allowed profiler instance
  g_pProfiler  = this;
  return S_OK;

  error:
  if(m_pProfInfo!=NULL) { m_pProfInfo->Release(); m_pProfInfo = NULL; }
  return hr;
}

#include <stdio.h>
HRESULT ProfilerCallback::Shutdown()
{ // if we have an ICorProfilerInfo2 pointer, release it.
  if(m_pProfInfo!=NULL) { m_pProfInfo->Release(); m_pProfInfo = NULL; }
  g_pProfiler = NULL;
  return S_OK;
}

HRESULT ProfilerCallback::FunctionUnloadStarted(FunctionID id) { return S_OK; }
HRESULT ProfilerCallback::JITCompilationStarted(FunctionID id, BOOL fIsSafeToBlock) { return S_OK; }
HRESULT ProfilerCallback::JITCompilationFinished(FunctionID id, HRESULT hrStatus, BOOL fIsSafeToBlock) { return S_OK; }
HRESULT ProfilerCallback::JITFunctionPitched(FunctionID id) { return S_OK; }
HRESULT ProfilerCallback::JITInlining(FunctionID callerID, FunctionID calleeID, BOOL *pfShouldInline) { return S_OK; }

ThreadInfo * ProfilerCallback::GetCurrentThreadInfo()
{ assert(m_pProfInfo);
  ThreadID id;
  if(FAILED(m_pProfInfo->GetCurrentThreadID(&id))) return NULL;
  return GetThreadInfo(id);
}
 
ThreadInfo * ProfilerCallback::GetThreadInfo(ThreadID thread)
{ ThreadInfo *ret;
  if(!m_pThreadMap->Find(thread, &ret))
  { ret = new ThreadInfo(thread);
    m_pThreadMap->Add(thread, ret);
  }
  return ret;
}

void ProfilerCallback::EnterFunction(FunctionID func)
{ assert(g_pProfiler && func);
  g_pProfiler->GetCurrentThreadInfo()->EnterFunction(func);
}

void ProfilerCallback::LeaveFunction(UINT64 time)
{ assert(g_pProfiler);
  g_pProfiler->GetCurrentThreadInfo()->LeaveFunction(time);
}

void ProfilerCallback::TailcallFunction(FunctionID func, UINT64 time)
{ assert(g_pProfiler && func);
  ThreadInfo *pThread = g_pProfiler->GetCurrentThreadInfo();
  pThread->LeaveFunction(time);
  pThread->EnterFunction(func);
}

// not interested in these...
HRESULT ProfilerCallback::AppDomainCreationStarted(AppDomainID id) { return S_OK; }
HRESULT ProfilerCallback::AppDomainCreationFinished(AppDomainID id, HRESULT hrStatus) { return S_OK; }
HRESULT ProfilerCallback::AppDomainShutdownStarted(AppDomainID id) { return S_OK; }
HRESULT ProfilerCallback::AppDomainShutdownFinished(AppDomainID id, HRESULT hrStatus) { return S_OK; }
HRESULT ProfilerCallback::AssemblyLoadStarted(AssemblyID id) { return S_OK; }
HRESULT ProfilerCallback::AssemblyLoadFinished(AssemblyID id, HRESULT hrStatus) { return S_OK; }
HRESULT ProfilerCallback::AssemblyUnloadStarted(AssemblyID id) { return S_OK; }
HRESULT ProfilerCallback::AssemblyUnloadFinished(AssemblyID id, HRESULT hrStatus) { return S_OK; }
HRESULT ProfilerCallback::ModuleLoadStarted(ModuleID id) { return S_OK; }
HRESULT ProfilerCallback::ModuleLoadFinished(ModuleID id, HRESULT hrStatus) { return S_OK; }
HRESULT ProfilerCallback::ModuleUnloadStarted(ModuleID id) { return S_OK; }
HRESULT ProfilerCallback::ModuleUnloadFinished(ModuleID id, HRESULT hrStatus) { return S_OK; }
HRESULT ProfilerCallback::ModuleAttachedToAssembly(ModuleID id, AssemblyID assemblyID) { return S_OK; }
HRESULT ProfilerCallback::ClassLoadStarted(ClassID id) { return S_OK; }
HRESULT ProfilerCallback::ClassLoadFinished(ClassID id, HRESULT hrStatus) { return S_OK; }
HRESULT ProfilerCallback::ClassUnloadStarted(ClassID id) { return S_OK; }
HRESULT ProfilerCallback::ClassUnloadFinished(ClassID id, HRESULT hrStatus) { return S_OK; }
HRESULT ProfilerCallback::JITCachedFunctionSearchStarted(FunctionID id, BOOL *pbUseCachedFunction) { return S_OK; }
HRESULT ProfilerCallback::JITCachedFunctionSearchFinished(FunctionID id, COR_PRF_JIT_CACHE result) { return S_OK; }
HRESULT ProfilerCallback::ThreadCreated(ThreadID id) { return S_OK; }
HRESULT ProfilerCallback::ThreadDestroyed(ThreadID id) { return S_OK; }
HRESULT ProfilerCallback::ThreadAssignedToOSThread(ThreadID managedThreadID, DWORD osThreadID) { return S_OK; }
HRESULT ProfilerCallback::RemotingClientInvocationStarted() { return S_OK; }
HRESULT ProfilerCallback::RemotingClientSendingMessage(GUID *pCookie, BOOL fIsAsync) { return S_OK; }
HRESULT ProfilerCallback::RemotingClientReceivingReply(GUID *pCookie, BOOL fIsAsync) { return S_OK; }
HRESULT ProfilerCallback::RemotingClientInvocationFinished() { return S_OK; }
HRESULT ProfilerCallback::RemotingServerReceivingMessage(GUID *pCookie, BOOL fIsAsync) { return S_OK; }
HRESULT ProfilerCallback::RemotingServerInvocationStarted() { return S_OK; }
HRESULT ProfilerCallback::RemotingServerInvocationReturned() { return S_OK; }
HRESULT ProfilerCallback::RemotingServerSendingReply(GUID *pCookie, BOOL fIsAsync) { return S_OK; }
HRESULT ProfilerCallback::UnmanagedToManagedTransition(FunctionID func, COR_PRF_TRANSITION_REASON reason) { return S_OK; }
HRESULT ProfilerCallback::ManagedToUnmanagedTransition(FunctionID func, COR_PRF_TRANSITION_REASON reason) { return S_OK; }
HRESULT ProfilerCallback::RuntimeSuspendStarted(COR_PRF_SUSPEND_REASON suspendReason) { return S_OK; }
HRESULT ProfilerCallback::RuntimeSuspendFinished() { return S_OK; }
HRESULT ProfilerCallback::RuntimeSuspendAborted() { return S_OK; }
HRESULT ProfilerCallback::RuntimeResumeStarted() { return S_OK; }
HRESULT ProfilerCallback::RuntimeResumeFinished() { return S_OK; }
HRESULT ProfilerCallback::RuntimeThreadSuspended(ThreadID id) { return S_OK; }
HRESULT ProfilerCallback::RuntimeThreadResumed(ThreadID id) { return S_OK; }
HRESULT ProfilerCallback::MovedReferences(ULONG cmovedObjectIDRanges, ObjectID oldObjectIDRangeStart[], ObjectID newObjectIDRangeStart[], ULONG cObjectIDRangeLength[]) { return S_OK; }
HRESULT ProfilerCallback::ObjectAllocated(ObjectID objectID, ClassID classID) { return S_OK; }
HRESULT ProfilerCallback::ObjectsAllocatedByClass(ULONG classCount, ClassID classIDs[], ULONG objects[]) { return S_OK; }
HRESULT ProfilerCallback::ObjectReferences(ObjectID objectID, ClassID classID, ULONG cObjectRefs, ObjectID objectRefIDs[]) { return S_OK; }
HRESULT ProfilerCallback::RootReferences(ULONG cRootRefs, ObjectID rootRefIDs[]) { return S_OK; }
HRESULT ProfilerCallback::ExceptionThrown(ObjectID thrown) { return S_OK; }
HRESULT ProfilerCallback::ExceptionSearchFunctionEnter(FunctionID func) { return S_OK; }
HRESULT ProfilerCallback::ExceptionSearchFunctionLeave() { return S_OK; }
HRESULT ProfilerCallback::ExceptionSearchFilterEnter(FunctionID func) { return S_OK; }
HRESULT ProfilerCallback::ExceptionSearchFilterLeave() { return S_OK; }
HRESULT ProfilerCallback::ExceptionSearchCatcherFound(FunctionID func) { return S_OK; }
HRESULT ProfilerCallback::ExceptionCLRCatcherFound() { return S_OK; }
HRESULT ProfilerCallback::ExceptionCLRCatcherExecute() { return S_OK; }
HRESULT ProfilerCallback::ExceptionOSHandlerEnter(FunctionID func) { return S_OK; }
HRESULT ProfilerCallback::ExceptionOSHandlerLeave(FunctionID func) { return S_OK; }
HRESULT ProfilerCallback::ExceptionUnwindFunctionEnter(FunctionID func) { return S_OK; }
HRESULT ProfilerCallback::ExceptionUnwindFunctionLeave() { return S_OK; }
HRESULT ProfilerCallback::ExceptionUnwindFinallyEnter(FunctionID func) { return S_OK; }
HRESULT ProfilerCallback::ExceptionUnwindFinallyLeave() { return S_OK; }
HRESULT ProfilerCallback::ExceptionCatcherEnter(FunctionID functionID, ObjectID objectID) { return S_OK; }
HRESULT ProfilerCallback::ExceptionCatcherLeave() { return S_OK; }
HRESULT ProfilerCallback::COMClassicVTableCreated(ClassID wrappedClassID, REFGUID implementedIID, void *pVTable, ULONG cSlots) { return S_OK; }
HRESULT ProfilerCallback::COMClassicVTableDestroyed(ClassID wrappedClassID, REFGUID implementedIID, void *pVTable) { return S_OK; }
HRESULT ProfilerCallback::ThreadNameChanged(ThreadID thread, ULONG cchName, WCHAR name[]) { return S_OK; }
HRESULT ProfilerCallback::GarbageCollectionStarted(int cGenerations, BOOL generationCollected[], COR_PRF_GC_REASON reason) { return S_OK; }
HRESULT ProfilerCallback::GarbageCollectionFinished() { return S_OK; }
HRESULT ProfilerCallback::SurvivingReferences(ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[], ULONG cObjectIDRangeLength[]) { return S_OK; }
HRESULT ProfilerCallback::FinalizeableObjectQueued(DWORD finalizerFlags, ObjectID objectID) { return S_OK; }
HRESULT ProfilerCallback::RootReferences2(ULONG cRootRefs, ObjectID rootRefIDs[], COR_PRF_GC_ROOT_KIND rootKinds[], COR_PRF_GC_ROOT_FLAGS rootFlags[], UINT_PTR rootIDs[]) { return S_OK; }
HRESULT ProfilerCallback::HandleCreated(GCHandleID handleID, ObjectID initialObjectID) { return S_OK; }
HRESULT ProfilerCallback::HandleDestroyed(GCHandleID handleID) { return S_OK; }