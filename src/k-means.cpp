/*
 * k-means.cpp
 *
 *  Created on: 2014/09/04
 *      Author: Nguyen Anh Tuan <t_nguyen@hal.t.u-tokyo.ac.jp>
 */

#include <iostream>
#include <algorithm>
#include <vector>
#include <random>
#include <cstring>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "k-means.h"
#include "utilities.h"
#include "kd-tree.h"

using namespace std;

namespace SimpleCluster {


/*
 * Global variables
 */
size_t * range = NULL;

void random_seeds(size_t d, size_t N, size_t k,
		double ** data, double **& seeds, bool verbose) {
	size_t i, j;
	int tmp[k];

	// For generating random numbers
	random_device rd;
	mt19937 gen(rd());

	for(i = 0; i < k; i++)
		tmp[i] = static_cast<int>(i);
	for(i = k; i < N; i++) {
		uniform_int_distribution<> int_dis(i,N-1);
		j = int_dis(gen);
		if(j < k)
			tmp[j] = static_cast<int>(i);
	}

	for(i = 0; i < k; i++) {
		if(!copy_array<double>(data[tmp[i]],seeds[i],d)) {
			if(verbose)
				cerr << "Cannot copy array! The program exit now!" << endl;
			exit(1);
		}
	}
}

void kmeans_pp_seeds(size_t d, size_t N, size_t k,
		double ** data, double **& seeds, bool verbose) {
	// For generating random numbers
	random_device rd;
	mt19937 gen(rd());

	uniform_int_distribution<size_t> int_dis(0, N - 1);
	size_t tmp = int_dis(gen);

	double * d_tmp;
	init_array<double>(d_tmp,d);
	copy_array<double>(data[tmp],seeds[0],d);
	double * distances;
	init_array<double>(distances,N);
	double * sum_distances;
	init_array<double>(sum_distances,N);
	size_t i;
	for(i = 0; i < N; i++) {
		distances[i] = SimpleCluster::distance_square(data[i], d_tmp, d);
		sum_distances[i] = 0.0;
	}

	double sum, tmp2, sum1, sum2, pivot;
	size_t count = 1;
	while(count < k) {
		sum = 0.0;
		for(i = 0; i < N; i++) {
			sum += distances[i];
			sum_distances[i] = sum;
		}
		uniform_real_distribution<double> real_dis(0, sum);
		pivot = real_dis(gen);
		for(i = 0; i < N - 1; i++) {
			sum1 = sum_distances[i];
			sum2 = sum_distances[i + 1];
			if(sum1 < pivot && pivot <= sum2)
				break;
		}
		copy_array<double>(data[i+1],d_tmp,d);
		copy_array<double>(d_tmp,seeds[count++],d);
		// Update the distances
		if(count < k) {
			for(i = 0; i < N; i++) {
				tmp2 = SimpleCluster::distance_square(d_tmp,data[i],d);
				if(distances[i] > tmp2) distances[i] = tmp2;
			}
		}
	}
	::delete[] distances;
	::delete[] sum_distances;
	::delete[] d_tmp;
}

void assign_to_closest_centroid(size_t d, size_t N, size_t k,
		double ** data, double ** centroids, int **& clusters, bool verbose) {
	size_t i, j, tmp;
	i_vector i_tmp;
	init_array<int *>(clusters,k);

	double min = 0.0, temp = 0.0;
	if(range == NULL) {
		init_array<size_t>(range,k);
	}

	for(i = 0; i < k; i++) {
		range[i] = 0;
	}

	for(i = 0; i < N; i++) {
		// Find the minimum distances between d_tmp and a centroid
		min = SimpleCluster::distance(data[i],centroids[0],d);
		tmp = 0;
		for(j = 1; j < k; j++) {
			temp = SimpleCluster::distance(data[i],centroids[j],d);
			if(min < temp) {
				min = temp;
				tmp = j;
			}
		}
		// Assign the data[i] into cluster tmp
		clusters[tmp][range[tmp]++] = static_cast<int>(i);
	}
}

void assign_to_closest_centroid_2(size_t d, size_t N, size_t k,
		double ** data, double ** centroids, int **& clusters, bool verbose) {
	size_t i, tmp;
	KDNode<double> * root = NULL;
	make_random_tree(root,centroids,k,d,0,0,verbose);
	if(root == NULL) return;
	init_array<int *>(clusters,k);

	double min = DBL_MAX;

	if(range == NULL) {
		init_array<size_t>(range,k);
	}

	for(i = 0; i < k; i++) {
		range[i] = 0;
	}

	for(i = 0; i < N; i++) {
		if(verbose)
			cout << "Reached in-loop " << i << "-th" << endl;
		KDNode<double> * nn = NULL;
		// Find the minimum distances between d_tmp and a centroid
		nn_search(root,data[i],nn,min,d,0,verbose);
		tmp = nn->id;
		nn = NULL;
		min = DBL_MAX;
		// Assign the data[i] into cluster tmp
		clusters[tmp][range[tmp]++] = static_cast<int>(i);
	}

	::delete root;
}

/**
 * The k-means method: a description of the method can be found at
 * http://home.deib.polimi.it/matteucc/Clustering/tutorial_html/kmeans.html
 */
void simple_k_means(KmeansType type, size_t N, size_t k, KmeansCriteria criteria,size_t d,
		double ** data, double ** centroids,
		int ** clusters, double ** seeds, bool verbose) {
	// Pre-check conditions
	if (N < k) {
		if(verbose)
			cerr << "There will be some empty clusters!" << endl;
		exit(1);
	}

	if(seeds == NULL) {
		init_array_2<double>(seeds,k,d);
	}

	// Seeding
	// In case of defective seeds from users, we should overwrite it by kmeans++ seeds
	if (type == KmeansType::RANDOM_SEEDS) {
		random_seeds(d,N,k,data,seeds,verbose);
	} else if(type == KmeansType::KMEANS_PLUS_SEEDS) {
		kmeans_pp_seeds(d,N,k,data,seeds,verbose);
	}

	if(verbose)
		cout << "Finished seeding" << endl;

	// Criteria's setup
	size_t iters = criteria.iterations, i = 0;
	double error = criteria.accuracy, e = error, e_prev = 0.0;

	double ** c_tmp;
	init_array_2<double>(c_tmp,k,d);

	double tmp = 0.0;

	// Initialize the centroids
	copy_array_2<double>(seeds,centroids,k,d);

	while (1) {
		if(verbose)
			cout << "Reached loop " << i << "-th" << endl;
		// Assign the data points to clusters
		assign_to_closest_centroid_2(d,N,k,data,centroids,clusters,verbose);
		// Recalculate the centroids
		for(size_t j = 0; j < k; j++) {
			double * d_tmp = SimpleCluster::mean_vector(data,clusters[j],
					d,range[j],centroids[j]);
			c_tmp[j] = d_tmp;
		}
		// Calculate the distortion
		e_prev = e;
		e = 0.0;
		for(size_t j = 0; j < k; j++) {
			tmp = SimpleCluster::distance_square(centroids[j],c_tmp[j],d);
			e += tmp;
		}
		e = sqrt(e);

		copy_array_2<double>(c_tmp,centroids,k,d);
		i++;
		if(i >= iters ||
				//				(e - e_prev < error && e - e_prev > -error)) break;
				(e < error && e > -error)) break;
	}

	if(verbose)
		cout << "Finished clustering with error is " <<
		e << " after " << i << " iterations." << endl;

	dealloc_array_2<double>(c_tmp,k);
	dealloc_array_2<double>(seeds,k);
}

/**
 * Calculate the distortion of a set of clusters.
 */
double distortion(size_t d, size_t N, size_t k,
		double ** data, double ** centroids, int ** clusters) {
	double e = 0.0;
	size_t i, j = 0;
	double * d_tmp;
	int * i_tmp;

	for(i = 0; i < k; i++) {
		i_tmp = clusters[i];
		d_tmp = centroids[i];
		while(j < range[i]) {
			e += SimpleCluster::distance_square(d_tmp, data[i_tmp[j]], d);
			++j;
		}
	}

	::delete[] d_tmp;
	::delete[] i_tmp;

	return sqrt(e);
}
}

