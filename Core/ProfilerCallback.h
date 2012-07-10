#ifndef PROFILERCALLBACK_H_INC
#define PROFILERCALLBACK_H_INC

template<typename K, typename V> class Hashtable;
class ThreadInfo;
class ProfilerCallback;

extern ProfilerCallback *g_pProfiler;

class ProfilerCallback : public ICorProfilerCallback2
{ public:
  ProfilerCallback();
  virtual ~ProfilerCallback();
  
  // IUnknown
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();
  STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
  
  // ICorProfilerCallback
  STDMETHOD(Initialize)(IUnknown *pICorProfilerInfo);
  STDMETHOD(Shutdown)();

  STDMETHOD(AppDomainCreationStarted)(AppDomainID id);
  STDMETHOD(AppDomainCreationFinished)(AppDomainID id, HRESULT hrStatus);
  STDMETHOD(AppDomainShutdownStarted)(AppDomainID id);
  STDMETHOD(AppDomainShutdownFinished)(AppDomainID id, HRESULT hrStatus);

  STDMETHOD(AssemblyLoadStarted)(AssemblyID id);
  STDMETHOD(AssemblyLoadFinished)(AssemblyID id, HRESULT hrStatus);
  STDMETHOD(AssemblyUnloadStarted)(AssemblyID id);
  STDMETHOD(AssemblyUnloadFinished)(AssemblyID id, HRESULT hrStatus);

  STDMETHOD(ModuleLoadStarted)(ModuleID id);
  STDMETHOD(ModuleLoadFinished)(ModuleID id, HRESULT hrStatus);
  STDMETHOD(ModuleUnloadStarted)(ModuleID id);
  STDMETHOD(ModuleUnloadFinished)(ModuleID id, HRESULT hrStatus);
  STDMETHOD(ModuleAttachedToAssembly)(ModuleID id, AssemblyID assemblyID);

  STDMETHOD(ClassLoadStarted)(ClassID id);
  STDMETHOD(ClassLoadFinished)(ClassID id, HRESULT hrStatus);
  STDMETHOD(ClassUnloadStarted)(ClassID id);
  STDMETHOD(ClassUnloadFinished)(ClassID id, HRESULT hrStatus);
  STDMETHOD(FunctionUnloadStarted)(FunctionID id);

  STDMETHOD(JITCompilationStarted)(FunctionID id, BOOL fIsSafeToBlock);
  STDMETHOD(JITCompilationFinished)(FunctionID id, HRESULT hrStatus, BOOL fIsSafeToBlock);
  STDMETHOD(JITCachedFunctionSearchStarted)(FunctionID id, BOOL *pbUseCachedFunction);
  STDMETHOD(JITCachedFunctionSearchFinished)(FunctionID id, COR_PRF_JIT_CACHE result);
  STDMETHOD(JITFunctionPitched)(FunctionID id);
  STDMETHOD(JITInlining)(FunctionID callerID, FunctionID calleeID, BOOL *pfShouldInline);

  STDMETHOD(ThreadCreated)(ThreadID id);
  STDMETHOD(ThreadDestroyed)(ThreadID id);
  STDMETHOD(ThreadAssignedToOSThread)(ThreadID managedThreadID, DWORD osThreadID);

  STDMETHOD(RemotingClientInvocationStarted)();
  STDMETHOD(RemotingClientSendingMessage)(GUID *pCookie, BOOL fIsAsync);
  STDMETHOD(RemotingClientReceivingReply)(GUID *pCookie, BOOL fIsAsync);
  STDMETHOD(RemotingClientInvocationFinished)();
  STDMETHOD(RemotingServerReceivingMessage)(GUID *pCookie, BOOL fIsAsync);
  STDMETHOD(RemotingServerInvocationStarted)();
  STDMETHOD(RemotingServerInvocationReturned)();
  STDMETHOD(RemotingServerSendingReply)(GUID *pCookie, BOOL fIsAsync);

  STDMETHOD(UnmanagedToManagedTransition)(FunctionID func, COR_PRF_TRANSITION_REASON reason);
  STDMETHOD(ManagedToUnmanagedTransition)(FunctionID func, COR_PRF_TRANSITION_REASON reason);
                                            
