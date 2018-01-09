#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MyHash.h"

static inline void OutOfMemory() {
  fprintf(stderr, "Out of memory!\n");
  exit(EXIT_FAILURE);
}

#define BASE_BUCKET_COUNT  8
#define MAX_BUCKET_COUNT 65536

void MyHash_init(struct MyHash *self, int (*cmpFn)(const void *, const void *), size_t (*hashFn)(const void *)) {
  self->_nb = BASE_BUCKET_COUNT;
  self->_size = 0;
  self->_cmp = cmpFn;
  self->_hf = hashFn;
  self->_buckets = calloc(self->_nb, sizeof(struct HashBucket));
  if (self->_buckets == NULL) OutOfMemory();
}

struct HashBucket *MyHash_getBucket(struct MyHash *self, const void *key) {
  size_t hash = self->_hf(key) % self->_nb;
  struct HashBucket *it = self->_buckets[hash];
  while (it != NULL) {
    if (self->_cmp(it->key, key) == 0) {
      return it;
    }
    it = it->next;
  }
  return NULL;
}

void *MyHash_get(struct MyHash *table, const void *key) {
  struct HashBucket *it = MyHash_getBucket(table, key);
  if (it == NULL) return NULL;
  return it->value;
}

void *MyHash_set(struct MyHash *self, void *key, void *value) {
  size_t hash = self->_hf(key) % self->_nb;
  struct HashBucket *it = self->_buckets[hash];
  while (it != NULL) {
    if (self->_cmp(it->key, key) == 0) { // item exists
      void *oldData = it->value;
      it->value = value;
      return oldData;
    }
    it = it->next;
  }
  // item does not exist
  struct HashBucket *new1 = malloc(sizeof (struct HashBucket));
  if (!new1) OutOfMemory();
  new1->key = key;
  new1->value = value;
  new1->next = self->_buckets[hash];
  self->_buckets[hash] = new1;
  self->_size++;
  if (self->_size > self->_nb * 2 && self->_nb * 2 < MAX_BUCKET_COUNT) {
    MyHash_resize(self, self->_nb * 2);
  }
  return NULL;
}

struct HashBucket *MyHash_delete(struct MyHash *self, const void *key) {
  size_t hash = self->_hf(key) % self->_nb;
  struct HashBucket *it = self->_buckets[hash], *prev = NULL;
  while (it != NULL) {
    if (self->_cmp(it->key, key) == 0) {
      if (prev == NULL) { // first in bucket
        self->_buckets[hash] = it->next;
      }
      else {
        prev->next = it->next;
      }
      it->next = NULL;
      self->_size--;
      return it;
    }
    prev = it;
    it = it->next;
  }
  return NULL;
}

void MyHash_resize(struct MyHash *self, size_t newSize) {
  struct HashBucket **bucks = self->_buckets;
  self->_buckets = calloc(newSize, sizeof(struct HashBucket));
  if (self->_buckets == NULL) OutOfMemory();
  size_t i, nb = self->_nb;
  self->_nb =newSize;
  for (i = 0; i < nb; i++) {
    struct HashBucket *it = bucks[i];
    while (it != NULL) {
      size_t hash = self->_hf(it->key) % self->_nb;
      struct HashBucket *next = it->next;
      it->next = self->_buckets[hash];
      self->_buckets[hash] = it;
      it = next;
    }
  }
  free(bucks);
}

void MyHash_iterate(struct MyHash *self, struct MyHashIterator *iter) {
  iter->table = self;
  size_t i;
  for (i = 0; i < self->_nb; i++) {
    if (self->_buckets[i] != NULL) break;
  }
  if (i < self->_nb) {
    iter->bucket = i;
    iter->it = self->_buckets[i];
  }
  else {
    iter->it = NULL; // hash table is empty
  }
}

void MyHash_next(struct MyHashIterator *iter) {
  if (iter->it == NULL) return ; // end of iteration
  if (iter->it->next == NULL) {
    struct MyHash *tb = iter->table;
    size_t nbuckets = tb->_nb, i = iter->bucket + 1;
    while (i < nbuckets && tb->_buckets[i] == NULL) {
      i++;
    }
    if (i <nbuckets) {
      iter->bucket = i;
      iter->it = tb->_buckets[i];
    }
    else {
      iter->it = NULL; // hash table is empty
    }
  }
  else {
    iter->it = iter->it->next;
  }
}

size_t MyHash_strhash(const void *string) {
  const char *str = string;
  size_t n = strlen(str), i;
  size_t hash = 88487;
  for (i = 0; i < n; i++) {
    hash = (hash << 5) + hash + (size_t) str[i];
  }
  return hash;
}

int MyHash_strcmp(const void *s1, const void *s2) {
  const char *str1 = s1, *str2 = s2;
  return strcmp(s1, s2);
}
