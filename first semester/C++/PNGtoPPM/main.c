#define _CRT_SECURE_NO_WARNINGS
#include "return_codes.h"
#include <stdio.h>
#include <stdlib.h>

#define LIBDEFLATE

//#define ISAL

//#define ZLIB

#ifdef ZLIB

	#include <zlib.h>

#elif defined(LIBDEFLATE)

	#include <libdeflate.h>

#elif defined(ISAL)

	#include <include/igzip_lib.h>

#else
	#error wrong macros
#endif

// int has_IEND = 0;

void read_bytes(unsigned char* from, unsigned char* to, int len, long st)
{
	for (int i = 0; i < len; i++)
	{
		to[i] = from[st + i];
	}
}

int is_png(unsigned char* magic, long size)
{
	if (size < 8)
	{
		return 0;
	}

	const unsigned char magic_png[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	for (int i = 0; i < 8; i++)
	{
		if (magic[i] != magic_png[i])
		{
			return 0;
		}
	}
	return 1;
}

int is_smth(unsigned char* type, unsigned char* comp)
{
	for (int i = 0; i < 4; i++)
	{
		if (type[i] != comp[i])
		{
			return 0;
		}
	}
	return 1;
}

int is_IDAT(unsigned char* type)
{
	unsigned char IDAT[4] = { 'I', 'D', 'A', 'T' };
	return is_smth(type, IDAT);
}

int is_IEND(unsigned char* type)
{
	unsigned char IEND[4] = { 'I', 'E', 'N', 'D' };
	return is_smth(type, IEND);
}

int is_my_IEND(unsigned char* type)
{
	unsigned char iend[4] = { 'i', 'e', 'n', 'd' };
	return is_smth(type, iend);
}

int get_number(unsigned char* arr)
{
	const int power[4] = { 1, 256, 65536, 16777216 };
	int number = 0;
	for (int i = 0; i < 4; i++)
	{
		number += power[3 - i] * arr[i];
	}

	return number;
}

long get_IDAT_size(unsigned char* input_buffer, unsigned char* filling_buffer, int to_fill)
{
	long st = 33L;
	long IDAT_size = 0L;

	unsigned char* buff = 0;
	do
	{
		buff = (unsigned char*)malloc(4 * sizeof(unsigned char));
		if (NULL == buff)
		{
			fprintf(stderr, "can't allocate memory\n");
			return -1;
		}
		read_bytes(input_buffer, buff, 4, st);
		int len = get_number(buff);
		read_bytes(input_buffer, buff, 4, st + 4);

		if (is_IDAT(buff))
		{
			if (to_fill)
			{
				for (int i = 0; i < len; i++)
				{
					filling_buffer[IDAT_size + i] = input_buffer[st + 8 + i];
				}
			}
			IDAT_size += len;
		}

		st += len + 12;

	} while (!is_IEND(buff) && !is_my_IEND(buff));

	if (is_my_IEND(buff))
	{
		return -2;
	}

	free(buff);
	return IDAT_size;
}

int abss(int x, int y)
{
	return (x > y) ? x - y : y - x;
}

void fill_line(unsigned char* buffer, int filter_type, long ind, int width, int type_color)
{
	int bytes = type_color == 0 ? 1 : 3;

	for (int i = 1; i <= width; i++)
	{
		int bpp = i > bytes ? buffer[ind * (width + 1) + i - bytes] : 0;
		int upper = ind > 0 ? buffer[(ind - 1) * (width + 1) + i] : 0;
		int bpp_upper = i > bytes && ind > 0 ? buffer[(ind - 1) * (width + 1) + i - bytes] : 0;

		if (filter_type == 0)
		{
			continue;
		}
		else if (filter_type == 1)
		{
			buffer[ind * (width + 1) + i] += bpp;
		}
		else if (filter_type == 2)
		{
			buffer[ind * (width + 1) + i] += upper;
		}
		else if (filter_type == 3)
		{
			buffer[ind * (width + 1) + i] += (bpp + upper) / 2;
		}
		else if (filter_type == 4)
		{
			int p = bpp + upper - bpp_upper;
			int pa = abss(p, bpp);
			int pb = abss(p, upper);
			int pc = abss(p, bpp_upper);
			if (pa <= pb && pa <= pc)
			{
				p = bpp;
			}
			else if (pb <= pc)
			{
				p = upper;
			}
			else
			{
				p = bpp_upper;
			}
			buffer[ind * (width + 1) + i] += p;
		}
		else
		{
			fprintf(stderr, "The data in png was waste or wrong parsed\n");
		}
	}
}

void put_num(int x, FILE* fo)
{
	unsigned char a[8];

	int cnt = 0;
	while (x > 0)
	{
		a[cnt++] = (x % 10 + '0');
		x /= 10;
	}
	for (int i = cnt - 1; i >= 0; i--)
	{
		putc(a[i], fo);
	}
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		fprintf(stderr, "expected: <pic1>.png <pic2>.ppm");
		return ERROR_INVALID_PARAMETER;
	}

	FILE* input_img = fopen(argv[1], "rb");
	if (input_img == NULL)
	{
		fprintf(stderr, "file %s didn't exists", argv[1]);
		fclose(input_img);
		return ERROR_FILE_EXISTS;
	}

	// getting the size of file

	fseek(input_img, 0, SEEK_END);
	long input_size = ftell(input_img);
	rewind(input_img);

	unsigned char* input_buffer = (unsigned char*)malloc(sizeof(unsigned char) * input_size);
	if (input_buffer == NULL)
	{
		fprintf(stderr, "not enough memory\n");
		fclose(input_img);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	long result = fread(input_buffer, 1, input_size, input_img);

	for (int i = 0; i < 4; i++)
	{
		input_buffer[input_size + i] = '.';
	}
	
	input_buffer[input_size + 4] = 'i';
	input_buffer[input_size + 5] = 'e';
	input_buffer[input_size + 6] = 'n';
	input_buffer[input_size + 7] = 'd';

	if (result != input_size)
	{
		fprintf(stderr, "can't read file");
		fclose(input_img);
		free(input_buffer);
		return ERROR_UNKNOWN;
	}
	fclose(input_img);

	if (!is_png(input_buffer, input_size))
	{
		fprintf(stderr, "%s is not png!", argv[1]);
		free(input_buffer);
		return ERROR_INVALID_DATA;
	}

	// HEADER PARSING

	unsigned char len[4];
	read_bytes(input_buffer, len, 4, 16);
	int width = get_number(len);
	read_bytes(input_buffer, len, 4, 20);
	int height = get_number(len);
	read_bytes(input_buffer, len, 1, 25);
	int type_color = len[0];

	if (type_color != 0 && type_color != 2)
	{
		fprintf(stderr, "type color of %s is wrong(expected to grayscale(0) or RGB(2)), but get = %d\n", argv[1], type_color);
		free(input_buffer);
		return ERROR_INVALID_DATA;
	}

	// ========================

	long IDAT_arr_size = get_IDAT_size(input_buffer, 0, 0);
	if (IDAT_arr_size < 0)
	{
		if (IDAT_arr_size == -1)
		{
			fprintf(stderr, "Something goes wrong, while parsing IDAT(most likely memory error)\n");
		}
		else if (IDAT_arr_size == -2)
		{
			fprintf(stderr, "no IEND chunk");
		}
		free(input_buffer);
		return ERROR_UNKNOWN;
	}
	unsigned char* IDAT_source = (unsigned char*)malloc(IDAT_arr_size * sizeof(unsigned char));
	IDAT_arr_size = get_IDAT_size(input_buffer, IDAT_source, 1);
	if (IDAT_arr_size == -1)
	{
		fprintf(stderr, "Something goes wrong, while parsing IDAT(most likely memory error)\n");
		free(input_buffer);
		free(IDAT_source);
		return ERROR_UNKNOWN;
	}

	long available_size = (long)width * height * 4;
	unsigned char* buffer = (unsigned char*)malloc(available_size * sizeof(unsigned char));

#ifdef ZLIB
	uLong available_size_zlib = available_size;
	int res = uncompress(buffer, &available_size_zlib, IDAT_source, IDAT_arr_size);

	if (res == Z_MEM_ERROR)
	{
		fprintf(stderr, "not enough memory\n");
		free(IDAT_source);
		free(buffer);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	else if (res == Z_DATA_ERROR)
	{
		fprintf(stderr, "bad data...\n");
		free(IDAT_source);
		free(buffer);
		return ERROR_INVALID_DATA;
	}

#endif

#ifdef LIBDEFLATE
	size_t p = (size_t)width * height * 6;
	int res = libdeflate_zlib_decompress(libdeflate_alloc_decompressor(), IDAT_source, IDAT_arr_size, buffer, available_size, &p);
	if (res == LIBDEFLATE_BAD_DATA)
	{
		fprintf(stderr, "bad data...\n");
		free(IDAT_source);
		free(buffer);
		return ERROR_INVALID_DATA;
	}
	else if (res == LIBDEFLATE_INSUFFICIENT_SPACE)
	{
		fprintf(stderr, "not enough memory\n");
		free(IDAT_source);
		free(buffer);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
#endif

#ifdef ISAL
	struct inflate_state uncompress;
	isal_inflate_init(&uncompress);
	uncompress.next_in = IDAT_source;
	uncompress.avail_in = IDAT_arr_size;
	uncompress.next_out = buffer;
	uncompress.avail_out = available_size;
	uncompress.crc_flag = IGZIP_ZLIB;
	int res = isal_inflate(&uncompress);

	if (res == ISAL_OUT_OVERFLOW)
	{
		fprintf(stderr, "not enough memory\n");
		free(IDAT_source);
		free(buffer);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

#endif

	free(IDAT_source);

	if (res == 0)
	{
		FILE* output_image = fopen(argv[2], "wb");
		if (NULL == output_image)
		{
			fprintf(stderr, "file already exists\n");
			free(buffer);
			return ERROR_ALREADY_EXISTS;
		}

		putc('P', output_image);
		char v = (type_color == 0) ? '5' : '6';
		putc(v, output_image);
		putc('\n', output_image);
		put_num(width, output_image);
		putc(' ', output_image);
		put_num(height, output_image);
		putc('\n', output_image);
		put_num(255, output_image);
		putc('\n', output_image);

		width *= (type_color == 0) ? 1 : 3;
		for (int i = 0; i < height; i++)
		{
			int filter_type = buffer[i * (width + 1)];
			fill_line(buffer, filter_type, (long)i, width, type_color);
		}

		for (int i = 0; i < height; i++)
		{
			for (int j = 1; j <= width; j++)
			{
				putc(buffer[(long)i * (width + 1) + j], output_image);
			}
		}
		fclose(output_image);
	}
	else
	{
		fprintf(stderr, "Something goes wrong, while uncompressing data, probably data is bad");
		free(buffer);
		return ERROR_UNKNOWN;
	}
	free(buffer);
}
