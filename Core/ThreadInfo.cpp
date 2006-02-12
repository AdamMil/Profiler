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

void ThreadInfo::EnterFunction(FunctionID func, UINT64 time)
{ FunctionInfo *pFunc;
  if(!m_pFuncMap->Find(func, &pFunc))
  { pFunc = new FunctionInfo(func);
    m_pFuncMap->Add(func, pFunc);
  }

  if(m_count)
  { FunctionInfo *pCaller = m_frames[m_count-1].Function;
    ChildInfo &child = pCaller->GetChildInfo(pFunc);
    child.Depth++;
    child.NumCalls++;

    if(m_count==m_capacity)
    { m_capacity *= 2;
      StackFrame *newFrames = new StackFrame[m_capacity];
      memcpy(newFrames, m_frames, sizeof(StackFrame)*m_count);
      delete[] m_frames;
      m_frames = newFrames;
    }
  }
  
  pFunc->Depth++;
  pFunc->NumCalls++;
  
  new (&m_frames[m_count++]) StackFrame(pFunc, time);
}

UINT64 ThreadInfo::LeaveFunction(UINT64 time)
{ assert(m_count);
  UINT64 elapsed = time - m_frames[--m_count].StartTime;

  FunctionInfo *pFunc = m_frames[m_count].Function;
  assert(pFunc->Depth);
  if(--pFunc->Depth) pFunc->RecursiveTime += elapsed;
  else pFunc->Time += elapsed;
  
  if(m_count)
  { ChildInfo &child = m_frames[m_count-1].Function->GetChildInfo(pFunc);
    assert(child.Depth);
    if(--child.Depth) child.RecursiveTime += elapsed;
    else child.Time += elapsed;
  }

  return elapsed;
}
