#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include "defs.h"
#include "list.h"
#include "strings.h"
#include "sorts.h"
#include "chunk.h"
#include "utils.h"

static void *
graph_thread_handler(void *arg)
{
    char *uargv;
    
    while (1) {
        
    }
    return uargv;
}

int 
main (int argc, char *argv[]) 
{
    pthread_t graph_thread;
    
    /*
     * Utility functions
     */
    util_unit_test(UTIL_OP_NONE);
    
    /*  
     * Tree operations
     */
    list_unit_test(LIST_OP_NONE);

    /*
     * String operations
     */
    string_unit_test(STRING_OP_NONE);

    /*
     * Sorting.
     */
    sort_unit_test(SORT_OP_ALL);

    pthread_create(&graph_thread, NULL, graph_thread_handler, NULL);

    /*
     * The main thread will be suspended until the graph_thread terminates.
     */
    pthread_join(graph_thread, NULL);

    return 0;
}
