

#include <sys/time.h>
#include "osm.h"
#include <cmath>


double osm_operation_time(unsigned int iterations){
    timeval start{},finish{};
    int x = 0;
    unsigned int a = 0;
    //sample start time
    if(gettimeofday(&start, nullptr) == -1){
        return -1;
    }
    //make the simple arithmetic operation many times
    while(a < iterations){
               x +=1;
               x +=1;
               x +=1;
               x +=1;
               x +=1;
               x +=1;
               x +=1;
               x +=1;
               x +=1;
               a += 10;
    }
    //sample finish time
    if(gettimeofday(&finish, nullptr) == -1){
        return -1;
    }

    //calc the time of the operation in nano-sec
    double timeofoperetion = (finish.tv_usec - start.tv_usec) * pow(10, 3);

    //return the time of the operation in nanoseconds
    return timeofoperetion;
}

void empty_func(){}

double osm_function_time(unsigned int iterations){
    timeval start{},finish{};
    //sample start time
    if(gettimeofday(&start, nullptr) == -1){
        return -1;
    }
    //make the simple arithmetic operation many times
    for (unsigned int i = 0; i < iterations; i+=10) {
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();
    }
    //sample finish time
    if(gettimeofday(&finish, nullptr) == -1){
        return -1;
    }

    //calc the time of the operation in nano-sec
    double timeofoperetion = (finish.tv_usec - start.tv_usec) * pow(10, 3);

    //return the time of the operation in nanoseconds
    return timeofoperetion;
}

double osm_syscall_time(unsigned int iterations){
    timeval start{},finish{};
    //sample start time
    if(gettimeofday(&start, nullptr) == -1){
        return -1;
    }
    //make the simple arithmetic operation many times
    for (unsigned int i = 0; i < iterations; i+=10) {
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
    }
    //sample finish time
    if(gettimeofday(&finish, nullptr) == -1){
        return -1;
    }

    //calc the time of the operation in nano-sec
    double timeofoperetion = (finish.tv_usec - start.tv_usec) * pow(10, 3);

    //return the time of the operation in nanoseconds
    return timeofoperetion;
}


