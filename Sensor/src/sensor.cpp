#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <thread>
#include <chrono>
#if defined(WRITE_TO_FILE) || defined(READ_FROM_FILE)
#include <fstream>
#include <sys/stat.h>
#endif

#include "GimbaledSensor.hpp"


//******************************************************************************

//******************************************************************************
int main(int argc, char *argv[]) {

    sleep(1);

    //GimbaledSensor* gs = new GimbaledSensor();
    std::thread sensor(GimbaledSensor(), 1);
    sensor.join();
    sleep(2);

    printf("Done\n");
    return 0;
}
