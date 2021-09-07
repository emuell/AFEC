#pragma once

// modifications made for AFEC:
//  - add missing prototypes
//  - prefix public functions with ooura_

// See c impl for official ooura comments and credits

void ooura_cdft(int n, int isgn, double *a, int *ip, double *w);
void ooura_rdft(int n, int isgn, double *a, int *ip, double *w);
void ooura_ddct(int n, int isgn, double *a, int *ip, double *w);
void ooura_ddst(int n, int isgn, double *a, int *ip, double *w);
void ooura_dfct(int n, double *a, double *t, int *ip, double *w);
void ooura_dfst(int n, double *a, double *t, int *ip, double *w);

