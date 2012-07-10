#include "stdafx.h"
#include "FunctionInfo.h"
#include "Hashtable.h"

FunctionInfo::FunctionInfo(FunctionID id) : ID(id)
{ m_pChildMap = NULL;
  Time        = SuspendTime = 0;
  NumCalls    = 0;
}

FunctionInfo::~FunctionInfo()
{ if(m_pChildMap)
  { int childCount = m_pChildMap->Count();
    if(childCount)
    { ChildInfo **children = new ChildInfo*[childCount];
      m_pChildMap->GetValues(children, childCount);
      for(int i=0; i<childCount; i++) delete children[i];
      delete[] children;
    }

    delete m_pChildMap;
  }
}

ChildInfo & FunctionInfo::GetChildInfo(FunctionInfo *pFunc)
{ ChildInfo *ret;
  if(!m_pChildMap) m_pChildMap = new Hashtable<FunctionID,ChildInfo*>();
  if(!m_pChildMap->Find(pFunc->ID, &ret))
  { ret = new ChildInfo();
    m_pChildMap->Add(pFunc->ID, ret);
  }
  return *ret;
}