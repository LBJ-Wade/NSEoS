#include <math.h>
#include <gsl/gsl_spline.h>

#include "mathconst.h"
#include "phyconst.h"
#include "tov.h"

#define G_CGS (G*1000.)
#define SPEEDOFL_CGS (SPEEDOFL*100.)

double calc_dm(double rho_, double r_, double dr_)
{
    return 4.*PI*r_*r_*rho_*dr_;
}

double calc_dp(double rho_, double p_, double r_, double dr_, double m_)
{
    return -G_CGS*m_*rho_/r_/r_*(1.+p_/rho_/SPEEDOFL_CGS/SPEEDOFL_CGS)
        *(1.+4.*PI*r_*r_*r_*p_/m_/SPEEDOFL_CGS/SPEEDOFL_CGS)
        /(1.-2.*G_CGS*m_/r_/SPEEDOFL_CGS/SPEEDOFL_CGS)*dr_;
}

void solve_tov_equation(int lines, double pt, double epst, char *outfile[])
{
    double msun = 1.989e33; // in g
    double p_factor_nu_to_cgs = 1.6022e33; // MeV/fm^3 to dyn/cm^2

    FILE *myeos;
    FILE *mytov;

    myeos = fopen(outfile[4], "r");
    mytov = fopen(outfile[5], "w+");

    double Rho[lines], P[lines];

    for(int i = 0; i < lines; i++)
    {
        fscanf(myeos, "%lf %lf", &Rho[i], &P[i]);
        P[i] *= p_factor_nu_to_cgs;
        if(i > 0 && P[i] < P[i-1])
            P[i] = P[i-1] + 1.e20;
    }

    gsl_interp_accel *acc
        = gsl_interp_accel_alloc ();
    gsl_spline *spline
        = gsl_spline_alloc (gsl_interp_linear, lines);

    double rhosat = 2.3e14; // saturation density in g/cm^3
    double dr = 100.; // radius step in cm
    double rhoc, pc;
    double rho, p;
    double m, r;
    double p_sav, m_sav, r_sav;
    double mcore, rcore;
    double rs, i_over_mr2, icrust_over_mr2;

    int N = 100; // number of points

    for(int j = 0; j < N; j++)
    {
        rhoc = rhosat + ((double)j / (double)N)*(Rho[lines-1] - rhosat);

        gsl_spline_init (spline, Rho, P, lines);
        pc = gsl_spline_eval (spline, rhoc, acc);

        rho = rhoc;
        p = pc;
        p_sav = p;

        m = 0.;
        r = 10.;
        m_sav = m;
        r_sav = r;

        gsl_spline_init (spline, P, Rho, lines);

        while(rho > 2.e5)
        {
            r += dr;
            m += calc_dm(rho, r, dr);
            p += calc_dp(rho, p, r, dr, m);

            if(p_sav > pt * p_factor_nu_to_cgs 
                    && p < pt * p_factor_nu_to_cgs)
            {
                mcore = (m_sav + m)/2.;
                rcore = (r_sav + r)/2.;
            }

            if(p > P[0])
            {
                rho = gsl_spline_eval(spline, p, acc);
            }
            else
                break;

            p_sav = p;
            r_sav = r;
            m_sav = m;
        }

        rs = 2.*G_CGS*m;
        // see: PRC82,025810(2010)
        i_over_mr2 = 0.21/(1.-rs/r/SPEEDOFL_CGS/SPEEDOFL_CGS);
        icrust_over_mr2 = 16.*PI/3.*pow(rcore,6.)*pt*p_factor_nu_to_cgs/rs
            *(1. - rs/r/SPEEDOFL_CGS/SPEEDOFL_CGS*i_over_mr2)
            *(1. + 48./5.*(rcore/rs*SPEEDOFL_CGS*SPEEDOFL_CGS - 1.)*(pt/epst))
            /m/r/r;

        r /= 100000.;
        m /= msun;
        rcore /= 100000.;
        mcore /= msun;

        fprintf(mytov, "%g %g %g %g %g %g %g %g\n", rhoc, pc, 
                r, m,
                rcore, mcore,
                i_over_mr2, icrust_over_mr2);
    }

    gsl_spline_free (spline);
    gsl_interp_accel_free (acc);

    fclose(myeos);
    fclose(mytov);
}