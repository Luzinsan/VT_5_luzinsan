#include <iostream>
#include <windows.h>
#include <ctime>
#define BITMAP_HEADER_SIZE 40 + 14 

//11. Уменьшить яркость синей и красной составляющих у нечётных строк.

time_t ci_prog(unsigned w, unsigned h, unsigned bpp, FILE* in, FILE* out, int r, int b)
{
	time_t start = clock();
	const int q = w * bpp / 8;
	unsigned char* row = new unsigned char[q];
	
	for (int i = 0; i < h; i++)
	{
		fread(row, 1, q, in);
		fwrite(row, 1, q, out);
		fread(row, 1, q, in);
		for (int j = 0; j <= q; j += bpp / 8)
		{
			if      (row[j] - r     < 0) // RED
				     row[j]         = 0;
			else if (row[j] - r     > 255) 
				     row[j]         = 255; 
			else     row[j]        -= r;
			
			if      (row[j + 2] - b < 0) // BLUE
				     row[j + 2]     = 0;
			else if (row[j + 2] - b > 255) 
				     row[j + 2]     = 255;
			else     row[j + 2]    -= b;
		}
		fwrite(row, 1, q, out);
	}
	fseek(in, BITMAP_HEADER_SIZE, SEEK_SET);
	return clock() - start;
}

time_t asm_scal(unsigned w, unsigned h, unsigned bpp, FILE* in, FILE* out, _int16 r, _int16 b)
{
	time_t start = clock();
	_int16 C;
	const int q = w * bpp / 8;
	unsigned _int8* row = new unsigned char[q];
	unsigned i=0, j;
	
	
	_asm M1: // for(int i = 1; i < height; i++) - the first loop 
	fread(row, 1, q, in);
	fwrite(row, 1, q, out);
	_asm 
	{
		add i,2
		mov ecx, h // ecx_1 - height
		cmp i, ecx
			ja M9 // if i >= height than exit from loopes
		
		mov j, 0
	M2: //     for(int j = 0; i < width; j++) - the second loop
		mov ecx, w // ecx_2 - width
		cmp j, ecx
			ja M1 // if j = width than jump to M1 (the first loop)
	}
	fread(&C, 1, 1, in); // read color - red
	_asm
	{
		mov ax, C
		sub ax, r
		cmp ax, 0 
			jl M3 // if (R - r < 0) C = 0
			cmp ax, 255 
				jg M4 // else if (R - r > 255) C = 255
				mov C, ax // else C = R - r
				jmp M5
	M3: 
		mov C, 0  // C = 0
			jmp M5
	M4:
		mov C, 255  // C = 255
	M5:
	}
	fwrite(&C, 1, 1, out); // write red(changed)
	fread(&C, 1, 1, in); // read greed
	fwrite(&C, 1, 1, out); // write greed(didn't unchanged)
	fread(&C, 1, 1, in); // write blue
	_asm
	{
		mov ax, C
		sub ax, b
		cmp ax, 0
			jl M6 // if (B - b < 0) C = 0
			cmp ax, 255
				jg M7 // else if (B - b > 255) C = 255
				mov C, ax // else C = B - b
				jmp M8
	M6 :
		mov C, 0  // C = 0
			jmp M8
	M7 :
		mov C, 255  // C = 255
	M8 :
	}
	fwrite(&C, 1, 1, out); // write blue(changed)
		_asm inc j
		_asm	jmp M2 // jump to second loop
	_asm M9: // exit from loop
	
	fseek(in, BITMAP_HEADER_SIZE, SEEK_SET);
	return clock() - start;
}

