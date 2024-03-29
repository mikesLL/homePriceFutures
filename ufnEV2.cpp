/*
ufnEV2.cpp
Copyright A. Michael Sharifi, 2016
*/

#include "headers.h"

// enter data to speed up value function evaluation
void ufnEV2::enter_data(void *snodes_in, void *vf2_in) {

	int i_s2, i_ph2;
	int i_ph1;
	snodes1 = (snodes *)snodes_in;     // state dimensions
	vf2 = (vfn *)vf2_in;               // next period value function

	t_hor = (*snodes1).t_hor; // current horizon / value function under computation
	i_s1 = (*vf2).i_s1;       // current state
	t_i2 = (*vf2).t_i2;       // next period tenure (predetermined)
	hu = (*snodes1).hu_ten[t_i2];        // housing utility (fn of tenure)
	ph_2e = 0.0;    
         
	i_ph1 = (*snodes1).s2i_ph[i_s1];             // retrieve current home price index
	ph1 = (*snodes1).p_gridt[t_hor][i_ph1];      // current home price

	vw3_grid_ti2 = (*vf2).vw3_grid[t_i2];
	vw3_d_grid_ti2 = (*vf2).vw3_d_grid[t_i2];
	vw3_dd_grid_ti2 = (*vf2).vw3_dd_grid[t_i2];

	// compute home price expectation
	for (i_s2 = 0; i_s2 < n_s; i_s2++) {                     
		i_ph2 = (*snodes1).s2i_ph[i_s2];        // pass in single state; return price index
		ph_2e = ph_2e + (*snodes1).gammat[t_hor][i_s1][i_s2] * (*snodes1).p_gridt[t_hor+1][i_ph2];
	}

	// compute home price future payout in each state
	//if ( t_hor <= (T_max - 2) ) {
	for (i_s2 = 0; i_s2 < n_s; i_s2++) {
		i_ph2 = (*snodes1).s2i_ph[i_s2];
		//csf_net2[i_s2] = (*snodes1).p_gridt[t_hor + 1][i_ph2] - ph_2e;
		csf_net2[i_s2] = ( (*snodes1).p_gridt[t_hor + 1][i_ph2] - ph_2e ) / ph_2e;
	}

	i_s2p = 0;
	for (i_s2 = 0; i_s2 < n_s; i_s2++) {
		if ( (*snodes1).gammat[t_hor][i_s1][i_s2] > 0.0) {
			i_s2p_vec[i_s2p] = i_s2;
			i_s2p++;
		}
	}
	N_s2p = i_s2p; // store number of positive probability states

}


// given vector of control variables x, evaluate value function
double ufnEV2::eval( vector<double> x ){

	int i_s2, i_ph2, i_x2;                         // state index, price index, equity return index
	double rb_eff = rb;                            // effective return on bonds	(default = rb)       
	double Evw_2 = 0.0;                            // Value Function Expectation
	double w2, w2_move, vw2;

	double uc = ufn(x[0], hu, (*vf2).pref);  // composite utility
	
	// calc effective effective interest rate
	if ((*vf2).t_i2 == 0) {           
		if (x[1] < 0.0) {
			rb_eff = rb + credit_prem;     // unsecured credit APR
		}
	}
	else {
		if (x[1] < 0.0) {                                         
			rb_eff = rb + mort_spread;                    // if agent holds mortgage debt, add mortgage spread
		}
		                                                       
		if (x[1] < -ph1*(1.0 - pmi_dpmt)*(*snodes1).ten_w[t_i2]) {
			rb_eff = rb + mort_spread + pmi_prem;         // if agent has a low down payment, add pmi
		}
	}
	
	// cycle accross possible future states to compute value function expectation
	for (i_s2p = 0; i_s2p < N_s2p; i_s2p++) {
		i_s2 = i_s2p_vec[i_s2p];
		i_ph2 = (*snodes1).s2i_ph[i_s2];

		// cycle across equity returns
		for (i_x2 = 0; i_x2 < retxn; i_x2++) {
			w2 = rb_eff*x[1] + exp(retxv[i_x2])*x[2] +
				csfLev * csf_net2[i_s2] * (x[3] - x[4]) +
				x[3] + x[4] + (*snodes1).ten_w[t_i2] * (*snodes1).p_gridt[t_hor + 1][i_ph2];
                            
			res1 = eval_v(i_s2, w2);    // evaluate value function in state
			vw2 = (1.0 - p_move) * res1.v_out;
			Evw_2 = Evw_2 + retxp[i_x2] * (*snodes1).gammat[t_hor][i_s1][i_s2] * vw2;  // compute expectation
		}

	}
	return uc + beta*Evw_2;
}


inline eval_res ufnEV2::eval_v( int i_s_in, double w_in) {
	double num1, den1, w_diff1, w_diff2;

	if ( (w_in >= w_min) && (w_in < w_max) ) {
		w_i_d = (double)(w_n - 1.0)*(w_in - w_min) / (w_max - w_min);    // map w_in to w_i
		w_i_low = (int)floor(w_i_d);
		w_low = (*vf2).w_grid[w_i_low];
	
		w_diff2 = ( (*vf2).w_grid[1] - (*vf2).w_grid[0] );

		alpha1 = (w_in - w_low) / w_diff2;
		
		v_tlower = vw3_grid_ti2[i_s_in][w_i_low];
		v_tupper = vw3_grid_ti2[i_s_in][w_i_low + 1];

		res2.v_out = v_tlower + alpha1*(v_tupper - v_tlower);
		
		res2.w_i_floor = w_i_low;
		res2.v_i_floor = vw3_grid_ti2[i_s_in][w_i_low];  // TODO: double check; this may be an issue when considering tenure changes
	}
	else {
		if (w_in >= w_max) {
			w_i_low = w_n - 1;

			v_tlower = vw3_grid_ti2[i_s_in][w_i_low];

			w_diff1 = (w_in - w_max);
			w_diff2 = -vw3_d_grid_ti2[i_s_in][w_i_low] / vw3_dd_grid_ti2[i_s_in][w_i_low] + w_max;

			if (w_diff1 <= w_diff2) {
				res2.v_out = vw3_grid_ti2[i_s_in][w_i_low] + 
					w_diff1*vw3_d_grid_ti2[i_s_in][w_i_low] +
					0.5*pow( w_diff1, 2.0)*vw3_dd_grid_ti2[i_s_in][w_i_low];
			}
			else {
				res2.v_out = vw3_grid_ti2[i_s_in][w_i_low] +
					w_diff2*vw3_d_grid_ti2[i_s_in][w_i_low] +
					0.5*pow(w_diff2, 2.0)*vw3_dd_grid_ti2[i_s_in][w_i_low];
			}
			
			res2.v_out = max(res2.v_out , v_tlower);

			if (res2.v_out != res2.v_out) {
				res2.v_out = v_tlower;
			}

			res2.v_out = v_tlower; // impose satiation
			res2.w_i_floor = w_n - 1;
			res2.v_i_floor = res2.v_out;
		} else {
			res2.v_out = vw3_grid_ti2[i_s_in][0] - 1e6*pow(w_in - w_min, 2);
			res2.w_i_floor = 0;
			res2.v_i_floor = res2.v_out;
		}
	}

	return res2;
}


