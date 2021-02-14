#include <stdlib.h>
#include "types.h"

IVValue new_iv() {
    return (IVValue) malloc(sizeof(union ivvalue_t));
}

IVValue new_iv_str(char *str) {
    IVValue iv = new_iv();
    iv->str = str;
    return iv;
}

IVValue new_iv_v(void *v) {
    IVValue iv = new_iv();
    iv->v = v;
    return iv;
}

IVValue new_iv_c(char c) {
    IVValue iv = new_iv();
    iv->c = c;
    return iv;
}

IVValue new_iv_ca(char ca[8]) {
    IVValue iv = new_iv();
    int i;
    for (i = 0; i < 8; i++)
        iv->ca[i] = ca[i];
    return iv;
}

IVValue new_iv_uc(unsigned char uc) {
    IVValue iv = new_iv();
    iv->uc = uc;
    return iv;
}

IVValue new_iv_uca(unsigned char uca[8]) {
    IVValue iv = new_iv();
    int i;
    for (i = 0; i < 8; i++)
        iv->uca[i] = uca[i];
    return iv;
}

IVValue new_iv_s(short s) {
    IVValue iv = new_iv();
    iv->s = s;
    return iv;
}

IVValue new_iv_sa(short sa[4]) {
    IVValue iv = new_iv();
    int i;
    for (i = 0; i < 4; i++)
        iv->sa[i] = sa[i];
    return iv;
}

IVValue new_iv_us(unsigned short us) {
    IVValue iv = new_iv();
    iv->us = us;
    return iv;
}

IVValue new_iv_usa(unsigned short usa[4]) {
    IVValue iv = new_iv();
    int i;
    for (i = 0; i < 4; i++)
        iv->usa[i] = usa[i];
    return iv;
}

IVValue new_iv_i(int i) {
    IVValue iv = new_iv();
    iv->i = i;
    return iv;
}

IVValue new_iv_ia(int ia[2]) {
    IVValue iv = new_iv();
    iv->ia[0] = ia[0];
    iv->ia[1] = ia[1];
    return iv;
}

IVValue new_iv_ui(unsigned int ui) {
    IVValue iv = new_iv();
    iv->ui = ui;
    return iv;
}

IVValue new_iv_uia(unsigned int uia[2]) {
    IVValue iv = new_iv();
    iv->uia[0] = uia[0];
    iv->uia[1] = uia[1];
    return iv;
}

IVValue new_iv_l(long long l) {
    IVValue iv = new_iv();
    iv->l = l;
    return iv;
}

IVValue new_iv_la(long long *la) {
    IVValue iv = new_iv();
    iv->la = la;
    return iv;
}

IVValue new_iv_ul(unsigned long long ul) {
    IVValue iv = new_iv();
    iv->ul = ul;
    return iv;
}

IVValue new_iv_ula(unsigned long long *ula) {
    IVValue iv = new_iv();
    iv->ula = ula;
    return iv;
}

IVValue new_iv_f(float f) {
    IVValue iv = new_iv();
    iv->f = f;
    return iv;
}

IVValue new_iv_fa(float fa[2]) {
    IVValue iv = new_iv();
    iv->fa[0] = fa[0];
    iv->fa[1] = fa[1];
    return iv;
}

IVValue new_iv_d(double d) {
    IVValue iv = new_iv();
    iv->d = d;
    return iv;
}

IVValue new_iv_da(double *da) {
    IVValue iv = new_iv();
    iv->da = da;
    return iv;
}

char *iv_str(IVValue iv) {
    return iv->str;
}

void *iv_v(IVValue iv) { return iv->v; }
char  iv_c(IVValue iv) { return iv->c; }
char *iv_ca(IVValue iv) { return iv->ca; }
unsigned char  iv_uc(IVValue iv) { return iv->uc; }
unsigned char *iv_uca(IVValue iv) { return iv->uca; }
short  iv_s(IVValue iv) { return iv->s; }
short *iv_sa(IVValue iv) { return iv->sa; }
unsigned short  iv_us(IVValue iv) { return iv->us; }
unsigned short *iv_usa(IVValue iv) { return iv->usa; }
int  iv_i(IVValue iv) { return iv->i; }
int *iv_ia(IVValue iv) { return iv->ia; }
unsigned int  iv_ui(IVValue iv) { return iv->ui; }
unsigned int *iv_uia(IVValue iv) { return iv->uia; }
long long  iv_l(IVValue iv) { return iv->l; }
long long *iv_la(IVValue iv) { return iv->la; }
unsigned long long  iv_ul(IVValue iv) { return iv->ul; }
unsigned long long *iv_ula(IVValue iv) { return iv->ula; }
float  iv_f(IVValue iv) { return iv->f; }
float *iv_fa(IVValue iv) { return iv->fa; }
double  iv_d(IVValue iv) { return iv->d; }
double *iv_da(IVValue iv) { return iv->da; }