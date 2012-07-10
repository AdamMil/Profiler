#ifndef FUNCTIONMETADATA_H_INC
#define FUNCTIONMETADATA_H_INC

class FunctionMetadata
{ public:
  TypeMetadata * GetTypeMetadata();
  const wchar_t * GetName();

  private:
  FunctionMetadata();
  ~FunctionMetadata();
}

#endif // FUNCTIONMETADATA_H_INC