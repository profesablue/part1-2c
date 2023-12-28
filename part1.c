#include <stdio.h>
#include <math.h>
#include "params.h"
#include <omp.h> // Include OpenMP library

void init(double u[N][N], double v[N][N]) {
    double uhi, ulo, vhi, vlo;
    uhi = 0.5;
    ulo = -0.5;
    vhi = 0.1;
    vlo = -0.1;

#pragma omp parallel for collapse(2)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            u[i][j] = ulo + (uhi - ulo) * 0.5 * (1.0 + tanh((i - N / 2) / 16.0));
            v[i][j] = vlo + (vhi - vlo) * 0.5 * (1.0 + tanh((j - N / 2) / 16.0));
        }
    }
}

void dxdt(double du[N][N], double dv[N][N], double u[N][N], double v[N][N]) {
    double lapu, lapv;
    int up, down, left, right;

#pragma omp parallel for private(lapu, lapv, up, down, left, right)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i == 0) {
                down = i;
            } else {
                down = i - 1;
            }
            if (i == N - 1) {
                up = i;
            } else {
                up = i + 1;
            }
            if (j == 0) {
                left = j;
            } else {
                left = j - 1;
            }
            if (j == N - 1) {
                right = j;
            } else {
                right = j + 1;
            }
            lapu = u[up][j] + u[down][j] + u[i][left] + u[i][right] + -4.0 * u[i][j];
            lapv = v[up][j] + v[down][j] + v[i][left] + v[i][right] + -4.0 * v[i][j];
            du[i][j] = DD * lapu + u[i][j] * (1.0 - u[i][j]) * (u[i][j] - b) - v[i][j];
            dv[i][j] = d * DD * lapv + c * (a * u[i][j] - v[i][j]);
        }
    }
}

void step(double du[N][N], double dv[N][N], double u[N][N], double v[N][N]) {
#pragma omp parallel for collapse(2)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            u[i][j] += dt * du[i][j];
            v[i][j] += dt * dv[i][j];
        }
    }
}

double norm(double x[N][N]) {
    double nrmx = 0.0;
#pragma omp parallel for reduction(+:nrmx)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            nrmx += x[i][j] * x[i][j];
        }
    }
    return nrmx;
}

int main(int argc, char **argv) {
    double t = 0.0, nrmu, nrmv;
    double u[N][N], v[N][N], du[N][N], dv[N][N];

    FILE *fptr = fopen("nrms.txt", "w");
    fprintf(fptr, "#t\t\tnrmu\t\tnrmv\n");

    // Initialize OpenMP parallel region
    omp_set_num_threads(4); // Set the number of threads as needed

    // initialize the state
    init(u, v);

    // time-loop
    for (int k = 0; k < M; k++) {
        // track the time
        t = dt * k;
        // evaluate the PDE
        dxdt(du, dv, u, v);
        // update the state variables u,v
        step(du, dv, u, v);
        if (k % m == 0) {
            // calculate the norms
            nrmu = norm(u);
            nrmv = norm(v);
            printf("t = %2.1f\tu-norm = %2.5f\tv-norm = %2.5f\n", t, nrmu, nrmv);
            fprintf(fptr, "%f\t%f\t%f\n", t, nrmu, nrmv);
        }
    }

    fclose(fptr);
    return 0;
}