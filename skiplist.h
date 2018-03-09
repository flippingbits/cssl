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

#ifndef __CSSL_SkipList_H
#define __CSSL_SkipList_H

#define MAX_SKIP 5
// initial size of the highest fast lane with number
// of keys that fit into one cache line
#define TOP_LANE_BLOCK 16
// number of keys that can be stored in one SIMD register
#define SIMD_SEGMENTS 8

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include <assert.h>
#include <limits.h>
#include <math.h>

// data list node
typedef struct {
  uint32_t key;
  struct _CSSL_DataNode *next;
} _CSSL_DataNode;

// proxy node
typedef struct {
  uint32_t keys[MAX_SKIP];
  _CSSL_DataNode* pointers[MAX_SKIP];
  bool updated;
} _CSSL_ProxyNode;

typedef struct {
  uint8_t           max_level;
  uint8_t           skip;
  uint32_t          num_elements;
  uint32_t*         items_per_level;
  uint32_t*         flane_items;
  uint32_t*         starts_of_flanes;
  uint32_t*         flanes;
  _CSSL_ProxyNode** flane_pointers;
  _CSSL_DataNode    *head, *tail;
} _CSSL_SkipList;

// result of a range query
typedef struct {
  _CSSL_DataNode* start;
  _CSSL_DataNode* end;
  uint32_t        count;
} _CSSL_RangeSearchResult;

_CSSL_SkipList*         createSkipList(uint8_t maxLevel, uint8_t skip);
void                    insertElement(_CSSL_SkipList* slist, uint32_t key);
uint32_t                insertItemIntoFastLane(_CSSL_SkipList* slist,
                                               int8_t level,
                                               _CSSL_DataNode* newNode);
void                    buildFastLanes(_CSSL_SkipList* slist);
void                    resizeFastLanes(_CSSL_SkipList* slist);
uint32_t                searchElement(_CSSL_SkipList* slist, uint32_t key);
_CSSL_RangeSearchResult searchRange(_CSSL_SkipList* slist, uint32_t startKey, uint32_t endKey);
_CSSL_DataNode*         newNode(uint32_t key);
#endif
