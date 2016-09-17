// gen_x0.cpp
// Given coh (cash on hand), b_min (lower bound for bonds / mortgage borrowing), ph_i1 (current price), t_i2 (tenure in next period),
// get_x0_loop loops through choice space in C,B,X,CSFp,CSFn to find an initial guess for the policy that maximizes the value fn
// then get_x0_loop runs a hill-climbing algorithm to find the optimal choice values and returns these as a vector
//
// Copyright A. Michael Sharifi


#include "headers.h"

vector<double> gen_x0(double coh_in, double b_min, void *vf1_in, void *vf2_in, void *ufnEV21_in, vector<double> x0_in) {
	
	vfn *vf1 = (vfn *)vf1_in;             // pointer to vf1 or V1 value fn
	vfn *vf2 = (vfn *)vf2_in;             // pointer to vf2 or V2 value fn

	ufnEV2 * ufnEV21 = (ufnEV2*)ufnEV21_in;

	//int t_i2 = (*ufnEV21).t_i2;
	double coh = coh_in;
	double csf_min = 0.0;

	double h_tol = 0.0005; //.0005; //  50 dollar accuracy
	double csf_tol = 0.005;

	int opt_flag = 1, it_max = 100000;
	int N_control2 = N_control - 3;  // For part without csf
	int N_control3 = N_control - 2; // -1;  // for part with csf

	int N_controlh;
	double h_step0 = 0.2;
	double h_step_mult = 0.25;
	double h_step = h_step0;

	vector<double> x0 = x0_in;    
	vector<double> x0_nocsf = x0_in;
	vector<double> x0_g2 = x0_in;
	vector<double> x0_h(5, 0.0);
	vector<double> x1(5, 0.0);
	
	double lb[] = { 0.01, b_min, 0.0, 0.0, 0.0 };
	
	int it = 0;
	double v0, v0_h, v1, vi_max, vi_min;
	double v0_nocsf, v0_g2;

	int i_max, i_min, i_min_flag, i, j, i_inner_loop;

	int it_max_nocsf = it_max;
	double h_tol_nocsf = h_tol;

	int it_max_g2 = it_max;
	double h_tol_g2 = h_tol;
	

	// starting guess for v0, adjusted to current coh, b_min
	csf_min = min(x0[3], x0[4]);
	x0[3] = x0[3] - csf_min;
	x0[4] = x0[4] - csf_min;
	x0[1] = max(x0[1], b_min);                       
	x0[0] = coh - x0[1] - x0[2] - x0[3] - x0[4];
	v0 = (*ufnEV21).eval(x0);       

	// x0_g2: simple, robust guess
	x0_g2[3] = 0.0;
	x0_g2[4] = 0.0;
	x0_g2[2] = 0.0; 
	x0_g2[1] = b_min; //  max(x0_g2[1], b_min);
	x0_g2[0] = coh - x0_g2[1];
	v0_g2 = (*ufnEV21).eval(x0_g2);

	vector<double> x0_g3 = x0_g2;
	double v0_g3 = v0_g2;
	double vi = -1.0e6;

	N_controlh = N_control2; // for larger step sizes, only allow access to C,B,X

	int k1 = 0, k2 = 0;
	int nds = 10;
	int nds2 = 2;

	if ( (*vf2).w_i1 % 10 == 0 ) {

		// first, compute a rough guess without access to csf
		for (k1 = 0; k1 <= nds; k1++) {
			for (k2 = 0; k2 <= (nds - k1); k2++) {
				x1[0] = c_fs + double(k1) / double(nds) * (coh - b_min);
				x1[1] = b_min + double(k2) / double(nds) * (coh - b_min);
				x1[2] = coh - x1[0] - x1[1];

				if (x1[2] >= 0.0) {
					v1 = (*ufnEV21).eval(x1);

					if (v1 > v0_g3) {
						x0_g3 = x1;
						v0_g3 = v1;
					}
				}
			}
		}
	}

	if (v0_g3 > v0) {
		v0 = v0_g3;
		x0 = x0_g3;
	}

	double h_step1 = 0.0;
	
	while (opt_flag) {
		
		i_max = 0;
		i_min = 0;
		i_min_flag = 0;

		v1 = -1.0e6;               
		vi_max = -1.0e6;  
		vi_min = 1.0e6;

		for (i = 0; i < N_control3; i++) {
			if ( (i == 3 ) && ( (*ufnEV21).t_i2 >= 1 ) ) {
				i++;
			}

			x0_h = x0;
			x0_h[i] = x0[i] + h_step;

			v0_h = (*ufnEV21).eval(x0_h);

			if (v0_h > vi_max) {
				i_max = i;
				vi_max = v0_h;
			}

			if ((v0_h < vi_min) && ( (x0[i] - h_step) >= lb[i])) {
				i_min = i;
				i_min_flag = 1;
				vi_min = v0_h;	
			}
		}
	
		h_step1 = h_step;
		if ( i_min_flag >= 1 ) {
			
			x1 = x0;
			v1 = -1.0e6;

			if (h_step >= 0.2) {
				nds2 = 10;
			} else {
				nds2 = 4;
			}

			for (k1 = 0; k1 <= nds2; k1++) {
				x0_h = x0;
				x0_h[i_max] = x0_h[i_max] + double(k1) / double(nds2) * h_step;
				x0_h[i_min] = x0_h[i_min] - double(k1) / double(nds2) * h_step;
				
				v0_h = (*ufnEV21).eval(x0_h);
					
				if (v0_h > v1) {
					x1 = x0_h;
					v1 = v0_h;
					h_step1 = double(k1) / double(nds2) * h_step;
				}
			}
		}

		if (v1 > v0) {
			v0 = v1;
			x0 = x1;
			h_step = h_step1;
		}
		else {
			h_step = h_step * h_step_mult;
		}

		it++;

		if ( (it > it_max) || (h_step < h_tol) ) {
			opt_flag = 0;
		}
	}

	if (v0_g3 > v0) {
		v0 = v0_g3;
		x0 = x0_g3;
	}

	return x0;
}