time_t asm_MMX(unsigned w, unsigned h, unsigned bpp, FILE* in, FILE* out, int r, int b)
{
	time_t start = clock();
	const int q = w * bpp / 8;
	unsigned _int8* row = new unsigned _int8[q];
	unsigned i, j;
	_asm mov i, 0

	_asm M1: // for(int i = 1; i < height; i++) - the first loop 
	fread(row, 1, q, in);
	fwrite(row, 1, q, out);
	_asm 
	{
		add i,2
		mov ecx, h // ecx_1 - height
		cmp i, ecx
			ja M4 // if i = height than exit from loopes
		
		mov j, 0
	}
	fread(row, sizeof(unsigned _int8), q, in);
	_asm 
	{
		mov esi, row
	M2: //     for(int j = 0; i < width; j++) - the second loop
		mov ecx, w // ecx_2 - width
		cmp j, ecx
			ja M3

		movd mm0, [esi]
		movq mm1, qword ptr r
		psubusb mm0, mm1
		movd [esi], mm0

		add esi, 2
		movd mm0, [esi]
		movq mm1, qword ptr b
		psubusb mm0, mm1
		movd [esi], mm0
		
		add esi, 1
		emms
			inc j
			jmp M2
	}
	_asm M3:
	fwrite(row, sizeof(unsigned _int8), q, out);
	_asm jmp M1
	_asm M4:
	fseek(in, BITMAP_HEADER_SIZE, SEEK_SET);
	return clock() - start;
}

int main()
{
	setlocale(LC_ALL, "Rus");
	
	FILE* in;
	fopen_s(&in, "space.bmp", "rb");

	if (!in)
	{
		printf("Error: Can't open space.bmp for reading\n");
		return 0;
	}
	char header[BITMAP_HEADER_SIZE];
	fread(header, 1, BITMAP_HEADER_SIZE, in);
	fseek(in, BITMAP_HEADER_SIZE, SEEK_SET);

	unsigned w = *((unsigned*)(header + 18)), 
			 h = *((unsigned*)(header + 22)), 
			 bpp = *((unsigned*)(header + 28));
	
	printf("Ширина: %u\nВысота: %u\nГлубина цвета: %u\n", w, h, bpp);
	printf("\nНасколько уменьшить яркость красной составляющей картинки: ");
	int r;
	scanf_s("%d", &r);
	printf("\nНасколько уменьшить яркость синей составляющей картинки: ");
	int b;
	scanf_s("%d", &b);
	time_t t1=0, t2=0, t3=0;
	int count = 100;
	for (int i = 0; i < count; i++)
	{
		std::cout << "\n\tИтерация #" << i << ": " << t1 + t2 + t3;
		// С++
		{
			FILE* out;
			fopen_s(&out, "space_c.bmp", "wb");
			if (!out)
			{
				printf("Error: Can't open space_c.bmp \n");
				return 0;
			}
			fwrite(header, 1, BITMAP_HEADER_SIZE, out);
			fseek(out, BITMAP_HEADER_SIZE, SEEK_SET);
			t1 += ci_prog(w, h, bpp, in, out, r, b);
			//std::cout << "Время работы на С++: " << t1 << std::endl;
			fclose(out);
		}

		//  Cкалярные операции
		{
			FILE* out;
			fopen_s(&out, "space_scal.bmp", "wb");
			if (!out)
			{
				printf("Error: Can't space_scal.bmp \n");
				return 0;
			}
			fwrite(header, 1, BITMAP_HEADER_SIZE, out);
			fseek(out, BITMAP_HEADER_SIZE, SEEK_SET);
			t2 += asm_scal(w, h, bpp, in, out, r, b);
			//std::cout << "Время работы со скалярными операциями: " << t2 << std::endl;
			fclose(out);
		}

		// MMX
		{
			FILE* out;
			fopen_s(&out, "space_mmx.bmp", "wb");
			if (out == NULL)
			{
				printf("Error: Can't open space_mmx.bmp \n");
				return 0;
			}
			fwrite(header, 1, BITMAP_HEADER_SIZE, out);
			fseek(out, BITMAP_HEADER_SIZE, SEEK_SET);
			t3 += asm_MMX(w, h, bpp, in, out, r, b);
			//std::cout << "Время работы с MMX: " << t3 << std::endl;
			fclose(out);
		}
	}

	std::cout << "\n\tСреднее время работы программы на С++ на " 
		<< count << " итераций: " << t1 / count << std::endl;
	std::cout << "\n\tСреднее время работы со скалярными операциями на " 
		<< count << " итераций: " << t2 / count << std::endl;
	std::cout << "\n\tСреднее время работы с MMX на " 
		<< count << " итераций: " << t3 / count << std::endl;

	return 0;
}
