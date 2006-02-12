#ifndef THREADINFO_H_INC
#define THREADINFO_H_INC

template<typename K, typename V> class Hashtable;
class FunctionInfo;

struct StackFrame
{ StackFrame() { }
  StackFrame(FunctionInfo *pFunc, UINT64 time) { Function=pFunc; StartTime=time; }
  UINT64 StartTime;
  FunctionInfo *Function;
};

class ThreadInfo
{ public:
  ThreadInfo(ThreadID id);
  virtual ~ThreadInfo();

  bool IsRunning() { return m_bRunning; }

  DWORD GetNativeID();

  void Start();
  void End();

  void Suspend(UINT64 time);
  void Resume(UINT64 time);

  void EnterFunction(FunctionID func, UINT64 time);
  UINT64 LeaveFunction(UINT64 time);

  private: UINT64 m_startTime, m_endTime, m_suspendStart, m_suspendTime;
  public: const ThreadID ID; // this is placed here because I want the 64-bit members first
  private:
  Hashtable<FunctionID,FunctionInfo*> *m_pFuncMap;
  StackFrame *m_frames;
  int m_count, m_capacity;
  bool m_bRunning;
}; // ThreadInfo

#endif // THREADINFO_H_INC