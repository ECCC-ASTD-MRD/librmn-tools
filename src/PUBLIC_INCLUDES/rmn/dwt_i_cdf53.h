
void fwd_1d_cdf53_asis(int *x, int n);
void inv_1d_cdf53_asis(int *x, int n);

void fwd_1d_cdf53_split(int *x, int *e, int *o, int n);
void inv_1d_cdf53_split(int *x, int *e, int *o, int n);

void fwd_1d_cdf53(int *x, int n);
void inv_1d_cdf53(int *x, int n);

void fwd_1d_cdf53_n(int *x, int n, int levels);
void inv_1d_cdf53_n(int *x, int n, int levels);

void fwd_2d_cdf53(int *x, int lni, int ni, int nj);
void inv_2d_cdf53(int *x, int lni, int ni, int nj);

void fwd_2d_cdf53_n(int *x, int lni, int ni, int nj, int levels);
void inv_2d_cdf53_n(int *x, int lni, int ni, int nj, int levels);
