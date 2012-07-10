#include "stdafx.h"
#include "Hashtable.h"
#include "ThreadInfo.h"
#include "FunctionInfo.h"

ThreadInfo::ThreadInfo(ThreadID id) : ID(id)
{ m_capacity  = 8;
  m_count     = 0;
  m_frames    = new StackFrame[m_capacity];
  m_pFuncMap  = new Hashtable<FunctionID,FunctionInfo*>();
  m_bRunning  = false;
  m_startTime = m_endTime = m_suspendStart = m_suspendTime = 0;
}

ThreadInfo::~ThreadInfo()
{ delete[] m_frames;

  int funcCount = m_pFuncMap->Count();
  if(funcCount)
  { FunctionInfo **funcs = new FunctionInfo*[funcCount];
    m_pFuncMap->GetValues(funcs, funcCount);
    for(int i=0; i<funcCount; i++) delete funcs[i];
    delete[] funcs;
  }

  delete m_pFuncMap;
}

void ThreadInfo::Start()
{ m_startTime  = rdtsc();
  m_bRunning = true;
}

void ThreadInfo::End()
{ m_endTime = rdtsc();
  m_bRunning = false;
}

void ThreadInfo::Suspend(UINT64 time)
{ assert(m_count);
  m_suspendStart = time;
}

void ThreadInfo::Resume(UINT64 time)
{ assert(m_count);
  UINT64 elapsed = time-m_suspendStart;
  m_frames[m_count-1].Function->SuspendTime += elapsed;
  m_suspendTime += elapsed;
}

/***
int outer()             Enter outer
{ dostuff;
  inner();              Enter inner
}                       Leave inner, Depth=0, Time=Elapsed0 [Child outer->inner Time=Elapsed0]
                        Leave outer, Depth=0, Time=Elapsed1
outer's time+children = outer.Time
outer's time-children = outer.Time - child.Time
inner's time = inner.Time

int factorial(int n)
{ if(n<=0) return 0;
  if(n==1) return 1;
  return n * factorial(n-1);
}

factorial(1) = 1t
factorial(2) = 1t + 1t
factorial(3) = 1t + 1t + 1t

factorial(3)            Enter Depth=1, Count=1
  factorial(2)          Enter Depth=2, Count=2 [-> Child Depth=1, Count=1]
    factorial(1)        Enter Depth=3, Count=3 [-> Child Depth=2, Count=2]
                        Leave Depth=2, Time=0, RecTime=1 [-> Child Depth=1, Time=0, RecTime=1]
                        Leave Depth=1, Time=0, RecTime=1+2 [-> Child Depth=0, Time=2, RecTime=1]
                        Leave Depth=0, Time=3, RecTime=1+2
outer time+children = factorial.Time = 3
outer time-children = factorial.Time - child.Time = 1
average time+children = (Time+RecTime)/Count = (3+3)/3 = 6/3 = 2
average time-children = (Time+RecTime-child.Time-child.RecTime)/Count = (3+3-2-1)/3 = 3/3 = 1

If we only need the average, we can combine Time and RecTime and eliminate Depth, producing

factorial(3)            Enter Count=1
  factorial(2)          Enter Count=2 [-> Child Count=1]
    factorial(1)        Enter Count=3 [-> Child Count=2]
                        Leave Time=1 [-> Child Time=1]
                        Leave Time=1+2 [-> Child Time=1+2]
                        Leave Time=1+2+3
outer time+children = <can't be retrieved>
outer time-children = <can't be retrieved>
average time+children = Time/Count = 6/3 = 2
average time-children = (Time-child.Time)/Count = (6-3)/3 = 3/3 = 1
***/
void ThreadInfo::EnterFunction(FunctionID func)
{ FunctionInfo *pFunc;
  if(!m_pFuncMap->Find(func, &pFunc))
  { pFunc = new FunctionInfo(func);
    m_pFuncMap->Add(func, pFunc);
  }

  if(m_count)
  { FunctionInfo *pCaller = m_frames[m_count-1].Function;
    ChildInfo &child = pCaller->GetChildInfo(pFunc);
    child.NumCalls++;

    if(m_count==m_capacity)
    { m_capacity *= 2;
      StackFrame *newFrames = new StackFrame[m_capacity];
      memcpy(newFrames, m_frames, sizeof(StackFrame)*m_count);
      delete[] m_frames;
      m_frames = newFrames;
    }
  }
  
  pFunc->NumCalls++;

  new (&m_frames[m_count++]) StackFrame(pFunc);
}

void ThreadInfo::LeaveFunction(UINT64 time)
{ assert(m_count);
  UINT64 elapsed = time - m_frames[--m_count].StartTime;

  FunctionInfo *pFunc = m_frames[m_count].Function;
  pFunc->Time += elapsed;
  if(m_count) m_frames[m_count-1].Function->GetChildInfo(pFunc).Time += elapsed;
}
