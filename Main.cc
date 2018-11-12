#include <iostream>
#include "Session.h"
#include "Loop.h"

#define CATCH_CONFIG_MAIN

#ifndef CATCH_CONFIG_MAIN

using namespace std;


int main () {
    LoopEvent loopEvent; 
    if (loopEvent.init()) {
        loopEvent.run(); 
    } 
    else {

    }
    return 0;
}

#endif

#ifdef CATCH_CONFIG_MAIN
#include "unittest/UnitTest.h"
#endif
