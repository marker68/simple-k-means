/*
 * k-means.h
 *
 *  Created on: 2014/09/04
 *      Author: Nguyen Anh Tuan <t_nguyen@hal.t.u-tokyo.ac.jp>
 */

#ifndef K_MEANS_H_
#define K_MEANS_H_

#include <iostream>
#include <vector>

using namespace std;

/**
 * Cluster methods' space
 */
namespace cluster {
/**
 * Types of the k-means seeding
 */
enum class KmeansType {
	RANDOM_SEEDS,
	KMEANS_PLUS_SEEDS, // k-means++
	USER_SEEDS
};

double simple-k-means(KmeansType, size_t, size_t, int, vector<double>, vector<double>, vector<vector<double>>, vector<double>);
}

#endif /* K_MEANS_H_ */
