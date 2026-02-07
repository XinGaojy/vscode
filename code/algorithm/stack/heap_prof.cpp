// heap_prof.cpp
#include <vector>
#include <unistd.h>
#include<google/profiler.h>
void leak1(){
    for (int i = 0; i < 1e6; ++i) new int[100]; // 40 MB
}
void leak2(){
    for (int i = 0; i < 1e6; ++i) new double[50]; // 40 MB
}

int main(){
    ProfilerStart("cpu.prof");
    leak1();
    leak2();
    ProfilerStop();
    return 0;
}
