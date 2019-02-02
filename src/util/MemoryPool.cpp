#include "MemoryPool.h"

#include <iostream>
#include <stdlib.h>

struct Vec3 {
    float x,y,z;
};

void Test_MemoryPool(){



    Pool<float> p_f(1000);
    Pool<Vec3> p_vec3(10000);
    
    auto id = p_f.allocate();
    auto data = p_f.at(id);
    data = 42.24f;

    std::cout << p_f.at(id) << "\n";

    for(int i = 0; i < 5000; i++){
        Pool<float>::ID ids[100];
        for(int i = 0; i < 100; i++){
            ids[i] = p_f.allocate();
        }
        for(int i = 0; i < 100; i++){
            p_f.deallocate(ids[i]);
        }
    }
}