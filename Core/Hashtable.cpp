#include "stdafx.h"
#include "Hashtable.h"
#include "Common.h"
#include <math.h>

/*** HashtableBase ***/
static const float loadFactor = 0.72f;

HashtableBase::HashtableBase()
{ m_capacity = getCapacity(0);
  m_count    = 0;
  m_buckets  = new Bucket[m_capacity];
  m_loadSize = (int)(loadFactor*m_capacity);
  memset(m_buckets, 0, sizeof(Bucket)*m_capacity);
}

HashtableBase::~HashtableBase() { delete[] m_buckets; }

void HashtableBase::Clear()
{ if(m_count!=0)
  { for(int i=0; i<m_capacity; i++) m_buckets[i].Hash = 0;
    m_count = 0;
  }
}

bool HashtableBase::Contains(const void *key) const
{ assert(key);
  Bucket *b;
  UINT32 incr, hash=calcHash(key, &incr), seed=hash;
  int ntry=0;
  do
  { b = &m_buckets[(int)(seed % m_capacity)];
    if(!b->Key) return false;
    if(b->Key==key) return true;
    seed += incr;
  } while((b->Hash&0x80000000) && ++ntry<m_capacity);
  return false;
}

bool HashtableBase::Find(const void *key, void **value) const
{ assert(key && value);
  Bucket *b;
  UINT32 incr, hash=calcHash(key, &incr), seed=hash;
  int ntry=0;
  do
  { b = &m_buckets[(int)(seed % m_capacity)];
    if(!b->Key) break;
    if(b->Key==key)
    { *value = const_cast<void*>(b->Value);
      return true;
    }
    seed += incr;
  } while((b->Hash&0x80000000) && ++ntry<m_capacity);
  *value = NULL;
  return false;
}

void HashtableBase::Set(const void *key, const void *value, bool throwIfExists)
{ assert(key);
  if(m_count>=m_loadSize) enlarge();
  Bucket *b;
  UINT32 incr, hash=calcHash(key, &incr), seed=hash;
  int ntry=0, emptySlot=-1, bucketi;
  do
  { bucketi = (int)(seed % m_capacity);
    b = &m_buckets[bucketi];
    if(emptySlot==-1 && b->Key==m_buckets && (b->Hash&0x80000000)) emptySlot = bucketi;
    if(!b->Key || b->Key==m_buckets && !(b->Hash&0x80000000))
    { if(emptySlot!=-1) bucketi = emptySlot;
      b->Value = value;
      b->Key   = key;
      b->Hash |= hash;
      m_count++;
      return;
    }
    
    if(b->Key==key)
    { if(throwIfExists) throw already_exists();
      b->Value = value;
      return;
    }
    
    if(emptySlot==-1) b->Hash |= 0x80000000;
    seed += incr;
  } while(++ntry < m_capacity);
  
  if(emptySlot!=-1)
  { b = &m_buckets[emptySlot];
    b->Value = value;
    b->Key   = key;
    b->Hash |= hash;
    m_count++;
  }
  
  assert(false);
}

void HashtableBase::Remove(const void *key)
{ assert(key);
  Bucket *b;
  UINT32 incr, hash=calcHash(key, &incr), seed=hash;
  int ntry=0;
  do
  { b = &m_buckets[(int)(seed%m_capacity)];
    if(b->Key==key)
    { b->Hash &= 0x80000000;
      b->Key   = b->Hash ? m_buckets : NULL;
      m_count--;
      return;
    }
    seed += incr;
  } while((b->Hash&0x80000000) && ++ntry<m_capacity);
}

void HashtableBase::GetKeysAndValues(void *keys[], void *values[], int size) const
{ assert((keys || values) && size==m_count);
  int j=0;
  for(int i=m_capacity; --i>=0;)
  { Bucket *b = &m_buckets[i];
    if(b->Key && b->Key!=m_buckets)
    { if(keys) keys[j] = const_cast<void*>(b->Key);
      if(values) values[j] = const_cast<void*>(b->Value);
      j++;
    }
  }
  assert(j==size);
}

void HashtableBase::enlarge()
{ int oldCapacity = m_capacity;
  m_capacity = getCapacity(oldCapacity*2+1);

  Bucket *newBuckets = new Bucket[m_capacity];
  memset(newBuckets, 0, sizeof(Bucket)*m_capacity);

  for(int nb=0; nb<oldCapacity; nb++)
  { Bucket *old = &m_buckets[nb];
    if(old->Key && old->Key!=m_buckets)
      rehash(newBuckets, m_capacity, old->Key, old->Value, old->Hash&0x7FFFFFFF);
  }

  delete[] m_buckets;
  m_buckets  = newBuckets;
  m_loadSize = (int)(m_capacity * loadFactor);
}

int HashtableBase::calcHash(UINT32 baseHash, int capacity, UINT *incr)
{ UINT32 hash = baseHash & 0x7FFFFFFF;
  *incr = (((hash>>5)+1) % (UINT32)(capacity-1) + 1);
  return hash;
}

bool HashtableBase::isPrime(int n)
{ if(n&1)
  { for(int divisor=3,end=(int)sqrt((float)n); divisor<end; divisor+=2)
      if((n%divisor)==0) return false;
    return true;
  }
  else return n==2;
}

int HashtableBase::getCapacity(int minSize)
{ for(int *prime=s_primes; *prime; *prime++) if(*prime>=minSize) return *prime;
  for(int i=minSize|1; i<0x7FFFFFFF; i+=2) if(isPrime(i)) return i;
  throw std::bad_alloc();
}

void HashtableBase::rehash(Bucket *buckets, int capacity, const void *key, const void *value, UINT32 hash)
{ assert((hash&0x80000000)==0);
  Bucket *b;
  UINT32 incr, seed=calcHash(hash, capacity, &incr);
  while(true)
  { b = &buckets[(int)(seed%capacity)];
    if(!b->Key || b->Key==buckets)
    { b->Value = value;
      b->Key   = key;
      b->Hash |= hash;
      return;
    }
    b->Hash |= 0x80000000;
    seed += incr;
  }
}

int HashtableBase::s_primes[] =
{ 11,17,23,29,37,47,59,71,89,107,131,163,197,239,293,353,431,521,631,761,919,
  1103,1327,1597,1931,2333,2801,3371,4049,4861,5839,7013,8419,10103,12143,14591,
  17519,21023,25229,30293,36353,43627,52361,62851,75431,90523, 108631, 130363, 
  156437, 187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403,
  968897, 1162687, 1395263, 1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 
  4999559, 5999471, 7199369, 0
};
