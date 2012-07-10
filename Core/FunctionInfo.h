#ifndef FUNCTIONINFO_H_INC
#define FUNCTIONINFO_H_INC

template<typename K, typename V> class Hashtable;

struct ChildInfo
{ ChildInfo() { Time=0; NumCalls=0; }
  UINT64 Time;
  UINT NumCalls;
};

class FunctionInfo
{ public:
  FunctionInfo(FunctionID id);
  virtual ~FunctionInfo();

  ChildInfo & GetChildInfo(FunctionInfo *pFunc);

  UINT64 Time, SuspendTime;
  UINT32 NumCalls;
  const FunctionID ID;
  
  private:
  Hashtable<FunctionID,ChildInfo*> *m_pChildMap;
};

#endif // FUNCTIONINFO_H_INC