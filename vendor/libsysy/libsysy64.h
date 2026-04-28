#ifndef LIBSYSY64_H
#define LIBSYSY64_H

#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>

/* Input & output functions */

long long getint ();
double getfloat ();
long long getch ();
long long getarray (long long a[]);
long long getfarray (double a[]);

void putint (long long a);
void putfloat (double a);
void putch (long long a);
void putarray (long long n, long long a[]);
void putfarray (long long n, double a[]);

/* Timing function implementation */

struct timeval _sysy_start, _sysy_end;
#define _SYSY_N 1024
int _sysy_l1[_SYSY_N], _sysy_l2[_SYSY_N];
int _sysy_h[_SYSY_N], _sysy_m[_SYSY_N], _sysy_s[_SYSY_N], _sysy_us[_SYSY_N];
int _sysy_idx;
__attribute ((constructor)) void before_main ();
__attribute ((destructor)) void after_main ();
void _sysy_starttime (int lineno);
void _sysy_stoptime (int lineno);

#endif
