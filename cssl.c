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

#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "skiplist.h"

static double gettime(void) {
  struct timeval now_tv;
  gettimeofday (&now_tv,NULL);
  return ((double)now_tv.tv_sec) + ((double)now_tv.tv_usec) / 1000000.0;
}

int compare_ints(const void *a, const void *b) {
  const uint32_t *da = (const uint32_t *) a;
  const uint32_t *db = (const uint32_t *) b;

  return (*da > *db) - (*da < *db);
}

void shuffle_array(uint32_t* arr, uint32_t size) {
  uint32_t rnd, tmp;
  for (uint32_t i = 0; i < size; i++) {
    rnd = rand() % size;
    tmp = arr[rnd]; arr[rnd] = arr[i]; arr[i] = tmp;
  }
}

int main( int argc, const char* argv[] ) {
  if (argc != 3) {
    printf("Usage: %s num_elements 0|1 (0=dense, 1=sparse)\n", argv[0]);
    return 1;
  }

  SkipList* slist = createSkipList(9, 5);
  uint32_t n = atoi(argv[1]);
  uint32_t* keys = malloc(sizeof(uint32_t) * n);

  if (atoi(argv[2]) == 0) {
    // dense integers
    for(uint32_t i = 0; i < n; i++)
      keys[i] = i + 1;
  } else {
    // sparse integers
    for (uint32_t i = 0; i < n; i++)
      keys[i] = (rand() % (INT_MAX/2-1)) + 1;
    qsort(keys, n, sizeof(uint32_t), compare_ints);
  }

  // Insert keys into CSSL
  double start = gettime();
  for (uint32_t i = 0; i < n; i++)
    insertElement(slist, keys[i]);
  printf("Insertion: %d ops/s.\n", (int) (n / (gettime() - start)));

  // Execute a large amount of single-key lookups using random search keys
  shuffle_array(keys, n);
  uint16_t repeat=100000000/n;
  if (repeat < 1) repeat = 1;
  start = gettime();
  for (uint16_t r = 0; r < repeat; r++) {
    for (uint32_t i = 0; i < n; i++)
      assert(searchElement(slist, keys[i]) == keys[i]);
  }
  printf("Lookup:    %d ops/s.\n", (int) (n*repeat / (gettime() - start)));

  uint32_t m = 1000000;
  uint32_t r_size = n / 10;
  uint32_t* rkeys = malloc(sizeof(uint32_t) * m);
  for(int i = 0; i < m; i++)
    rkeys[i] = keys[rand() % n];
  start = gettime();
  for (int i = 0; i < m; i++) {
    RangeSearchResult res = searchRange(slist, rkeys[i], (rkeys[i] + r_size));
    assert(res.start->key >= rkeys[i] && res.end->key <= (rkeys[i] + r_size));
  }
  printf("Range:     %d ops/s. (Range size: %d)\n",
         (int) (m / (gettime() - start)), r_size);

  return 0;
}
