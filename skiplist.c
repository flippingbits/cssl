#include "skiplist.h"

// Creates a new skip list
SkipList* createSkipList(uint8_t maxLevel, uint8_t skip) {
  SkipList* slist = malloc(sizeof(*slist));

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

// Appends a given element at the end of CSSL (may be used for bulk inserting a pre-sorted list of keys)
void bulkInsertElement(SkipList* slist, uint32_t key) {
  DataNode *new_node = newNode(key);
  bool nodeInserted = true;

  slist->tail->next = (struct DataNode*) new_node;
  slist->tail = new_node;

  // add key to fast lanes
  for (uint8_t level = 0; level < slist->max_level; level++) {
    if (slist->num_elements % (uint32_t) pow(slist->skip, (level + 1)) == 0 &&
        nodeInserted)
      nodeInserted = insertItemIntoFastLane(slist, level, new_node);
    else
      break;
  }

  slist->num_elements++;

  // resize fast lanes if more space is needed
  if (slist->items_per_level[0] - slist->flane_items[0] < 5)
    resizeFastLanes(slist);
}

// Inserts a new element into the given skip list (bulk insert)
void insertElement(SkipList* slist, uint32_t key) {
  DataNode *new_node = newNode(key);
  bool nodeInserted = true;

  // TODO: Use fast lanes to find insert position
  DataNode *cur = slist->head;
  uint32_t insert_pos = 0;
  while (cur->next != NULL && key > ((DataNode*) cur->next)->key) {
    cur = (DataNode*) cur->next;
    insert_pos++;
  }
  new_node->next = cur->next;
  cur->next = (struct DataNode*) new_node;

  if (slist->tail == cur)
    slist->tail = new_node;

  // add key to fast lanes
  for (uint8_t level = 0; level < slist->max_level; level++) {
    if (insert_pos % (uint32_t) pow(slist->skip, (level + 1)) == 0 && nodeInserted)
      nodeInserted = insertItemIntoFastLane(slist, level, new_node);
    else
      break;
  }

  slist->num_elements++;

  // resize fast lanes if more space is needed
  if (slist->items_per_level[0] - slist->flane_items[0] < 5)
    resizeFastLanes(slist);
}

// Inserts a given key into a fast lane at the given level
uint32_t insertItemIntoFastLane(SkipList* slist, int8_t level, DataNode* newNode) {
  uint32_t curPos = slist->starts_of_flanes[level];
  uint32_t levelLimit = curPos + slist->items_per_level[level];

  if (curPos > levelLimit)
    curPos = levelLimit;

  while (newNode->key > slist->flanes[curPos] && curPos < levelLimit)
    curPos++;

  if (slist->flanes[curPos] == INT_MAX) {
    slist->flanes[curPos] = newNode->key;
    if (level == 0)
      slist->flane_pointers[curPos - slist->starts_of_flanes[0]] = newNode;
    slist->flane_items[level]++;
  } else if (slist->flane_items[level] < slist->items_per_level[level]) {
    uint32_t shift_pos = slist->starts_of_flanes[level] +
      slist->flane_items[level];
    while (shift_pos > curPos) {
      slist->flanes[shift_pos] = slist->flanes[shift_pos - 1];
      if (level == 0)
        slist->flane_pointers[shift_pos - slist->starts_of_flanes[0]] =
          slist->flane_pointers[shift_pos - slist->starts_of_flanes[0] - 1];
      shift_pos--;
    }
    slist->flanes[curPos] = newNode->key;
    if (level == 0)
      slist->flane_pointers[curPos - slist->starts_of_flanes[0]] = newNode;
    slist->flane_items[level]++;
  } else
    return INT_MAX;
  return curPos;
}

// Build fast lanes
void buildFastLanes(SkipList* slist) {
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
  slist->flane_pointers = malloc(sizeof(DataNode*) * slist->items_per_level[0]);
  // initialize arrays with placeholder values
  for (uint32_t i = 0; i < flane_size; i++)
    slist->flanes[i] = INT_MAX;
  for (uint32_t i = 0; i < slist->items_per_level[0]; i++)
    slist->flane_pointers[i] = NULL;
}

// Increase size of existing fast lanes of a given skip list
void resizeFastLanes(SkipList* slist) {
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
  DataNode** new_fpointers = malloc(sizeof(DataNode*) * level_items[0]);

  for (uint32_t i = slist->flane_items[slist->max_level - 1]; i < new_size; i++)
    new_flanes[i] = INT_MAX;

  DataNode *cur = (DataNode*) slist->head->next;
  uint32_t i    = 0;
  uint32_t j    = level_starts[0];
  while (cur != NULL) {
    if (i % slist->skip == 0) {
      new_flanes[j] = cur->key;
      new_fpointers[i / slist->skip] = cur;
      j++;
    }
    cur = (DataNode*) cur->next;
    i++;
  }

  for (int8_t level = 1; level < slist->max_level; level++) {
    i = level_starts[level - 1];
    j = level_starts[level];
    for (uint32_t k = 0; k < level_items[level - 1]; k++) {
      if (k % slist->skip == 0)
        new_flanes[j++] = new_flanes[i];
      i++;
    }
  }

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
uint32_t searchElement(SkipList* slist, uint32_t key) {
  uint32_t curPos = 0;
  uint32_t first = 0;
  uint32_t last = slist->items_per_level[slist->max_level - 1] - 1;
  uint32_t middle = (first + last) / 2;
  // scan highest fast lane with binary search
  while (first <= last) {
	  if (slist->flanes[middle] < key)
		  first = middle + 1;
	  else if (slist->flanes[middle] == key) {
		  curPos = middle;
		  break;
	  }
	  else
		  last = middle - 1;
	  middle = (first + last) / 2;
  }
  if (first > last)
    curPos = last;
  uint32_t level;
  // traverse over fast lanes
  for (level = slist->max_level - 1; level >= 0; level--) {
    uint32_t rPos = curPos - slist->starts_of_flanes[level];
    if (slist->flanes[curPos] > key)
      while (rPos > 0 &&
          key < slist->flanes[--curPos])
        rPos--;
    else
      while (rPos < slist->items_per_level[level] &&
          key >= slist->flanes[++curPos])
        rPos++;
    if (level == 0) break;
    curPos  = slist->starts_of_flanes[level - 1] + rPos * slist->skip;
  }
  if (key == slist->flanes[--curPos])
    return key;

  DataNode *cur = slist->flane_pointers[curPos - slist->starts_of_flanes[0]];
  while (cur != NULL && key > cur->key)
    cur = (DataNode*) cur->next;
  if (key == cur->key)
    return key;

  return INT_MAX;
}

// Range query on a given skip list using range boundaries startKey and endKey
RangeSearchResult searchRange(SkipList* slist, uint32_t startKey, uint32_t endKey) {
  // use the cache to determine the section of the first fast lane that
  // should be used as starting position for search
  __m256 avx_creg, res, avx_sreg;
  RangeSearchResult result;
  uint32_t level, bitmask;
  uint32_t curPos = 0; uint32_t rPos = 0; uint32_t first = 0;
  uint32_t last = slist->items_per_level[slist->max_level - 1] - 1;
  uint32_t middle = (first + last) / 2;
  while (first <= last) {
	  if (slist->flanes[middle] < startKey) {
		  first = middle + 1;
    } else if (slist->flanes[middle] == startKey) {
		  curPos = middle;
		  break;
	  } else {
		  last = middle - 1;
    }
	  middle = (first + last) / 2;
  }
  if (first > last)
    curPos = last;

  for (level = slist->max_level - 1; level >= 0; level--) {
    rPos = curPos - slist->starts_of_flanes[level];
    if (slist->flanes[curPos] > startKey)
      while (rPos > 0 &&
          startKey < slist->flanes[--curPos])
        rPos--;
    else
      while (rPos < slist->items_per_level[level] &&
          startKey >= slist->flanes[++curPos])
        rPos++;
    if (level == 0) break;
    curPos  = slist->starts_of_flanes[level - 1] + rPos * slist->skip;
  }
  uint start_of_flane = slist->starts_of_flanes[0];
  while (startKey < slist->flanes[curPos] && curPos > start_of_flane)
    curPos--;

  result.count = 0;

  DataNode* cur = slist->flane_pointers[curPos - slist->starts_of_flanes[0]];
  while (cur != NULL && startKey > cur->key)
    cur = (DataNode*) cur->next;
  result.start = cur;

  // search for the range's last matching node
  avx_sreg = _mm256_castsi256_ps(_mm256_set1_epi32(endKey));
  uint itemsInFlane = slist->items_per_level[0] - SIMD_SEGMENTS;
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
  curPos--;rPos--;
  itemsInFlane += SIMD_SEGMENTS;

  if (rPos < 0) {
    rPos = 0;
    curPos = start_of_flane;
  }

  while (endKey >= slist->flanes[++curPos] && rPos < itemsInFlane)
    rPos++;

  cur = slist->flane_pointers[rPos];
  while (cur != NULL && endKey >= cur->key) {
    result.end = cur;
    cur = (DataNode*) cur->next;
  }

  return result;
}

// Creates a new node
DataNode* newNode(uint32_t key) {
  DataNode* node = malloc(sizeof(*node));
  node->key  = key;
  node->next = NULL;

  return node;
}
