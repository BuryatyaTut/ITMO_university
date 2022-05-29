#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <algorithm>
#include <vector>
#include <fstream>
#include <omp.h>
using namespace std;
FILE *fp;
FILE *fo;
uint8_t c;
int new_val(double old, double minn, double maxx)
{
    if (maxx > minn)
    {
        return (old / maxx - ((1 - old / maxx) / (1 - minn / maxx) * minn / maxx)) *
               255;
    }
    return maxx;
}
void get_num(int &x)
{
    while (true)
    {
        c = getc(fp);
        if (c == ' ' || c == '\n')
            return;
        x = x * 10 + (c - '0');
    }
}
void put_num(int x)
{
    vector<uint8_t> a;
    while (x > 0)
    {
        a.push_back(x % 10 + '0');
        x /= 10;
    }
    for (int i = a.size() - 1; i >= 0; i--)
    {
        putc(a[i], fo);
    }
}
void put_header(char v, int w, int h)
{
    putc('P', fo);
    putc(v, fo);
    putc('\n', fo);
    put_num(w);
    putc(' ', fo);
    put_num(h);
    putc('\n', fo);
    put_num(255);
    putc('\n', fo);
}
void get_header(string &ver, int &w, int &h, int &num, int &type)
{
    c = getc(fp);
    ver += c;
    c = getc(fp);
    ver += c;
    c = getc(fp);
    type = (ver == "P6") ? 3 : 1;
    get_num(w);
    get_num(h);
    get_num(num);
}
int find_minim(vector<double> &val_cnt, double k)
{
    for (int i = 0; i < 256; i++)
    {
        if (val_cnt[i] > k)
        {
            return i;
        }
        else
        {
            k -= val_cnt[i];
        }
    }
}
int find_maxim(vector<double> &val_cnt, double k)
{
    for (int i = 255; i >= 0; i--)
    {
        if (val_cnt[i] > k)
        {
            return i;
        }
        else
        {
            k -= val_cnt[i];
        }
    }
}
string get_type_file(string name)
{
    string type = "";
    bool ok = false;
    for (int i = 0; i < name.size(); i++)
    {
        if (name[i] == '.')
            ok = true;
        else if (ok)
            type += name[i];
    }
    return type;
}
bool is_support(string file)
{
    return file == "ppm" || file == "pgm";
}
int main(int amount, char **args)
{
    int threads_cnt = 0;
    for (int i = 0; i < 32; i++)
    {
        char n = args[1][i];
        if (!isdigit(n))
        {
            break;
        }
        threads_cnt = threads_cnt * 10 + (n - '0');
    }
    double k = 0;
    cin >> k;
    omp_set_dynamic(0);
    if (threads_cnt != 0)
        omp_set_num_threads(threads_cnt);
    string file_one = get_type_file(args[2]);
    string file_two = get_type_file(args[3]);
    if (!is_support(file_one) || !is_support(file_two))
    {
        cout << "this programm didn't support that type of files\n";
        return 0;
    }
    fp = fopen(args[2], "rb");
    if (fp == NULL)
    {
        cout << "Something goes wrong. Please check your file\n";
        return 0;
    }
    string ver;
    int w = 0, h = 0, num = 0, type = 0;
    get_header(ver, w, h, num, type);

    int image_size = w * h * type;
    vector<int> bytes(image_size);
    vector<double> val_cnt(256);
    int minim = 256;
    int maxim = -1;
    for (int i = 0; i < image_size; i++)
    {
        c = getc(fp);
        bytes[i] = c;
    }
    fclose(fp);
    double st = omp_get_wtime();

#pragma omp parallel
    {
        int cnt_private[256] = {0};
#pragma omp for schedule(static, 4)
        for (int i = 0; i < image_size; ++i)
        {
            cnt_private[bytes[i]]++;
        }
#pragma omp critical
        {
            for (int i = 0; i < 256; i++)
            {
                val_cnt[i] += cnt_private[i];
            }
        }
    }

    for (int i = 0; i < 256; i++)
    {
        val_cnt[i] /= image_size;
    }
    minim = find_minim(val_cnt, k);
    maxim = find_maxim(val_cnt, k);
    fo = fopen(args[3], "wb");
    put_header(ver == "P6" ? '6' : '5', w, h);
#pragma omp parallel for schedule(static, 4)
    for (int i = 0; i < image_size; i++)
    {
        bytes[i] = min(new_val(bytes[i], minim, maxim), 255);
    }
    double end = omp_get_wtime();
    printf("Time (%i thread(s): %g ms\n", threads_cnt, (end - st) * 100);
    for (int i = 0; i < image_size; i++)
    {
        putc(bytes[i], fo);
    }
    fclose(fo);
}