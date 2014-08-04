/*
 ============================================================================
 Name        : FoolTest.c
 Author      : LIU Lu
 Version     :
 Copyright   : Department of Cartography, TUM
 Description : Fool test for multimodal shortest path algorithms in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mmspa4pg/mmspa4pg.h"

int main(void)
{
	/* 50 sources selected from the underground network randomly by another program for test */
	const char* testSources[] = {
"100201006346",
"100201006351",
"100201006352",
"100201006356",
"100201006360",
"100201006361",
"100201006364",
"100201006367",
"100201006369",
"100201006374",
"100201006381",
"100201006383",
"100201006386",
"100201006388",
"100201006389",
"100201006390",
"100201006391",
"100201006393",
"100201006395",
"100201006399",
"100201006401",
"100201006402",
"100201006404",
"100201006405",
"100201006407",
"100201006411",
"100201006412",
"100201006413",
"100201006414",
"100201006415",
"100201006418",
"100201006419",
"100201006422",
"100201006425",
"100201006426",
"100201006427",
"100201006428",
"100201006430",
"100201006431",
"100201006432",
"100201006434",
"100201006435",
"100201006436",
"100201006437",
"100201006438",
"100201006439",
"100201006440",
"100201006444",
"100201006445",
"100201006446"
    };
	/* change the PostgreSQL connection string according to your own config */
	const char* conninfo = "dbname = 'mmrp_munich' user = 'liulu' password = 'workhard'";
	if (ConnectDB(conninfo) != EXIT_SUCCESS) {
    	printf("Connection to database failed.\n");
    	return EXIT_FAILURE;
    }
    printf("Connected to database. \n");
    printf("Creating multimodal routing plans... ");
    CreateRoutingPlan(1, 1);
    SetModeListItem(0, 1900);
    SetPublicTransitModeSetItem(0, 1003);
    SetCostFactor("speed");
    SetTargetConstraint(NULL);
    printf("done! \n");
	struct timeval start_time, finish_time;
	float duration = 0.0, averageTime = 0.0;
	/* read graphs */
    printf("Constructing multimodal graphs from PostgreSQL database... ");
	gettimeofday(&start_time, NULL);
    if (Parse() != EXIT_SUCCESS) { 
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
	for (int i = 0; i < 50; i++)
		MultimodalTwoQ(atoll(testSources[i]));
	gettimeofday(&finish_time, NULL);
	averageTime = (double) ((finish_time.tv_sec * 1000000 + finish_time.tv_usec) - (start_time.tv_sec * 1000000 + start_time.tv_usec)) / 1000 / 50;
	printf(
			"Calculation finished, average time consumed by MultimodalTwoQ:   %10.2f   milliseconds\n",
			averageTime);

	Dispose();
	DisconnectDB();
	return EXIT_SUCCESS;
}
