// gen_VPw.cpp
// Set up problem to run hill-climbing algo
// Copyright A. Michael Sharifi, 2016

#include "headers.h"

gen_res gen_VPw(void *snodes_in, void *vf1_in, void *vf2_in, 
	            double coh, vector <double> x_w_lag, 
	            double b_min, double beg_equity ){

	snodes *snodes1 = (snodes *)snodes_in;
	vfn * vf1 = (vfn *)vf1_in;
	vfn * vf2 = (vfn *)vf2_in;

	int opt_flag = 1;
	int valid_flag = 1;

	vector<double> x_guess;
	vector<double> x0_default = { c_fs, 0.0, 0.0, 0.0, 0.0 };
	double v0_default = 1.0 / (1.0 - beta)*ufn(x0_default[0], hu_ten_def, pref);
	double v_guess;
	
	ufnEV2 ufnEV21;
	ufnEV21.enter_data(snodes1, vf2 );

	
	x_w_lag[1] = max(x_w_lag[1], b_min);
	x_w_lag[0] = coh - x_w_lag[1] - x_w_lag[2] - x_w_lag[3] - x_w_lag[4];

	if (x_w_lag[0] > 0.0) {
		x_guess = x_w_lag;
		v_guess = ufnEV21.eval(x_guess);
		
	} else {
		x_guess = x0_default;
		v_guess = -1.0e6;
	}

	if (  (coh - b_min) <= 0.0 ) {
		opt_flag = 0;
	}
	
	gen_res res1;
	res1.x_opt = x0_default;
	res1.v_opt = v0_default;
	res1.valid_flag = 0;

	int i_yi = (*snodes1).s2i_yi[(*vf2).i_s1];
	int t_hor = (*snodes1).t_hor;

	// current cash on hand; wealth and income only
	double cohQ = (*vf1).w_grid[(*vf2).w_i1] + (*snodes1).yi_gridt[t_hor][i_yi];
	
	if ( cohQ  < beg_equity ) {
		opt_flag = 0;
	}

	if ( opt_flag >= 1 ) {
	//if (  ( (coh - b_min) > 0.0 )  && ( cohQ > beg_equity ) ) {
		res1.x_opt = gen_x0(coh, b_min, vf1, vf2, &ufnEV21, x_guess);              // get x policy from loop and optimization
		res1.v_opt = ufnEV21.eval(res1.x_opt);
		res1.valid_flag = 1;
	}
	
	if ( (res1.x_opt[0] + res1.x_opt[1] + res1.x_opt[2] + res1.x_opt[3] + res1.x_opt[4]) >  (coh + 0.01 ) ) {
		valid_flag = 0;
	}

	if (res1.x_opt[1] < (b_min - 0.01) ) {
		valid_flag = 0;
	}

	if (res1.v_opt < v0_default) {
		valid_flag = 0;
	}

	if ((opt_flag <= 0) || (valid_flag <= 0)) {

		if (v_guess > v0_default) {
			res1.x_opt = x_guess;
			res1.v_opt = v_guess;
			res1.valid_flag = 1;

		}
		else {
			res1.x_opt = x0_default;
			res1.v_opt = v0_default;
			res1.valid_flag = 0;
		}
	}

    return res1;
}


//if (v_guess > res1.v_opt) {
//	cout << "gen_VPw: v_guess = " << v_guess << endl;
//	res1.v_opt = v_guess;
//	res1.x_opt = x_guess;
//}

//if ((*vf2).t_i2 == 2) &&  {
//}