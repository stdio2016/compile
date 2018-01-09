#pragma once
#ifndef MyHash_INCLUDED
#define MyHash_INCLUDED
struct HashBucket {
  struct HashBucket *next;
  void *key;
  void *value;
};

struct MyHash {
  struct HashBucket **_buckets;
  size_t _nb; // number of buckets
  size_t _size; // number of keys

  // compare function
  // if a < b return < 0
  // if a = b return 0
  // if a > b return > 0
  int (*_cmp)(const void *a, const void *b);
  size_t (*_hf)(const void *key); // hash function
};

struct MyHashIterator {
  struct MyHash *table;
  size_t bucket;
  struct HashBucket *it; // points to current bucket or NULL if at end
};

void MyHash_init(struct MyHash *table, int (*cmpFn)(const void *, const void *), size_t (*hashFn)(const void *));

// returns HashBucket if key exists, NULL if not
struct HashBucket *MyHash_getBucket(struct MyHash *table, const void *key);

// returns NULL if key does not exist
void *MyHash_get(struct MyHash *table, const void *key);

// return old value if key exists, NULL if not
// old value is replaced
void *MyHash_set(struct MyHash *table, void *key, void *value);

// returns HashBucket if key exists, NULL if not
struct HashBucket *MyHash_delete(struct MyHash *table, const void *key);

void MyHash_resize(struct MyHash *table, size_t newSize);

// stores iterator into iter, and store the pointer to first entry into iter->it
// Don't modify the hash table when iterating
void MyHash_iterate(struct MyHash *table, struct MyHashIterator *iter);

// move to the next entry
void MyHash_next(struct MyHashIterator *iter);

// Default hash function, useful if the key is plain old C string ('\0' terminated)
size_t MyHash_strhash(const void *string);

// Default compare function, for C string keys
int MyHash_strcmp(const void *s1, const void *s2);

#endif
