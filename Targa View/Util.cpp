#include "stdafx.h"
#include "Targa View.h"
#include "Util.h"

#include <stdio.h>
//#include "../../../jpeg-9d/jpeglib.h"
//#include <setjmp.h>

void img_basis::unflip() {
	if (!pixels || !xres || !yres || !bpp || !dataSize)
		//printf(0, 0);
		return;
	if (!(orientation & 0x20)) {
		byte* pi = (byte*)pixels;
		byte* pf = (byte*)pixels + dataSize - xres * (bpp / 8);
		byte* tmp = new byte[xres * (bpp / 8)];
		//if (!tmp) return;
		for (int i = 0; (i < yres) && (pi < pf); i++) {
			memcpy(tmp, pf, xres * (bpp / 8));
			memcpy(pf, pi, xres * (bpp / 8));
			memcpy(pi, tmp, xres * (bpp / 8));
			pi += xres * (bpp / 8);
			pf -= xres * (bpp / 8);
		}
		delete[] tmp;
	}
}

BOOL isInvalidTGA(TGA* h) {
	//return 0;
	// next modification, translate ifs for switch to make more readable and ease to maintain and add more opitions to suported file format // not needed anymore, switch on loader.
	// it is only for initial security and later testes on unknown files.
	if (!(h->color_type == 0 || h->color_type == 1) ||
		(h->xres == 0 || h->yres == 0 || h->color_start != 0) ||
		!(h->bpp == 8 || h->bpp == 15 || h->bpp == 16 || h->bpp == 24 || h->bpp == 32) ||
		!(h->image_type == 1 || h->image_type == 2 || h->image_type == 3 || h->image_type == 9 || h->image_type == 10 || h->image_type == 11) ||
		!(h->color_length == 0 || h->color_length == 256) ||
		!(h->color_depth == 0 || h->color_depth == 24) //&&
		// !(h.orientation == 0 || h.orientation == 1 || h.orientation == 8 ) // not required
		)
		return 1;//IMG_ERRO_UNSUPORTED_FORMAT;
	return 0;
}

BOOL isUnsuportedTGA(TGA* h) {
	//return 0;
	// second verirification (logic)
	if ((h->color_type == 1 &&
		(!(h->image_type == 1 || h->image_type == 9) || h->color_length < 1 || h->color_depth != 24 || !(h->bpp == 8 || h->bpp == 16))) ||
		(h->image_type == 3 && h->bpp != 8)
		)
		return 1;
	return 0;
}

uchar decodeTGA_RLE(uchar* pData, uchar bytes, size_t dataSize, FILE* f) {
	ulong cnt = 0;
	ulong  data = 0;
	int	len = 0;
	byte tmp = 0;
	long l = 0;
	while (cnt * bytes < dataSize)
	{
		//tmp = fgetc(f);
		tmp = 0;
		l = fgetc(f);
		if (l < 0)
		{
			//addErrorList(IMG_ERRO_FILE_SMALLER, __LINE__);
			return 1;
		}
		tmp = (byte)l;
		if ((tmp & 0x80))
		{
			len = tmp & 0x7F;
			//if (fread(&data, 1, bytes, f) != bytes)
			l = fread(&data, 1, bytes, f);
			if (l != bytes)
			{
				//addErrorList(IMG_ERRO_FILE_SMALLER, __LINE__);
				return 0;
			}
			while ((len >= 0) && (cnt * bytes < dataSize))
			{
				memcpy(pData, &data, bytes);
				pData += bytes;
				len--;
				cnt++;
			}
		}
		else
		{
			len = tmp & 0x7F;
			while ((len >= 0) && (cnt * bytes < dataSize))
			{
				//if (fread(pData, 1, bytes, f) != bytes)
				l = fread(pData, 1, bytes, f);
				if (l != bytes)
				{
					//addErrorList(IMG_ERRO_FILE_SMALLER, __LINE__);
					return 0;
				}
				pData += bytes;
				len--;
				cnt++;
			}
		}
	}
	return 0;
}

