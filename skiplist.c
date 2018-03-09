/*
 * Copyright 2017 Stefan Sprenger <sprengsz@informatik.hu-berlin.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "skiplist.h"

// Creates a new skip list
_CSSL_SkipList* createSkipList(uint8_t maxLevel, uint8_t skip) {
  _CSSL_SkipList* slist = malloc(sizeof(*slist));

  slist->max_level        = maxLevel;
  slist->num_elements     = 0;
  slist->head             = newNode(0);
  slist->tail             = slist->head;
  slist->skip             = skip > 1 ? skip : 2;
  slist->items_per_level  = malloc(sizeof(uint32_t) * maxLevel);
  slist->starts_of_flanes = malloc(sizeof(uint32_t) * maxLevel);
  slist->flane_items      = malloc(sizeof(uint32_t) * maxLevel);

  for (uint8_t level = 0; level < maxLevel; level++)
    slist->flane_items[level] = 0;

  buildFastLanes(slist);
  srand(0);

  return slist;
}

// Creates a new proxy node in the given skip list
_CSSL_ProxyNode* newProxyNode(_CSSL_SkipList* slist, _CSSL_DataNode* node) {
  _CSSL_ProxyNode* proxy = malloc(sizeof(*proxy));
  proxy->keys[0] = node->key;
  proxy->updated = false;

  for (uint8_t i = 1; i < slist->skip; i++)
    proxy->keys[i] = INT_MAX;
  proxy->pointers[0] = node;

  return proxy;
}

// Adds a new element to the corresponding proxy lane in the given skip list
void findAndInsertIntoProxyNode(_CSSL_SkipList* slist, _CSSL_DataNode* node) {
  _CSSL_ProxyNode* proxy = slist->flane_pointers[slist->flane_items[0] - 1];

  for (uint8_t i = 1; i < slist->skip; i++) {
    if (proxy->keys[i] == INT_MAX) {
      proxy->keys[i] = node->key;
      proxy->pointers[i] = node;
      return;
    }
  }
}

// Inserts a new element into the given skip list (bulk insert)
void insertElement(_CSSL_SkipList* slist, uint32_t key) {
  _CSSL_DataNode *new_node = newNode(key);
  bool nodeInserted = true;
  bool flaneInserted = false;

  // add new node at the end of the data list
  slist->tail->next = (struct _CSSL_DataNode*) new_node;
  slist->tail = new_node;

  // add key to fast lanes
  for (uint8_t level = 0; level < slist->max_level; level++) {
    if (slist->num_elements % (uint32_t) pow(slist->skip, (level + 1)) == 0 &&
        nodeInserted)
      nodeInserted = insertItemIntoFastLane(slist, level, new_node);
    else
      break;
    flaneInserted = true;
  }

  if (!flaneInserted)
    findAndInsertIntoProxyNode(slist, new_node);

  slist->num_elements++;

  // resize fast lanes if more space is needed
  if (slist->num_elements %
      (TOP_LANE_BLOCK*((uint32_t) pow(slist->skip, slist->max_level))) == 0)
    resizeFastLanes(slist);
}

// Inserts a given key into a fast lane at the given level
uint32_t insertItemIntoFastLane(_CSSL_SkipList* slist, int8_t level, _CSSL_DataNode* newNode) {
  uint32_t curPos = slist->starts_of_flanes[level] + slist->flane_items[level];
  uint32_t levelLimit = curPos + slist->items_per_level[level];

  if (curPos > levelLimit)
    curPos = levelLimit;

  while (newNode->key > slist->flanes[curPos] && curPos < levelLimit)
    curPos++;

  if (slist->flanes[curPos] == INT_MAX) {
    slist->flanes[curPos] = newNode->key;
    if (level == 0)
      slist->flane_pointers[curPos - slist->starts_of_flanes[0]] =
                                                   newProxyNode(slist, newNode);
    slist->flane_items[level]++;
  } else {
    return INT_MAX;
  }

  return curPos;
}

// Build fast lanes
void buildFastLanes(_CSSL_SkipList* slist) {
  uint32_t flane_size = TOP_LANE_BLOCK;

  slist->items_per_level[slist->max_level - 1]  = flane_size;
  slist->starts_of_flanes[slist->max_level - 1] = 0;

  // calculate level sizes level by level
  for (int8_t level = slist->max_level - 2; level >= 0; level--) {
    slist->items_per_level[level]  = slist->items_per_level[level + 1] *
                                       slist->skip;
    slist->starts_of_flanes[level] = slist->starts_of_flanes[level + 1 ] +
                                       slist->items_per_level[level + 1];
    flane_size += slist->items_per_level[level];
  }

  slist->flanes = malloc(sizeof(uint32_t) * flane_size);
  slist->flane_pointers = malloc(sizeof(_CSSL_ProxyNode*) * slist->items_per_level[0]);
  // initialize arrays with placeholder values
  for (uint32_t i = 0; i < flane_size; i++)
    slist->flanes[i] = INT_MAX;
  for (uint32_t i = 0; i < slist->items_per_level[0]; i++)
    slist->flane_pointers[i] = NULL;
}

// Increase size of existing fast lanes of a given skip list
void resizeFastLanes(_CSSL_SkipList* slist) {
  uint32_t new_size = slist->items_per_level[slist->max_level - 1] +
                        TOP_LANE_BLOCK;
  uint32_t* level_items = malloc(sizeof(uint32_t) * slist->max_level);
  uint32_t* level_starts = malloc(sizeof(uint32_t) * slist->max_level);

  level_items[slist->max_level - 1]  = new_size;
  level_starts[slist->max_level - 1] = 0;

  for (int8_t level = slist->max_level - 2; level >= 0; level--) {
    level_items[level] = level_items[level + 1] * slist->skip;
    level_starts[level] = level_starts[level + 1] + level_items[level + 1];
    new_size += level_items[level];
  }

  uint32_t* new_flanes = malloc(sizeof(uint32_t) * new_size);
  _CSSL_ProxyNode** new_fpointers = malloc(sizeof(_CSSL_ProxyNode*) * level_items[0]);

  for (uint32_t i = slist->flane_items[slist->max_level - 1]; i < new_size; i++) {
    new_flanes[i] = INT_MAX;
  }

  // copy from old flane to new flane
  for (int8_t level = slist->max_level - 1; level >= 0; level--) {
    memcpy(&new_flanes[level_starts[level]],
           &slist->flanes[slist->starts_of_flanes[level]],
           sizeof(uint32_t) * slist->items_per_level[level]);
  }
  memcpy(&new_fpointers[0],
         &slist->flane_pointers[0],
         sizeof(_CSSL_ProxyNode*) * slist->items_per_level[0]);

  free(slist->flanes);
  free(slist->flane_pointers);
  free(slist->items_per_level);
  free(slist->starts_of_flanes);
  slist->flanes = new_flanes;
  slist->flane_pointers = new_fpointers;
  slist->items_per_level = level_items;
  slist->starts_of_flanes = level_starts;
}

// Single-key lookup on a given skip list
uint32_t searchElement(_CSSL_SkipList* slist, uint32_t key) {
  uint32_t curPos = 0;
  uint32_t first = 0;
  uint32_t last = slist->items_per_level[slist->max_level - 1] - 1;
  uint32_t middle = 0;
  // scan highest fast lane with binary search
  while (first < last) {
    middle = (first + last) / 2;
    if (slist->flanes[middle] < key) {
      first = middle + 1;
    } else if (slist->flanes[middle] == key) {
      curPos = middle;
      break;
    } else {
      last = middle;
    }
  }
  if (first > last)
    curPos = last;
  int level;
  // traverse over fast lanes
  for (level = slist->max_level - 1; level >= 0; level--) {
    uint32_t rPos = curPos - slist->starts_of_flanes[level];
    while (rPos < slist->items_per_level[level] &&
           key >= slist->flanes[++curPos])
      rPos++;
    if (level == 0) break;
    curPos  = slist->starts_of_flanes[level - 1] + rPos * slist->skip;
  }
  if (key == slist->flanes[--curPos])
    return key;

  _CSSL_ProxyNode* proxy = slist->flane_pointers[curPos - slist->starts_of_flanes[0]];
  for (uint8_t i = 1; i < slist->skip; i++) {
    if (proxy->keys[i] == key)
      return key;
  }

  // search on data lane if proxy->skips_data == true
  return INT_MAX;
}

// Range query on a given skip list using range boundaries startKey and endKey
_CSSL_RangeSearchResult searchRange(_CSSL_SkipList* slist, uint32_t startKey, uint32_t endKey) {
  // use the cache to determine the section of the first fast lane that
  // should be used as starting position for search
  __m256 avx_creg, res, avx_sreg;
  _CSSL_RangeSearchResult result;
  int level, bitmask;
  uint32_t curPos = 0;
  uint32_t rPos = 0;
  uint32_t first = 0;
  uint32_t last = slist->items_per_level[slist->max_level - 1] - 1;
  uint32_t middle = 0;
  // scan highest fast lane with binary search
  while (first < last) {
    middle = (first + last) / 2;
    if (slist->flanes[middle] < startKey) {
      first = middle + 1;
    } else if (slist->flanes[middle] == startKey) {
      curPos = middle;
      break;
    } else {
      last = middle;
    }
  }
  if (first > last)
    curPos = last;

  for (level = slist->max_level - 1; level >= 0; level--) {
    rPos = curPos - slist->starts_of_flanes[level];
    while (rPos < slist->items_per_level[level] &&
           startKey >= slist->flanes[++curPos]) {
      rPos++;
    }
    if (level == 0) break;
    curPos  = slist->starts_of_flanes[level - 1] + rPos * slist->skip;
  }
  uint32_t start_of_flane = slist->starts_of_flanes[0];
  while (startKey < slist->flanes[curPos] && curPos > start_of_flane) {
    curPos--;
  }

  result.count = 0;

  _CSSL_ProxyNode* proxy = slist->flane_pointers[curPos - slist->starts_of_flanes[0]];
  result.start = (_CSSL_DataNode*) proxy->pointers[slist->skip - 1]->next;
  for (uint8_t i = 0; i < slist->skip; i++) {
    if (startKey <= proxy->keys[i]) {
      result.start = proxy->pointers[i];
      break;
    }
  }

  // search for the range's last matching node
  avx_sreg = _mm256_castsi256_ps(_mm256_set1_epi32(endKey));
  uint32_t itemsInFlane = slist->items_per_level[0] - SIMD_SEGMENTS;
  rPos = curPos - start_of_flane;
  while (rPos < itemsInFlane) {
    avx_creg = _mm256_castsi256_ps(
        _mm256_loadu_si256((__m256i const *) &slist->flanes[curPos]));
    res      = _mm256_cmp_ps(avx_sreg, avx_creg, 30);
    bitmask  = _mm256_movemask_ps(res);
    if (bitmask < 0xff) break;
    curPos += SIMD_SEGMENTS; rPos += SIMD_SEGMENTS;
    result.count += (SIMD_SEGMENTS *  slist->skip);
  }
  curPos--;
  rPos--;
  itemsInFlane += SIMD_SEGMENTS;

  while (endKey >= slist->flanes[++curPos] && rPos < itemsInFlane) {
    rPos++;
  }

  proxy = slist->flane_pointers[rPos];
  result.end = proxy->pointers[slist->skip - 1];
  for (uint8_t i = 1; i < slist->skip; i++) {
    if (endKey < proxy->keys[i]) {
      result.end = proxy->pointers[i - 1];
      break;
    }
  }

  return result;
}

// Creates a new node
_CSSL_DataNode* newNode(uint32_t key) {
  _CSSL_DataNode* node = malloc(sizeof(*node));
  node->key  = key;
  node->next = NULL;

  return node;
}
