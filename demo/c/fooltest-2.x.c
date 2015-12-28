/*
   ============================================================================
Name        : fooltest-2.x.c
Author      : LIU Lu
Version     :
Copyright   : Department of Cartography, TUM
Description : Fool test for mmspa library v2.x
============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mmspa4pg/mmspa4pg.h"

int main(void) {
    /* 50 sources selected from the underground network randomly by another program for test */
    const char *testSources[] = {
        "122786152450", "122786152495", "122786152501", "121533104185",
        "122786152522", "122786152527", "122786152534", "121742819427",
        "121742819429", "121742819430", "121742819431", "121742819432",
        "121742819433", "121742819436", "121333874798", "121333874800",
        "121333874801", "121399935113", "121399935115", "121399935116",
        "121399935124", "12449218585", "122598107848", "12193890484",
        "123148959929", "122425442536", "12357468416", "12357468420",
        "121195462952", "121195462954", "12160336172", "12160336180",
        "12160336185", "121195462972", "12296651086", "12296651087",
        "12296651088", "12366905739", "121837958246", "122935928433",
        "122041663925", "12654215609", "12654215611", "12654215613",
        "12654215615", "12654215618", "12654215623", "12654215625",
        "12654215638", "12366905819"
    };

    const int64_t testDrivingSource = 11618163561; 
    const int64_t testWalkingSource = 123256574726; 
    const int64_t testTarget = 12672741190;
    /* change the PostgreSQL connection string according to your own config */
    const char *conninfo = "dbname = 'testdb' user = 'liulu' password = 'workhard'";
    if (MSPinit(conninfo) != EXIT_SUCCESS) {
        printf("Init mmspa library failed.\n");
        return EXIT_FAILURE;
    }
    printf("mmspa lib initialized. \n");
    printf("Creating multimodal routing plans... ");
    //CreateRoutingPlan(1, 1);
    MSPcreateRoutingPlan(2, 0);
    MSPsetMode(0, PRIVATE_CAR);
    MSPsetMode(1, FOOT);
    //SetPublicTransitModeSetItem(0, UNDERGROUND);
    MSPsetSwitchCondition(0, "type_id=91 AND is_available=TRUE");
    MSPsetCostFactor("speed");
    MSPsetTargetConstraint(NULL);
    printf("done! \n");
    struct timeval start_time, finish_time;
    float duration = 0.0, averageTime = 0.0;
    /* read graphs */
    printf("Constructing multimodal graphs from PostgreSQL database... ");
    gettimeofday(&start_time, NULL);
    if (MSPassembleGraphs() != EXIT_SUCCESS) { 
        printf("read graphs failed \n");
        return EXIT_FAILURE;
    }
    gettimeofday(&finish_time, NULL);
    duration = (double) ((finish_time.tv_sec * 1000000 + finish_time.tv_usec) - (start_time.tv_sec * 1000000 + start_time.tv_usec)) / 1000000;
    printf("done! \n");
    printf(
            "Multimodal graph construction:   %8.4f   seconds\n",
            duration);
    printf("Calculating multimodal routes...\n");
    printf("Test times: 50\n");
    gettimeofday(&start_time, NULL);
    Path **finalPath = MSPfindPath(testDrivingSource, testTarget);
    double final_cost = 0.0;
    final_cost = MSPgetFinalCost(testTarget, "distance");
    printf("Final cost is: %f\n", final_cost);
    //	for (int i = 0; i < 50; i++)
    //		MultimodalTwoQ(atoll(testSources[i]));
    gettimeofday(&finish_time, NULL);
    averageTime = (double) ((finish_time.tv_sec * 1000000 + finish_time.tv_usec) - (start_time.tv_sec * 1000000 + start_time.tv_usec)) / 1000 / 50;
    printf(
            "Calculation finished, average time consumed by MultimodalTwoQ:   %10.2f   milliseconds\n",
            averageTime);

    MSPclearPaths(finalPath);
    MSPfinalize();
    return EXIT_SUCCESS;
}
