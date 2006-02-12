#ifndef HASHTABLE_H_INC
#define HASHTABLE_H_INC

class HashtableBase
{ public:
  HashtableBase();
  virtual ~HashtableBase();
  
  void Clear();
  int Count() const { return m_count; }

  protected:
  void * operator[](const void *key) const
  { void *ret;
    assert(Contains(key));
    Find(key, &ret);
    return ret;
  }

  bool Contains(const void *key) const;
  bool Find(const void *key, void **value) const;
  void Remove(const void *key);
  void Set(const void *key, const void *value, bool throwIfNotFound);

  void GetKeysAndValues(void **keys, void **values, int size) const;

  private:
  struct Bucket
  { const void *Key, *Value;
    int Hash;
  };

  #pragma warning(disable : 4311) // pointer value truncated
  int calcHash(const void *key, UINT32 *incr) const
  { return calcHash(reinterpret_cast<UINT32>(key), m_capacity, incr);
  }
  #pragma warning(default : 4311)

  Bucket *m_buckets;
  int m_count, m_capacity, m_loadSize;
  
  void enlarge();

  static int calcHash(UINT32 baseHash, int capacity, UINT32 *incr);
  static bool isPrime(int n);
  static int getCapacity(int minSize);
  static void rehash(Bucket *buckets, int capacity, const void *key, const void *value, UINT32 hash);
  static int s_primes[];
}; // HashtableBase

template<typename K, typename V>
class Hashtable : public HashtableBase
{ public:
  V operator[](const K key) const { return HashtableBase::operator[](reinterpret_cast<void*>(key)); }

  void Add(const K key, const V value)
  { HashtableBase::Set(reinterpret_cast<void*>(key), reinterpret_cast<void*>(value), true);
  }

  bool Contains(const K key) const { return HashtableBase::Contains(reinterpret_cast<void*>(key)); }

  bool Find(const K key, V *value) const
  { return HashtableBase::Find(reinterpret_cast<void*>(key), reinterpret_cast<void**>(value));
  }

  void GetKeys(K *keys, int length) const
  { HashtableBase::GetKeysAndValues(reinterpret_cast<void*[]>(keys), NULL, length);
  }

  void GetValues(V *values, int length) const
  { HashtableBase::GetKeysAndValues(NULL, reinterpret_cast<void**>(values), length);
  }

  void GetKeysAndValues(K *keys, V *values, int length) const
  { HashtableBase::GetKeysAndValues(reinterpret_cast<void**>(keys), reinterpret_cast<void*[]>(values), length);
  }

  void Remove(const K key) { HashtableBase::Remove(reinterpret_cast<void*>(key)); }

  void Set(const K key, const V value)
  { HashtableBase::Set(reinterpret_cast<void*>(key), reinterpret_cast<void*>(value), false);
  }
}; // Hashtable

#endif // HASHTABLE_H_INC