  STDMETHOD(RuntimeSuspendStarted)(COR_PRF_SUSPEND_REASON suspendReason);
  STDMETHOD(RuntimeSuspendFinished)();
  STDMETHOD(RuntimeSuspendAborted)();
  STDMETHOD(RuntimeResumeStarted)();
  STDMETHOD(RuntimeResumeFinished)();
  STDMETHOD(RuntimeThreadSuspended)(ThreadID id);
  STDMETHOD(RuntimeThreadResumed)(ThreadID id);

  STDMETHOD(MovedReferences)(ULONG cmovedObjectIDRanges, ObjectID oldObjectIDRangeStart[],
                             ObjectID newObjectIDRangeStart[], ULONG cObjectIDRangeLength[]);

  STDMETHOD(ObjectAllocated)(ObjectID objectID, ClassID classID);
  STDMETHOD(ObjectsAllocatedByClass)(ULONG classCount, ClassID classIDs[], ULONG objects[]);
  STDMETHOD(ObjectReferences)(ObjectID objectID, ClassID classID, ULONG cObjectRefs, ObjectID objectRefIDs[]);
  STDMETHOD(RootReferences)(ULONG cRootRefs, ObjectID rootRefIDs[]);

  STDMETHOD(ExceptionThrown)(ObjectID thrown);

  STDMETHOD(ExceptionSearchFunctionEnter)(FunctionID func);
  STDMETHOD(ExceptionSearchFunctionLeave)();
  STDMETHOD(ExceptionSearchFilterEnter)(FunctionID func);
  STDMETHOD(ExceptionSearchFilterLeave)();
  STDMETHOD(ExceptionSearchCatcherFound)(FunctionID func);
  STDMETHOD(ExceptionCLRCatcherFound)();
  STDMETHOD(ExceptionCLRCatcherExecute)();
  STDMETHOD(ExceptionOSHandlerEnter)(FunctionID func);
  STDMETHOD(ExceptionOSHandlerLeave)(FunctionID func);

  STDMETHOD(ExceptionUnwindFunctionEnter)(FunctionID func);
  STDMETHOD(ExceptionUnwindFunctionLeave)();
  STDMETHOD(ExceptionUnwindFinallyEnter)(FunctionID func);
  STDMETHOD(ExceptionUnwindFinallyLeave)();
  STDMETHOD(ExceptionCatcherEnter)(FunctionID functionID, ObjectID objectID);
  STDMETHOD(ExceptionCatcherLeave)();

  STDMETHOD(COMClassicVTableCreated)(ClassID wrappedClassID, REFGUID implementedIID, void *pVTable, ULONG cSlots);
  STDMETHOD(COMClassicVTableDestroyed)(ClassID wrappedClassID, REFGUID implementedIID, void *pVTable);

  // ICorProfilerCallback2
  STDMETHOD(ThreadNameChanged)(ThreadID thread, ULONG cchName, WCHAR name[]);
  
  STDMETHOD(GarbageCollectionStarted)(int cGenerations, BOOL generationCollected[], COR_PRF_GC_REASON reason);
  STDMETHOD(GarbageCollectionFinished)();
  STDMETHOD(SurvivingReferences)(ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[],
                                 ULONG cObjectIDRangeLength[]);
  STDMETHOD(FinalizeableObjectQueued)(DWORD finalizerFlags, ObjectID objectID);
  STDMETHOD(RootReferences2)(ULONG cRootRefs, ObjectID rootRefIDs[], COR_PRF_GC_ROOT_KIND rootKinds[],
                             COR_PRF_GC_ROOT_FLAGS rootFlags[], UINT_PTR rootIDs[]);
  STDMETHOD(HandleCreated)(GCHandleID handleID, ObjectID initialObjectID);
  STDMETHOD(HandleDestroyed)(GCHandleID handleID);

  private:
  ThreadInfo * GetCurrentThreadInfo();
  ThreadInfo * GetThreadInfo(ThreadID thread);

  ICorProfilerInfo2 *m_pProfInfo;
  Hashtable<ThreadID,ThreadInfo*> *m_pThreadMap;
  LONG m_nRefCnt;
  
  static void __stdcall EnterFunction(FunctionID func);
  static void __stdcall LeaveFunction(UINT64 time);
  static void __stdcall TailcallFunction(FunctionID func, UINT64 time);
};

#endif // PROFILERCALLBACK_H_INC