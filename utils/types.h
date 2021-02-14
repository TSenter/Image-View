#ifndef __TYPES_IV
#define __TYPES_IV

#define VT_STRING 1
#define VT_CHAR 2
#define VT_SHORT 3
#define VT_INT 4
#define VT_FLOAT 5
#define VT_LONG 6
#define VT_DOUBLE 7
#define VT_VOID 8

typedef union ivvalue_t {
    char *str;
    void *v;

    char c;
    char ca[8];
    unsigned char uc;
    unsigned char uca[8];

    short s;
    short sa[4];
    unsigned short us;
    unsigned short usa[4];

    int i;
    int ia[2];
    unsigned int ui;
    unsigned int uia[2];

    long long l;
    long long *la;
    unsigned long long ul;
    unsigned long long *ula;
    
    float f;
    float fa[2];

    double d;
    double *da;
} *IVValue;

IVValue new_iv_str(char *);
IVValue new_iv_v(void *);
IVValue new_iv_c(char);
IVValue new_iv_ca(char [8]);
IVValue new_iv_uc(unsigned char);
IVValue new_iv_uca(unsigned char [8]);
IVValue new_iv_s(short);
IVValue new_iv_sa(short [4]);
IVValue new_iv_us(unsigned short);
IVValue new_iv_usa(unsigned short [4]);
IVValue new_iv_i(int);
IVValue new_iv_ia(int [2]);
IVValue new_iv_ui(unsigned int);
IVValue new_iv_uia(unsigned int [2]);
IVValue new_iv_l(long long);
IVValue new_iv_la(long long *);
IVValue new_iv_ul(unsigned long long);
IVValue new_iv_ula(unsigned long long *);
IVValue new_iv_f(float);
IVValue new_iv_fa(float [2]);
IVValue new_iv_d(double);
IVValue new_iv_da(double *);

char *iv_str(IVValue);
void *iv_v(IVValue);
char  iv_c(IVValue);
char *iv_ca(IVValue);
unsigned char  iv_uc(IVValue);
unsigned char *iv_uca(IVValue);
short  iv_s(IVValue);
short *iv_sa(IVValue);
unsigned short  iv_us(IVValue);
unsigned short *iv_usa(IVValue);
int  iv_i(IVValue);
int *iv_ia(IVValue);
unsigned int  iv_ui(IVValue);
unsigned int *iv_uia(IVValue);
long long  iv_l(IVValue);
long long *iv_la(IVValue);
unsigned long long  iv_ul(IVValue);
unsigned long long *iv_ula(IVValue);
float  iv_f(IVValue);
float *iv_fa(IVValue);
double  iv_d(IVValue);
double *iv_da(IVValue);

#endif