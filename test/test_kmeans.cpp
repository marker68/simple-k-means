/*
 *  SIMPLE CLUSTERS: A simple library for clustering works.
 *  Copyright (C) 2014 Nguyen Anh Tuan <t_nguyen@hal.t.u-tokyo.ac.jp>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  test_kmeans.cpp
 *
 *  Created on: 2014/09/12
 *      Author: Nguyen Anh Tuan <t_nguyen@hal.t.u-tokyo.ac.jp>
 */


#include <iostream>
#include <vector>
#include <random>
#include <float.h>
#include <gtest/gtest.h>
#include <math.h>
#include <stdlib.h>
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "k-means.h"
#include "utilities.h"

using namespace std;
using namespace cv;
using namespace SimpleCluster;

/**
 * A converter
 */
void convert_array_to_mat(float ** data, Mat& result, size_t M, size_t N) {
	for(size_t i = 0; i < M; i++) {
		for(size_t j = 0; j < N; j++) {
			result.push_back(data[i][j]);
		}
	}

	result = result.reshape(1,static_cast<int>(M));
}

/**
 * Customized test case for testing
 */
class KmeansTest : public ::testing::Test {
protected:
	// Per-test-case set-up.
	// Called before the first test in this test case.
	// Can be omitted if not needed.
	static void SetUpTestCase() {
		N = 10000;
		d = 128;
		k = 256;
		int i, j;

		// For generating random numbers
		random_device rd;
		mt19937 gen(rd());
		uniform_real_distribution<float> real_dis(0.0, static_cast<float>(N));

		if(!init_array_2<float>(data,N,d)) {
			cerr << "Cannot allocate memory for test data!" << endl;
			exit(1);
		}
		for(i = 0; i < N; i++) {
			for(j = 0; j < d; j++) {
				data[i][j] = real_dis(gen);
			}
		}

		if(!init_array_2<float>(seeds,k,d)) {
			cerr << "Cannot allocate memory for seeds data!" << endl;
			exit(1);
		}
		if(!init_array_2<float>(centers,k,d)) {
			cerr << "Cannot allocate memory for centers data!" << endl;
			exit(1);
		}
		label = (size_t *)calloc(N,sizeof(size_t));
		if(label == NULL) {
			cerr << "Cannot allocate memory for label data!" << endl;
			exit(1);
		}
		//		print_vector(data,d,N);
	}

	// Per-test-case tear-down.
	// Called after the last test in this test case.
	// Can be omitted if not needed.
	static void TearDownTestCase() {
		if(!dealloc_array_2<float>(data,N)) {
			cerr << "data: Deallocating failed!" << endl;
			exit(1);
		}
		if(!dealloc_array_2<float>(seeds,k)) {
			cerr << "seeds: Deallocating failed!" << endl;
			exit(1);
		}
		if(!dealloc_array_2<float>(centers,k)) {
			cerr << "centers: Deallocating failed!" << endl;
			exit(1);
		}
		::delete label;
	}

	// You can define per-test set-up and tear-down logic as usual.
	virtual void SetUp() { }
	virtual void TearDown() {}

public:
	// Some expensive resource shared by all tests.
	static float ** data;
	static float ** seeds;
	static float ** centers;
	static size_t * label;
	static size_t N, d, k;
};

float ** KmeansTest::data;
float ** KmeansTest::seeds;
float ** KmeansTest::centers;
size_t * KmeansTest::label;
size_t KmeansTest::N;
size_t KmeansTest::d;
size_t KmeansTest::k;

TEST_F(KmeansTest, DISABLED_test1) {
	random_seeds(data,seeds,d,N,k,true);
}

TEST_F(KmeansTest, DISABLED_test2) {
	kmeans_pp_seeds(data,seeds,d,N,k,true);
}

TEST_F(KmeansTest, DISABLED_test3) {
	KmeansCriteria criteria = {2.0,1.0,100};
	simple_k_means(
			data,centers,label,seeds,
			KmeansType::KMEANS_PLUS_SEEDS,
			KmeansAssignType::ANN_KD_TREE,
			criteria,
			N,k,d,
			false);
	cout << "ANN: Distortion is " << distortion(data,centers,label,d,N,k,false) << endl;
}

TEST_F(KmeansTest, DISABLED_test4) {
	KmeansCriteria criteria = {2.0,1.0,100};
	simple_k_means(
			data,centers,label,seeds,
			KmeansType::KMEANS_PLUS_SEEDS,
			KmeansAssignType::NN_KD_TREE,
			criteria,
			N,k,d,
			false);
	cout << "NN: Distortion is " << distortion(data,centers,label,d,N,k,false) << endl;
}

TEST_F(KmeansTest, test5) {
	KmeansCriteria criteria = {2.0,1.0,100};
	simple_k_means(
			data,centers,label,seeds,
			KmeansType::KMEANS_PLUS_SEEDS,
			KmeansAssignType::LINEAR,
			criteria,
			N,k,d,
			false);
	cout << "LINEAR: Distortion is " << distortion(data,centers,label,d,N,k,false) << endl;
}

TEST_F(KmeansTest, test6) {
	Mat _data;
	convert_array_to_mat(data,_data,N,d);
	_data.convertTo(_data,CV_32F);
	Mat _labels, _centers(k, 1, _data.type());
	TermCriteria opencv_criteria {CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 100, 1.0};
	kmeans(_data, k, _labels, opencv_criteria, 3, KMEANS_PP_CENTERS, _centers);
	// Find the distortion
	float distortion = 0.0;
	for(size_t i = 0; i < _labels.rows; i++) {
		size_t id = _labels.at<size_t>(i,0);
		Mat tmp = _data.row(i);
		Mat c = _centers.row(id);
		float d_t = norm(tmp,c,NORM_L2);
		distortion += d_t * d_t;
	}
	cout << "Distortion is " << sqrt(distortion) << endl;
}

TEST_F(KmeansTest, test7) {
	KmeansCriteria criteria = {2.0,1.0,100};
	for(size_t i = 0; i < 10; i++) {
		simple_k_means(
				data,centers,label,seeds,
				KmeansType::KMEANS_PLUS_SEEDS,
				KmeansAssignType::LINEAR,
				criteria,
				N,k,d,
				false);
		cout << "LINEAR: Distortion is " << distortion(data,centers,label,d,N,k,false) << endl;
	}
}
