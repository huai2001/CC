/*
 * A unit test and example of how to use the simple C hashmap
 */

#include <stdlib.h>
#include <stdio.h>
#include "map.h"

#define KEY_PREFIX ("simple C hashmap - ")
#define KEY_COUNT (1024000)

#if TESTES
/**/
bool_t _hashmap_equals(const pvoid_t custom, const pvoid_t keyword) {
    _map_t *u = (_map_t*)custom;
#if TESTES_STRING
    int n = _stricmp(u->key_string, (char*)keyword);
    //printf("%s == %s %d\n",u->key_string, (char*)keyword, n);
    return n == 0;
#else
    return (*(uint32_t*)keyword == u->number);
#endif
}

/*
 * Hashing function for a string
 */
static uint32_t hashmap_build_hash(const pvoid_t keyword) {
#if TESTES_STRING
    tchar_t* str = (tchar_t*)keyword;
    uint32_t key = _cc_crc32((byte_t*)str, (uint32_t)_tcslen(str) * sizeof(tchar_t));
#else
    uint32_t key = *(uint32_t*)keyword;
 #endif
    
//     /* Robert Jenkins' 32 bit Mix Function */
//     key += (key << 12);
//     key ^= (key >> 22);
//     key += (key << 4);
//     key ^= (key >> 9);
//     key += (key << 10);
//     key ^= (key >> 2);
//     key += (key << 7);
//     key ^= (key >> 12);
// 
//     /* Knuth's Multiplicative Method */
//     return (key >> 3) * 2654435761UL;
	return key;
}

#endif

int main (int argc, char * const argv[]) {
    int index;
    _map_t* value;
#if TESTES_STRING
    char key_string[KEY_MAX_LENGTH];
#endif
    
    clock_t start, end;
    
#if TESTES
    _cc_hashtable_t* mymap = NULL;
#else
    _cc_rbtree_t mymap;
#endif

#if TESTES
    mymap = _cc_create_hashtable(0, _hashmap_equals, hashmap_build_hash);
#else
    mymap.rb_node = NULL;
#endif
    start = clock();
    /* First, populate the hash map with ascending values */
    for (index = 0; index < KEY_COUNT; index += 1) {
        value = (_map_t*)_cc_malloc(sizeof(_map_t));
		value->number = index;
#if TESTES_STRING
        /* Store the key string along side the numerical value so we can free it later */
        _snprintf(value->key_string, KEY_MAX_LENGTH, "%s-%d", KEY_PREFIX, index);
        #if TESTES
        if(!_cc_hashtable_insert(mymap, value->key_string, value)) {
            printf("%d - put fail.\n", value->number);
        }
        #else
        if(put(&mymap, value->key_string, value) == 0){
            printf("%d - put fail.\n", value->number);
        }
        #endif
#else
#if TESTES
        if(!_cc_hashtable_insert(mymap, &value->number, value)) {
            //printf("%d - put fail.\n",index);
        }
#else
        if(put(&mymap, value->number, value) == 0){
            printf("%d - put fail.\n", index);
        }
#endif
#endif
    }


    end = clock();
    printf("insert time span: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    start = clock();
    /* Now, check all of the expected values are there */
    
    for (index = KEY_COUNT - 1; index >= 0; index -= 1) {
#if TESTES_STRING
        _snprintf(key_string, KEY_MAX_LENGTH, "%s-%d", KEY_PREFIX, index);
        #if TESTES
        if(_cc_hashtable_find(mymap, key_string) == NULL) {
            printf("%s - Not found\n", key_string);
        }
        #else
        if(get(&mymap, key_string) == NULL) {
            printf("%s - Not found\n", key_string);
        }
        #endif
#else
#if TESTES
        if(_cc_hashtable_find(mymap, &index) == NULL) {
            printf("%d - Not found\n", index);
        }
#else
        if(get(&mymap, index) == NULL) {
            printf("%d - Not found\n", index);
        }
#endif
#endif
    }

    end = clock();
    printf("finded time span: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    

    start = clock();

    for (index = 0; index < KEY_COUNT; index += 1) {
#if TESTES_STRING
        value = NULL;
        _snprintf(key_string, KEY_MAX_LENGTH, "%s-%d", KEY_PREFIX, index);
#if TESTES
        value = (_map_t*)_cc_hashtable_remove(mymap, &key_string);
#else
        value = map_erase(&mymap, key_string);
#endif
        if (value) {
            if (index != value->number)
                printf("%d - %s\n", value->number, value->key_string);
            _cc_free(value);
        } else {
            printf("%s - Not found\n", key_string);
        }
#else
#if TESTES
        value = (_map_t*)_cc_hashtable_remove(mymap, &index);
#else
        value = map_erase(&mymap, index);
#endif
        if (value) {
            _cc_free(value);
        } else {
            printf("%d - Not found\n", index);
        }
#endif
        
    }
    end = clock();
    printf("delete time span: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
 
 
    start = clock();
#if TESTES
    /* Now, destroy the map */
    _cc_destroy_hashtable(&mymap);
#else
    map_destroy(&mymap);
#endif
    end = clock();
    printf("destroy time span: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    return 1;
}
