#ifndef FUNCTIONINFO_H_INC
#define FUNCTIONINFO_H_INC

template<typename K, typename V> class Hashtable;

struct ChildInfo
{ ChildInfo() { Time=RecursiveTime=0; NumCalls=Depth=0; }
  UINT64 Time, RecursiveTime;
  UINT NumCalls, Depth;
};

class FunctionInfo
{ public:
  FunctionInfo(FunctionID id);
  virtual ~FunctionInfo();

  ChildInfo & GetChildInfo(FunctionInfo *pFunc);

  UINT64 Time, RecursiveTime, SuspendTime;
  UINT32 NumCalls, Depth;
  const FunctionID ID;
  
  private:
  Hashtable<FunctionID, ChildInfo*> *m_pChildMap;
};

#endif // FUNCTIONINFO_H_INC