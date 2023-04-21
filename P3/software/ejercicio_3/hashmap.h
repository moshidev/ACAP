/*
 * Source: https://github.com/merlinND/pthread-primes/blob/master/lib/hashmap.h
 *
 * Generic hashmap manipulation functions by Elliott C. Back.
 * Source: http://elliottback.com/wp/hashmap-implementation-in-c/
 */
#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <stdlib.h>
#include <stdint.h>

#define MAP_MISSING -3  /* No such element */
#define MAP_FULL -2   /* Hashmap is full */
#define MAP_OMEM -1   /* Out of Memory */
#define MAP_OK 0  /* OK */

/*
 * any_t is a pointer.  This allows you to put arbitrary structures in
 * the hashmap.
 */
typedef void *any_t;

/*
 * PFany is a pointer to a function that can take two any_t arguments
 * and return an integer. Returns status code..
 */
typedef int (*PFany)(any_t, any_t);

/*
 * map_t is a pointer to an internally maintained data structure.
 * Clients of this package do not need to know how hashmaps are
 * represented.  They see and manipulate only map_t's.
 */
typedef any_t map_t;

/*
 * Return an empty hashmap. Returns NULL if empty.
*/
map_t hashmap_new();

/*
 * Iteratively call f with argument (item, data) for
 * each element data in the hashmap. The function must
 * return a map status code. If it returns anything other
 * than MAP_OK the traversal is terminated. f must
 * not reenter any hashmap functions, or deadlock may arise.
 */
int hashmap_iterate(map_t in, PFany f, any_t item);

/*
 * Add an element to the hashmap. Return MAP_OK or MAP_OMEM.
 */
int hashmap_put(map_t in, uint64_t key, any_t value);

/*
 * Get an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
int hashmap_get(map_t in, uint64_t key, any_t *arg);

/*
 * Get an element from the hashmap. It is possible that it'll
 * return a MAP_MISSING when it isn't missing, as is thread usafe.
 */
int hashmap_get_thread_unsafe(map_t in, uint64_t key, any_t *arg);

/*
 * Free the hashmap
 */
void hashmap_free(map_t in);

/*
 * Get the current size of a hashmap
 */
int hashmap_length(map_t in);

#endif
