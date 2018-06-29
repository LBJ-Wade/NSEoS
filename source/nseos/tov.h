#ifndef H_TOV
#define H_TOV

#include <stdio.h>

double calc_dm(double rho_, double r_, double dr_);
double calc_dp(double rho_, double p_, double r_, double dr_, double m_);
void solve_tov_equation(const int lines, double pt, char *outfile[]);

#endif // H_TOV
