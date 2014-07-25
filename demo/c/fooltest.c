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
#include <time.h>
#include "../include/mmspa.h"

int main(void)
{
	/* 50 sources selected from the underground network randomly by another program for test */
	const char* testSources[] =
	{ "59529822", "59525660", "59524276", "59523770", "59522730", "59522320",
			"59521579", "59521043", "59520663", "59519758", "59519243",
			"59518864", "59517890", "59516900", "59509174", "59505072",
			"59503373", "568143481", "540165540", "813389942", "729211573",
			"655810285", "59533211", "59526235", "59525530", "59523807",
			"59523129", "59522463", "59521966", "59521294", "59520905",
			"59519810", "59519332", "59519142", "59518007", "59517610",
			"59509422", "59505327", "59504778", "568143762", "540175071",
			"817093120", "749091723", "674407202", "655769154", "59523916",
			"59523680", "59522594", "59522016", "59519381" };
	/* change the inputpath below to your local data path */
	char* inputpath = "/Users/user/Research/phd-work/data/UM/";
	printf("reading multimodal graphs from ASCII files...\n");
	clock_t start_time, finish_time;
	int i = 0;
	float duration = 0.0, totalTime = 0.0, averageTime = 0.0;
	/* read graphs */
	start_time = clock();
	if (Parse(inputpath) == EXIT_FAILURE)
	{
		printf("read graphs failed\n");
		return EXIT_FAILURE;
	}
	finish_time = clock();
	duration = (double) (finish_time - start_time) / 1000;
	printf(
			"reading and constructing graphs finished, time consumed:   %8.4f   seconds\n",
			duration);
	const char* mode_list[2] =
	{ "underground", "foot" };
	/* MMD */
	printf("calculating route by MultimodalDijkstra...\n");
	printf("Test times: 50\n");
	totalTime = 0.0;
	for (i = 0; i < 50; i++)
	{
		start_time = clock();
		MultimodalDijkstra(mode_list, 2, testSources[i]);
		finish_time = clock();
		duration = finish_time - start_time;
		printf("%10.5f,\n", duration);
		totalTime += duration;
	}
	averageTime = totalTime / 1000 / 50;
	printf(
			"calculation finished, average time consumed by MultimodalDijkstra:   %10.8f   seconds\n",
			averageTime);

	/* MMBF */
	//	printf("calculating route by MultimodalBellmanFord...\n");
	//	printf("Test times: 50\n");
	//	totalTime = 0.0;
	//	for (i = 0; i < 50; i++)
	//	{
	//		start_time = clock();
	//		MultimodalBellmanFord(mode_list, 2, testSources[i]);
	//		finish_time = clock();
	//		duration = finish_time - start_time;
	//		printf("%10.5f,\n", duration);
	//		totalTime += duration;
	//	}
	//	averageTime = totalTime / 1000 / 50;
	//	printf(
	//			"calculation finished, average time consumed by MultimodalBellmanFord:   %10.8f   seconds\n",
	//			averageTime);

	/* MMTQ */
	//	printf("calculating route by MultimodalTwoQ...\n");
	//	printf("Test times: 50\n");
	//	totalTime = 0.0;
	//	for (i = 0; i < 50; i++)
	//	{
	//		start_time = clock();
	//		MultimodalTwoQ(mode_list, 2, testSources[i]);
	//		finish_time = clock();
	//		duration = finish_time - start_time;
	//		printf("%10.5f,\n", duration);
	//		totalTime += duration;
	//	}
	//	averageTime = totalTime / 1000 / 50;
	//	printf(
	//			"calculation finished, average time consumed by MultimodalTwoQ:   %10.8f   seconds\n",
	//			averageTime);
	Dispose();
	return EXIT_SUCCESS;
}
