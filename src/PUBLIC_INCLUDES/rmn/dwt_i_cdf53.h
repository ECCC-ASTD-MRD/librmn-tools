
void fwd_1d_cdf53(int *tmp, int n);
void inv_1d_cdf53(int *tmp, int n);

void fwd_1d_cdf53_split_inplace(int *x, int n);
void fwd_1d_cdf53_split_even(int *x, int *e, int *o, int n);
void fwd_1d_cdf53_split_odd(int *x, int *e, int *o, int n);
void fwd_1d_cdf53_split(int *x, int *e, int *o, int n);

void inv_1d_cdf53_split_inplace(int *tmp, int n);
void inv_1d_cdf53_split_even(int *x, int *e, int *o, int n);
void inv_1d_cdf53_split_odd(int *x, int *e, int *o, int n);
void inv_1d_cdf53_split(int *x, int *e, int *o, int n);