img_basis* loadTGA(FILE* f, TGA& h, int sf) {
	img_basis* img = new img_basis;
	uchar ret = 0,
		inv = 0,
		* pr = 0, * pg = 0, * pb = 0, * pa = 0,
		* bpal = 0,
		* pbytes = 0,
		* data = 0,
		* pdata = 0; // data is stored here
	//FILE* f = 0;
	int r = 0, g = 0, b = 0,
		cnt1 = 0, cnt2 = 0, tmp = 0;
	size_t size = 0;
	ULONG l = 0;
	img->xres = h.x;
	img->yres = h.y;
	if (h.bpp == 15)
		h.bpp = 16;
	img->bpp = h.bpp;
	img->chanels = h.bpp / 8;
	if (h.pal_length && h.color_depth && h.color_type)
		img->palSize = h.pal_length * (h.color_depth / 8);
	inv = !(h.orientation & 0x20);
	img->orientation = h.orientation;
	if (h.id_length) {
		img->id = new byte[h.id_length + 1];
		if (!img->id) {
			delete img;
			return 0;
		}
		img->id[h.id_length] = 0;
		fread(img->id, 1, h.id_length, f);
	}
	if (h.pal_length || h.pal_depth) {
		if (h.pal_depth == 15) h.pal_depth = 16;
		img->palSize = h.pal_length * h.pal_depth / 8;
		img->pal = new byte[img->palSize];
		if (!img->pal) {
			delete img;
			return 0;
		}
		fread(img->pal, 1, img->palSize, f);
	}


	switch (h.image_type) // win hbitmap = BGRA_8 BGR_8
	{
	case 1: // 1=raw index pallete // OK
	{
		switch (h.bpp)
		{
		case 8: // DT_IDX_8
		{
			img->dataSize = h.x * h.y * 1;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			byte* data = new byte[h.x * h.y * 4];
			if (!data) {
				delete img;
				return 0;
			}
			l = fread(img->pixels, 1, img->dataSize, f);
			if (h.pal_depth == 16) {
				rgb_565* pal = (rgb_565*)img->pal;
				for (int i = 0; i < h.x * h.y; i++) {
					data[i * 4 + 0] = pal[img->pixels[i]].r << 4;
					data[i * 4 + 1] = pal[img->pixels[i]].g << 3;
					data[i * 4 + 2] = pal[img->pixels[i]].b << 4;
					data[i * 4 + 3] = 255;
				}
			}
			else {
				BGR_8* pal = (BGR_8*)img->pal;
				for (int i = 0; i < h.x * h.y; i++) {
					data[i * 4 + 0] = pal[img->pixels[i]].b;
					data[i * 4 + 1] = pal[img->pixels[i]].g;
					data[i * 4 + 2] = pal[img->pixels[i]].r;
					data[i * 4 + 3] = 255;
				}
			}
			delete img->pal;
			img->pal = 0;
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		case 16: // suported now // // DT_IDXA_8
		{
			img->dataSize = h.x * h.y * 2;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			byte* data = new byte[h.x * h.y * 4];
			if (!data) {
				delete img;
				return 0;
			}
			l = fread(img->pixels, 1, img->dataSize, f);
			for (int i = 0; i < h.x * h.y; i++) {
				data[i * 4 + 0] = img->pal[img->pixels[i * 2] + 0];
				data[i * 4 + 1] = img->pal[img->pixels[i * 2] + 1];
				data[i * 4 + 2] = img->pal[img->pixels[i * 2] + 2];
				data[i * 4 + 3] = img->pal[img->pixels[i * 2 + 1] + 2];
			}
			delete img->pal;
			img->pal = 0;
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		default:
		{
			delete img;
			return 0;
		}
		}
	}
	case 2: // 2=raw rgb rgba // OK
	{
		switch (h.bpp)
		{
		case 16: // rgba5551 // OK // DT_B5G5R5A1
		{
			img->dataSize = h.x * h.y * 2;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			byte* data = new byte[h.x * h.y * 4];
			if (!data) {
				delete img;
				return 0;
			}
			l = fread(img->pixels, 1, img->dataSize, f);
			rgba_5551* x = (rgba_5551*)img->pixels;
			for (int i = 0; i < h.x * h.y; i++) {
				data[i * 4 + 0] = (byte)x[i].r << 3;
				data[i * 4 + 1] = (byte)x[i].g << 3;
				data[i * 4 + 2] = (byte)x[i].b << 3;
				data[i * 4 + 3] = (byte)x[i].a << 7;
			}
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		case 24: // RGB // OK // DT_BGR_8
		{
			img->dataSize = h.x * h.y * 3;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			byte* data = new byte[h.x * h.y * 4];
			if (!data) {
				delete img;
				return 0;
			}
			l = fread(img->pixels, 1, img->dataSize, f);
			for (int i = 0; i < h.x * h.y; i++) {
				data[i * 4 + 0] = img->pixels[i * 3 + 0];
				data[i * 4 + 1] = img->pixels[i * 3 + 1];
				data[i * 4 + 2] = img->pixels[i * 3 + 2];
				data[i * 4 + 3] = 255;
			}
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		case 32: // RGBA // OK // DT_BGRA_8
		{
			img->dataSize = h.x * h.y * 4;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			l = fread(img->pixels, 1, img->dataSize, f);
			img->bpp = 32;
			return img;
		}
		case 31: // RGBA // OK // DT_RGBA_8
		{
			img->pixels = new byte[(h.x * h.y * 4)];
			if (!img->pixels)
			{
				delete img;
				return 0;
			}
			l = fread(img->pixels, 1, (h.x * h.y * 4), f);
			img->dataSize = h.x * h.y * 4;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		default:
		{
			delete img;
			return 0;
		}
		}
	}
	case 3: // 3=raw grey // OK
	{
		switch (h.bpp)
		{
		case 8: // A // DT_L_8
		{
			img->dataSize = h.x * h.y * 1;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			byte* data = new byte[h.x * h.y * 4];
			if (!data) {
				delete img;
				return 0;
			}
			l = fread(img->pixels, 1, img->dataSize, f);
			for (int i = 0; i < h.x * h.y; i++) {
				data[i * 4 + 0] = img->pixels[i];
				data[i * 4 + 1] = img->pixels[i];
				data[i * 4 + 2] = img->pixels[i];
				data[i * 4 + 3] = 255;
			}
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		case 16: // LA // DT_LA_8
		{
			img->dataSize = h.x * h.y * 2;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			byte* data = new byte[h.x * h.y * 4];
			if (!data) {
				delete img;
				return 0;
			}
			l = fread(img->pixels, 1, img->dataSize, f);
			for (int i = 0; i < h.x * h.y; i++) {
				data[i * 4 + 0] = img->pixels[i];
				data[i * 4 + 1] = img->pixels[i];
				data[i * 4 + 2] = img->pixels[i];
				data[i * 4 + 3] = img->pixels[i + 1];
			}
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		default:
		{
			delete img;
			return 0;
		}
		}
	}
	case 9:// 9=rle idx // OK
	{
		switch (h.bpp)
		{
		case 8: // IDX // OK // DT_IDX_8
		{
			img->dataSize = h.x * h.y * 1;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}

			byte* data = new byte[h.x * h.y * 4];
			if (!data) {
				delete img;
				return 0;
			}
			decodeTGA_RLE((uchar*)img->pixels, 1, img->dataSize, f);
			if (h.pal_depth == 16) {
				rgba_5551* pal = (rgba_5551*)img->pal;
				uint i = 0;
				for (i = 0; i < uint(h.x * h.y); i++) {	// edit
					data[i * 4 + 0] = pal[(img->pixels[i])].r << 3;
					data[i * 4 + 1] = pal[(img->pixels[i])].g << 3;
					data[i * 4 + 2] = pal[(img->pixels[i])].b << 3;
					data[i * 4 + 3] = 255;
				}
			}
			if (h.pal_depth == 24) {
				BGR_8* pal = (BGR_8*)img->pal;
				uint i = 0;
				for (i = 0; i < uint(h.x * h.y); i++) {	// edit
					data[i * 4 + 0] = pal[(img->pixels[i])].r;
					data[i * 4 + 1] = pal[(img->pixels[i])].g;
					data[i * 4 + 2] = pal[(img->pixels[i])].b;
					data[i * 4 + 3] = 255;
				}
			}
			delete img->pal;
			img->pal = 0;
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		case 16: // AIDX // OK // DT_IDXA_8
		{
			img->dataSize = h.x * h.y * 2;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			byte* data = new byte[h.x * h.y * 4];
			if (!data) {
				delete img;
				return 0;
			}
			decodeTGA_RLE((uchar*)img->pixels, 2, img->dataSize, f);
			for (int i = 0; i < h.x * h.y; i++) {
				data[i * 4 + 0] = img->pal[img->pixels[i * 2] + 0];
				data[i * 4 + 1] = img->pal[img->pixels[i * 2] + 1];
				data[i * 4 + 2] = img->pal[img->pixels[i * 2] + 2];
				data[i * 4 + 3] = img->pixels[i * 2 + 1];
			}
			delete img->pal;
			img->pal = 0;
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		default:
		{
			delete img;
			return 0;
		}
		}
	}
	case 10: // 10=rle rgb rgba // OK
	{
		switch (h.bpp)
		{
		case 16: // rgba5551 // OK // DT_B5G5R5A1
		{
			img->dataSize = h.x * h.y * 2;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			byte* data = new byte[h.x * h.y * 4];
			if (!data) {
				delete img;
				return 0;
			}
			decodeTGA_RLE((uchar*)img->pixels, 2, img->dataSize, f);
			rgba_5551* x = (rgba_5551*)img->pixels;
			for (uint i = 0; i < uint(h.x * h.y); i++) {
				data[i * 4 + 0] = (byte)x[i].r << 3;
				data[i * 4 + 1] = (byte)x[i].g << 3;
				data[i * 4 + 2] = (byte)x[i].b << 3;
				data[i * 4 + 3] = (byte)x[i].a << 7;
			}
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		case 24: // rgb // OK // DT_BGR_8
		{
			img->dataSize = h.x * h.y * 3;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			byte* data = new byte[h.x * h.y * 4];
			if (!data) {
				delete img;
				return 0;
			}
			decodeTGA_RLE((uchar*)img->pixels, 3, img->dataSize, f);
			for (uint i = 0; i < uint(h.x * h.y); i++) {
				data[i * 4 + 0] = img->pixels[i * 3 + 0];
				data[i * 4 + 1] = img->pixels[i * 3 + 1];
				data[i * 4 + 2] = img->pixels[i * 3 + 2];
				data[i * 4 + 3] = 255;
			}
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		case 32: // rgba // OK // DT_BGRA_8
		{
			img->dataSize = h.x * h.y * 4;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			decodeTGA_RLE((uchar*)img->pixels, 4, img->dataSize, f);
			//for (int i = 0; i < h.x * h.y * 4; i += 4) {
				//byte tmp = img->pixels[i];
				//img->pixels[i + 1] = img->pixels[i + 3];
				//img->pixels[i + 2] = img->pixels[i + 0];
				//img->pixels[i + 2] = img->pixels[i + 3];
				//img->pixels[i + 3] = tmp;
			//}
			img->bpp = 32;
			return img;
		}
		default:
		{
			delete img;
			return 0;
		}
		}
	}
	case 11: // 11=rle grey // OK
	{
		switch (h.bpp)
		{
		case 8: // A // OK // DT_L_8
		{
			img->dataSize = h.x * h.y * 1;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			byte* data = new byte[h.x * h.y * 4];
			if (!data) {
				delete img;
				return 0;
			}
			decodeTGA_RLE((uchar*)img->pixels, 1, img->dataSize, f);
			for (int i = 0; i < h.x * h.y; i++) {
				data[i * 4 + 0] = img->pixels[i];
				data[i * 4 + 1] = img->pixels[i];
				data[i * 4 + 2] = img->pixels[i];
				data[i * 4 + 3] = 255;
			}
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		case 16: // LA // OK // DT_LA_8
		{
			img->dataSize = h.x * h.y * 2;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			byte* data = new byte[h.x * h.y * 4];
			if (!data) {
				delete img;
				return 0;
			}
			decodeTGA_RLE((uchar*)img->pixels, 2, img->dataSize, f);
			for (int i = 0; i < h.x * h.y; i++) {
				data[i * 4 + 0] = img->pixels[i * 2];
				data[i * 4 + 1] = img->pixels[i * 2];
				data[i * 4 + 2] = img->pixels[i * 2];
				data[i * 4 + 3] = img->pixels[i * 2 + 1];
			}
			delete img->pixels;
			img->pixels = data;
			img->bpp = 32;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		default:
		{
			delete img;
			return 0;
		}
		}
	}
	default:
	{
		delete img;
		return 0;
	}
	}
	return 0;
}

LPSTR removeEndL(LPSTR str, uint size) {
	char text[100];
	sprintf(text, "%i %c %i %c %i %c %i %c %i %c", str[21], str[21], str[22], str[22], str[23], str[23], str[24], str[24], str[25], str[25]);
	MessageBox(0, text, "fi", 0);
	int sz = size;
	while (--sz >= 0) {
		if (str[sz] == 10) {
			str[sz] = 0;
		}
		if (str[sz] == 13) {
			str[sz] = 0;
		}
	}
	return str;
}









