#pragma once

// modifications made for AFEC:
//  - add missing prototypes
//  - add cast to (double) to avoid float conversion warnings
//  - prefix public functions with ooura_

// See c impl for official ooura comments

void ooura_cdft(int n, int isgn, double *a, int *ip, double *w);
void ooura_rdft(int n, int isgn, double *a, int *ip, double *w);
void ooura_ddct(int n, int isgn, double *a, int *ip, double *w);
void ooura_ddst(int n, int isgn, double *a, int *ip, double *w);
void ooura_dfct(int n, double *a, double *t, int *ip, double *w);
void ooura_dfst(int n, double *a, double *t, int *ip, double *w);

