// Editor_001.cpp : Defines the entry point for the application.
//
#define _CRT_SECURE_NO_WARNINGS 1
#include "stdafx.h"
#include "Targa View.h"

char g_szClassName[] = "Targa View";
char a[MAX_PATH*2] = "2/rle_rgba_8.tga\0\0\0";
char list[MAX_PATH];

static HINSTANCE g_hInst = NULL;
HBITMAP hbm1, hbmTmp;
HBRUSH background = CreateSolidBrush(0x999999);

//imagem img, img2;
FILE *fi = NULL;

typedef unsigned long int		   Bool;
typedef   signed char			   schar;
typedef   signed char			   sbyte;
typedef unsigned char			   uchar;
typedef unsigned char			   byte;
typedef unsigned short			   ushort;
typedef   signed short			   sshort;
typedef unsigned long int		   uint;
typedef   signed long int		   sint;
typedef unsigned long long int	   ulong;
typedef   signed long long int	   slong;
#pragma pack(push, 1)   // n = 16, pushed to stack

//struct tgaheader {
//	uchar 	id_length; 			// 1
//	uchar	color_type;			// 2
//	uchar	image_type;			// 3		1=rawindex,2=rawrgb,3=rawgrey,9=rleidx,10=rlergb,11=rlegrey
//	ushort	color_start;		// 4 5
//	union {
//		ushort	color_length, pal_length;		// 6 7
//	};
//	uchar	pal_depth;			// 8
//	ushort	x_offset;			// 9 10
//	ushort	y_offset;			// 11 12
//	union {
//		ushort	x, xres, width; // 13 14
//	};
//	union {
//		ushort	y, yres, heigth; // 15 16
//	};
//	uchar 	bpp;				// 17
//	uchar	descriptor;			// 18
//};

struct TGA {
	uchar 	id_length;  		// 1
	uchar	color_type; 		// 2		1=indexed
	uchar	image_type; 		// 3		1=raw index pallete,2=raw rgb rgba ,3=raw grey,
	ushort	color_start; 	// 4 5 9=rle idx, 10=rle rgb rgba,11=rle grey
	union {
		ushort	color_length, pal_length;	// 6 7
	};
	union {
		uchar	color_depth, pal_depth;		// 8
	};
	ushort	x_offset; 		// 9 10
	ushort	y_offset; 		// 11 12
	union {
		ushort	x, xres, width; // 13 14
	};
	union {
		ushort	y, yres, heigth; // 15 16
	};
	uchar 	bpp; 			// 17
	uchar	orientation; 	// 18
	// if color_length > 0 then pallete are here with lenght = (256 * color_depth / bpp) em formato bgr
};

struct rgba_4 {
	ushort	r : 4;
	ushort	g : 4;
	ushort	b : 4;
	ushort	a : 4;
};
struct rgb_565 {
	ushort	r : 5;
	ushort	g : 6;
	ushort	b : 5;
};
struct rgba_5551 {
	ushort	r : 5;
	ushort	g : 5;
	ushort	b : 5;
	ushort	a : 1;
};
struct abgr_1555 {
	ushort	a : 1;
	ushort	b : 5;
	ushort	g : 5;
	ushort	r : 5;
};

struct BGR_8 {
	byte	b;
	byte	g;
	byte	r;
};
struct BGRA_8 {
	byte	b;
	byte	g;
	byte	r;
	byte	a;
};
struct rgba_8888 {
	byte	r;
	byte	g;
	byte	b;
	byte	a;
};
struct la_44 {
	byte	r : 4;
	byte	g : 4;
};


#pragma pack(pop)   // n = 2 , stack popped

//int a = sizeof(la_44);
//int b = sizeof(TGA);


struct img_basis {
	img_basis() :
		dataSize(0),
		xres(0),
		yres(0),
		zres(0),
		palSize(0),
		dataFormat(0),
		bpp(0),
		chanels(0),
		orientation(0),
		isOk(0),
		packed(0),
		dataType(0),
		flip(0),
		pal(0),
		pixels(0),
		id(0)
	{

	}
	~img_basis() {
		if (pal)
			delete pal;
		if (pixels)
			delete pixels;
		if (id)
			delete id;
	}
	void unflip();
	uint	 dataSize;
	ushort	 xres;
	ushort   yres;
	ushort   zres;
	ushort   palSize;
	uchar	 dataFormat;
	uchar    bpp;
	uchar    chanels;
	uchar	 orientation;

	uchar	 isOk;
	uchar	 packed;
	uchar	 dataType;
	uchar	 flip;


	byte* pal;
	byte* pixels;
	byte* id;
};

void img_basis::unflip() {
	if (!pixels || !xres || !yres || !bpp || !dataSize)
		//printf(0, 0);
		return;
	if (!(orientation & 0x20)) {
		byte* pi = (byte*)pixels;
		byte* pf = (byte*)pixels + dataSize - xres * (bpp / 8);
		byte* tmp = new byte[xres * (bpp / 8)];
		//if (!tmp) return;
		for (int i = 0; (i < yres) && (pi<pf); i++) {
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
	long cnt = 0;
	ulong  data = 0;
	int	len = 0;
	byte tmp = 0;
	long l = 0;
	while (cnt * bytes < dataSize)
	{
		//tmp = fgetc(f);
		tmp = 0;
		l= fgetc(f);
		if (l < 0)
		{
			//addErrorList(IMG_ERRO_FILE_SMALLER, __LINE__);
			return 1;
		}
		tmp = l;
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
		//if (h.pal_depth == 16 && h.bpp == 8 && h.color_type == 1 && h.image_type == 1)
		//	h.pal_depth = 32;
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
		//if (h.color_depth != 24 || h.color_type != 1) {
		//	delete img;
		//	return 0;
		//}
		switch (h.bpp)
		{
		case 8: // DT_IDX_8
		{
			img->dataSize = h.x * h.y * 1;
			//img->pal = new byte[(h.color_length * 3)];
			//if (!img->pal) {
			//	delete img;
			//	return 0;
			//}
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}
			byte* data = new byte[h.x * h.y * 3];
			if (!data) {
				delete img;
				return 0;
			}
			//l = fread(img->pal, 1, h.color_length * 3, f);
			l = fread(img->pixels, 1, img->dataSize, f);
			if (h.pal_depth == 16) {
				rgb_565* pal = (rgb_565*)img->pal;
				for (int i = 0; i < h.x * h.y; i++) {
					data[i * 3 + 0] = pal[img->pixels[i]].r << 4;
					data[i * 3 + 1] = pal[img->pixels[i]].g << 3;
					data[i * 3 + 2] = pal[img->pixels[i]].b << 4;
				}
			}
			else {
				BGR_8* pal = (BGR_8*)img->pal;
				for (int i = 0; i < h.x * h.y; i++) {
					data[i * 3 + 0] = pal[img->pixels[i]].b;
					data[i * 3 + 1] = pal[img->pixels[i]].g;
					data[i * 3 + 2] = pal[img->pixels[i]].r;
				}
			}
			delete img->pal;
			img->pal = 0;
			delete img->pixels;
			img->pixels = data;
			img->bpp = 24;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		case 16: // unsuported now // // DT_IDXA_8
		{
			//delete img;
			//return 0;
			img->dataSize = h.x * h.y * 2;
			//img->pal = new byte[(h.color_length * 3)];
			//if (!img->pal) {
			//	delete img;
			//	return 0;
			//}
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
			//l = fread(img->pal, 1, h.color_length * 3, f);
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
				data[i *4 + 0] = (byte)x[i].r << 3;
				data[i *4 + 1] = (byte)x[i].g << 3;
				data[i *4 + 2] = (byte)x[i].b << 3;
				data[i *4 + 3] = (byte)x[i].a << 7;
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
			if(!data) {
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
			byte* data = new byte[h.x * h.y * 3];
			if (!data) {
				delete img;
				return 0;
			}
			l = fread(img->pixels, 1, img->dataSize, f);
			for (int i = 0; i < h.x * h.y; i++) {
				data[i * 3 + 0] = img->pixels[i];
				data[i * 3 + 1] = img->pixels[i];
				data[i * 3 + 2] = img->pixels[i];
			}
			delete img->pixels;
			img->pixels = data;
			img->bpp = 24;
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
			//img->pal = new byte[(h.color_length * 3)];
			//if (!img->pal) {
			//	delete img;
			//	return 0;
			//}
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				delete img;
				return 0;
			}

			byte* data = new byte[h.x * h.y * 3];
			if (!data) {
				delete img;
				return 0;
			}
			//l = fread(img->pal, 1, h.color_length * 3, f);
			decodeTGA_RLE((uchar*)img->pixels, 1, img->dataSize, f);
			if (h.pal_depth == 16) {
				rgba_5551* pal = (rgba_5551*)img->pal;
				uint i = 0;
				for (i = 0; i < h.x * h.y; i++) {	// edit
					data[i * 3 + 0] = pal[(img->pixels[i])].r << 3;
					data[i * 3 + 1] = pal[(img->pixels[i])].g << 3;
					data[i * 3 + 2] = pal[(img->pixels[i])].b << 3;
				}
			}
			if (h.pal_depth == 24) {
				BGR_8* pal = (BGR_8*)img->pal;
				uint i = 0;
				for (i = 0; i < h.x * h.y; i++) {	// edit
					data[i * 3 + 0] = pal[(img->pixels[i])].r;
					data[i * 3 + 1] = pal[(img->pixels[i])].g;
					data[i * 3 + 2] = pal[(img->pixels[i])].b;
				}
			}
			delete img->pal;
			img->pal = 0;
			delete img->pixels;
			img->pixels = data;
			img->bpp = 24;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		case 16: // AIDX // OK // DT_IDXA_8
		{
			img->dataSize = h.x * h.y * 2;
			//img->pal = new byte[(h.color_length * 3)];
			//if (!img->pal) {
			//	delete img;
			//	return 0;
			//}
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
			//l = fread(img->pal, 1, h.color_length * 3, f);
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
			//l = fread(img->pixels, 1, img->dataSize, f);
			rgba_5551* x = (rgba_5551*)img->pixels;
			for (uint i = 0; i < (h.x * h.y); i++) {
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
			decodeTGA_RLE((uchar*)img->pixels, 3, img->dataSize, f);
			img->bpp = 24;
			img->dataSize = h.x * h.y * (img->bpp / 8);
			return img;
		}
		case 32: // rgba // OK // DT_BGRA_8
		{
			img->dataSize = h.x * h.y * 4;
			img->pixels = new byte[(img->dataSize)];
			if (!img->pixels) {
				//fclose(f);
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
			//img->dataSize = h.x * h.y * (img->bpp / 8);
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
			byte* data = new byte[h.x * h.y * 3];
			if (!data) {
				delete img;
				return 0;
			}
			decodeTGA_RLE((uchar*)img->pixels, 1, img->dataSize, f);
			for (int i = 0; i < h.x * h.y; i++) {
				data[i * 3 + 0] = img->pixels[i];
				data[i * 3 + 1] = img->pixels[i];
				data[i * 3 + 2] = img->pixels[i];
			}
			delete img->pixels;
			img->pixels = data;
			img->bpp = 24;
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
			for (int i = 0; i < img->dataSize; i++) {
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

int screenx = 300, screeny = 300;
bool adaptive=1;


BOOL GetFileName(HWND hwnd, LPSTR pszFileName, BOOL bSave)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	pszFileName[0] = 0;
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = "True Vision Targa (*.tga)\0*.tga\0All Files (*.*)\0*.*\0\0\0\0\0\0";
	ofn.lpstrFile = pszFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = "tga";
	if(bSave)
	{
		ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
		if(!GetSaveFileName(&ofn))
			return FALSE;
	}
	else
	{
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		if(!GetOpenFileName(&ofn))
			return FALSE;
	}
	return TRUE;
}

LPSTR removeEndL(LPSTR str, uint size){
	char text[100];
	sprintf(text,"%i %c %i %c %i %c %i %c %i %c", str[21], str[21], str[22], str[22], str[23], str[23], str[24], str[24], str[25], str[25]);
	MessageBox(0,text,"fi",0);

	int sz = size;
	while (--sz >= 0){
		if(str[sz] == 10){
			str[sz] = 0;
		}
		if(str[sz] == 13){
			str[sz] = 0;
		}
		//MessageBox(0," ","fi",0);
	}
	return str;
}

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwnd, IDOK);
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
	case WM_CREATE:
		{
			//fi = fopen("imgList.txt","rb");
			//if(fi)
			// {
			//	int size = fSize(fi);
			//	rewind(fi);
			//	//list = new char[100][MAX_PATH];
			//	int ct = 0;
			//	while(fgets(list, 100, fi))
			//	{
			//		removeEndL(list, 100);
			//		//MessageBox(0,list[ct],"fi",0);
			//		//g = img.loadTga(list[ct]);
			//		//MessageBox(0,list[ct],"fi",0);
			//		ct++;
			//	}
			//	int ct2 = 0;
			//	while(ct2 < ct)
			//	{
			//		g = img.loadTga(list);
			//		//MessageBox(0,list[ct],"fi",0);
			//		ct2++;
			//	}
			// }
			//img.loadTga(a);
			//img2.loadTga(a);
			//BITMAPINFO bi = {0};
			////LPVOID pData2 = img.makeXBGR_8();
			//hbm1 = img.makeHbitmap(&bi);
			//if (!hbm1){
			//	char Text[500];
			//	sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, g);
			//	MessageBox(0,Text,"bytes vazios",0);
			// }
			//if (hbm1==NULL){
			//	char Text[500];
			//	sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, g);
			//	MessageBox(0,Text,"bytes vazios",0);
			// }
		};
		break;
	case WM_PAINT:
	{
		//if(hbm1)
		//{
		BITMAP bm;
		PAINTSTRUCT ps;
		HDC hdcMem, hdc;
		HBITMAP hbmOld;
		float w1=1, w2=1;
		float x, y, scx, scy;
		if (hbm1) {
			scx = screenx;
			scy = screeny;
			GetObject(hbm1, sizeof(bm), &bm);
			x = bm.bmWidth+2;
			y = bm.bmHeight+2;

			if (scx < x) w1 = scx / x; 
			if (scy < y) w2 = scy / y; 
			x = (bm.bmWidth+2) * min(w1, w2);
			y = (bm.bmHeight+2) * min(w1, w2);

			hdc = BeginPaint(hwnd, &ps);
			hdcMem = CreateCompatibleDC(hdc);
			hbmOld = (HBITMAP)SelectObject(hdcMem, hbm1);
			GetObject(hbm1, sizeof(bm), &bm);
			//     posstr		  posend                  posstr2
			if (adaptive)
				StretchBlt(hdc, 1, 1, int(floorf(x - 1.5)), int(floorf(y - 1.5)), hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
			else
				BitBlt(hdc, 1, 1, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
			//StretchDIBits(hdc, 10, 10, 200, 200, 0, 0, bm.bmWidth, bm.bmHeight, 0, 0, 0, 0);
			SelectObject(hdcMem, hbmOld);
			EndPaint(hwnd, &ps);
			DeleteDC(hdcMem);
			DeleteObject(hbmOld);
		}
		else {
			hdc = BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
		}
		break;
	}
	case WM_SIZE:
	{
		//RECT rc;
		//if (hbm1) {
			//BITMAP bm;
			//float w1, w2;
			//float x, y, scx = LOWORD(lParam), scy = HIWORD(lParam);
			//GetObject(hbm1, sizeof(bm), &bm);
			//x = bm.bmWidth;
			//y = bm.bmHeight;
			//if (scx < x) w1 = scx / x; else w1 = 1;
			//if (scy < y) w2 = scy / y; else w2 = 1;
		screenx = LOWORD(lParam);
		screeny = HIWORD(lParam);
		//w1 = min(x, y) / max(x, y);
		//w2 = min(scx, scy) / max(scx, scy);
		//x = min(bm.bmWidth, LOWORD(lParam));
		//y = min(bm.bmHeight, HIWORD(lParam));
		//if (scx > bm.bmWidth)		w = float(scx) / float(bm.bmWidth);			else w = float(bm.bmWidth) / float(scx);
		//if (scy > bm.bmHeight)		w2 = float(scx) / float(bm.bmHeight);		else w2 = float(bm.bmHeight) / float(scx);
		//w = max(w, w2);
		//if (x < y)
		//	w = float(y) / float(x);
		//else
		//	w = float(x) / float(y);
		//screenx = bm.bmWidth / w;
		//screeny = bm.bmHeight / w;
		//GetWindowRect(hwnd, &rc);
		//onResize(LOWORD(lParam), HIWORD(lParam), wParam, lParam);
		InvalidateRect(hwnd, NULL, true);
		//}
		break;
	}
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		DeleteObject(hbm1);
		DeleteObject(hbmTmp);
		DeleteObject(background);
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
			case ID_FILE_OPEN:
				{
					DeleteObject(hbm1);
					BOOL get = GetFileName(hwnd,a,false);
					if(get)
					{
						FILE * file;
						if (!(file = fopen(a, "rb"))) return NULL;
						img_basis* img ;
						int sf = 0;
						TGA h;
						fread(&h, 1, sizeof(TGA), file);
						img = loadTGA(file, h, sf);
						fclose(file);
						if (!img) {
							printf("***********************\n*** Unsuported File ***\n");
							printf("%s\nid_length = %i\ncolor_type = %i\nimage_type = %i\n"
								"color_start = %i\npal_length = %i\npal_depth = %i\n"
								"x_offset = %i\ny_offset = %i\nxres = %i\nyres = %i\n"
								"bpp = %i\norientation = %i\n\n",
								a, h.id_length, h.color_type, h.image_type,
								h.color_start, h.pal_length, h.pal_depth,
								h.x_offset, h.y_offset, h.xres, h.yres,
								h.bpp, h.orientation);
							break;
						}

						//BYTE g = img.loadTga(a);
						//img2.loadTga(a);
						BITMAPINFO bi = {0,0,0};
						img->unflip();

						bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
						bi.bmiHeader.biWidth = img->xres;
						bi.bmiHeader.biHeight = img->yres;
						bi.bmiHeader.biSizeImage = img->dataSize;
						bi.bmiHeader.biPlanes = 1;
						bi.bmiHeader.biBitCount = img->bpp;
						bi.bmiHeader.biCompression = 0;
						void* pData = NULL;
						if (!img->pixels) {
							delete img;
							MessageBox(0, "error3: makeXRGB_8 returned NULL;", "error3", 0);
							return NULL;
						}
						hbm1 = CreateDIBSection(NULL, &bi, 0, &pData, NULL, 0);
						if (hbm1)
							SetBitmapBits(hbm1, img->dataSize, img->pixels);
						//free(pData2);


						//hbm1 = hbm;
						if (!hbm1)
						{
							char Text[500];
							sprintf(Text, "hbm = %p, %i %i %i %p", hbm1, img->xres, img->yres, img->dataSize, img->pixels);
							MessageBox(0, Text, "bytes vazios", 0);
						}
						InvalidateRect(hwnd, NULL, true);
						char Text[500];
						sprintf(Text,	"%s\n,  id_length = %i\n color_type = %i\n image_type = %i\n "
										"color_start = %i\n pal_length = %i\n pal_depth = %i\n "
										"x_offset = %i\n y_offset = %i\n xres = %i\n yres = %i\n "
										"bpp = %i\n orientation = %i\n ", 
										a, h.id_length, h.color_type, h.image_type, 
										h.color_start, h.pal_length, h.pal_depth, 
										h.x_offset, h.y_offset, h.xres, h.yres, 
										h.bpp, h.orientation );
						printf(			"%s\nid_length = %i\ncolor_type = %i\nimage_type = %i\n"
										"color_start = %i\npal_length = %i\npal_depth = %i\n"
										"x_offset = %i\ny_offset = %i\nxres = %i\nyres = %i\n"
										"bpp = %i\norientation = %i\n\n", 
										a, h.id_length, h.color_type, h.image_type, 
										h.color_start, h.pal_length, h.pal_depth, 
										h.x_offset, h.y_offset, h.xres, h.yres, 
										h.bpp, h.orientation );
						//MessageBox(0, Text, "TGA Header", 0);
						delete img;
					}
					break;
				}
			case ID_FILE_SAVE:
				break;
			case ID_FILE_SAVE_AS:
				break;
			case ID_FILE_SAVE_ALL:
				break;
			case ID_FILE_CLONE:
				break;
			case ID_FILE_CLOSE:
				break;
			case ID_FILE_CLOSE_ALL:
				break;
			case ID_FILE_EXIT:
				PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;
			case ID_HELP_ABOUT:
				DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc);
				break;
			case ID_ADAPTIVE:
				adaptive = !adaptive;
				InvalidateRect(hwnd, NULL, true);
				break;
				/*
			case ID_RANDON_CONV:
				{
					int cnt = 20000;
					int val;
					while(cnt)
					{
						val = rand();
						if(val > 2 && val < 29)
						{
							cnt--;
							img2.convertDataType(val,GRAY_R,GRAY_G,GRAY_B);
							if(val%2){
								img2.pack();
								img2.unpack();
							}
							else{
								img2.unpack();
								img2.pack();
							}
						}
					}
					BITMAPINFO bi = {0,0,0};
					HBITMAP b = img2.makeHbitmap(&bi);
					if(b){
						DeleteObject(hbm1);
						hbm1 = b;
						InvalidateRect(hwnd,NULL,true);
					}
				}
				break;
			case ID_SHOW_CONVLIST:
				{
					char *text = new char[4096];
					sprintf(text, "%s\n\n%s\n\n%s\n\n%s\n\n%s\n\n%s\n\n%s\n\n%s",
							img2.m_conversionList[0],
							img2.m_conversionList[1],
							img2.m_conversionList[2],
							img2.m_conversionList[3],
							img2.m_conversionList[4],
							img2.m_conversionList[5],
							img2.m_conversionList[6],
							img2.m_conversionList[7]);
					MessageBox(0,text, "Log List", 0);
					delete text;
				}
				break;

			case ID_IMAGEM_ROTATE90:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					//img2.loadTga(a);
					img2.rotate90(1);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_ROTATE_90:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					//img2.loadTga(a);
					img2.rotate90(2);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_ROTATE180:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					//img2.loadTga(a);
					img2.rotate90(3);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_FLIP_VERTICAL:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					//img2.loadTga(a);
					img2.flip(2);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, x=%i y=%i ds=%i b=%i 0=%i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_FLIP_HORIZONTAL:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					//img2.loadTga(a);
					img2.flip(1);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_FLIP_HV:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					//img2.loadTga(a);
					img2.flip(3);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_RGB:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					hbm1 = img.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_GRAY_HUMAN:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.loadTga(a);
					img2.convertDataType(DT_L_8,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_GRAY_HUMAN_SIMPLIFIED:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.loadTga(a);
					img2.convertDataType(DT_L_8,GRAY_271_R,GRAY_271_G,GRAY_271_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_GRAY_D3:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.loadTga(a);
					img2.convertDataType(DT_L_8,GRAY1_R,GRAY1_G,GRAY1_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_GRAY_PDF:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.loadTga(a);
					img2.convertDataType(DT_L_8,GRAY2_R,GRAY2_G,GRAY2_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_GRAY_PDF_SIMPLIFIED:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.loadTga(a);
					img2.convertDataType(DT_L_8,GRAY_361_R,GRAY_361_G,GRAY_361_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_GRAY565:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.loadTga(a);
					img2.convertDataType(DT_L_8,GRAY_565_R,GRAY_565_G,GRAY_565_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_GRAY333:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.loadTga(a);
					img2.convertDataType(DT_L_8,GRAY3_R,GRAY3_G,GRAY3_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_GRAY_AJ:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.loadTga(a);
					img2.convertDataType(DT_L_8,GRAY_AJ_R,GRAY_AJ_G,GRAY_AJ_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_IMAGEM_GRAY_PERSONAL:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.loadTga(a);
					img2.convertDataType(DT_L_8,GRAY_PS_R,GRAY_PS_G,GRAY_PS_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;

			case ID_IMAGEM_MULTIPLIALPHA:
				{

				}
				break;

			case ID_DT_L_8:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_L_8,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_LA_8:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_LA_8,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_IDX_8:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_IDX_8,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_IDXA_8:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_IDXA_8,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_RGB_8:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_RGB_8,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_BGR_8:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_BGR_8,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_RGBA_8:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_RGBA_8,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_BGRA_8:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_BGRA_8,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_ARGB_8:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_ARGB_8,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_ABGR_8:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_ABGR_8,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;


			case ID_DT_L_16:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_L_16,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_LA_16:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_LA_16,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_RGB_16:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_RGB_16,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_BGR_16:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_BGR_16,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_RGBA_16:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_RGBA_16,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_BGRA_16:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_BGRA_16,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_ARGB_16:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_ARGB_16,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_ABGR_16:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_ABGR_16,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;



			case ID_DT_L_32:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_L_32,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_LA_32:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_LA_32,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_RGB_32:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_RGB_32,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_BGR_32:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_BGR_32,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_RGBA_32:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_RGBA_32,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_BGRA_32:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_BGRA_32,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_ARGB_32:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_ARGB_32,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_ABGR_32:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.convertDataType(DT_ABGR_32,GRAY_R,GRAY_G,GRAY_B);
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;


			case ID_DT_PACK:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.pack();
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;
			case ID_DT_UNPACK:
				{
					DeleteObject(hbm1);
					BITMAPINFO bi = {0,0,0};
					img2.unpack();
					hbm1 = img2.makeHbitmap(&bi);
					if (!hbm1)
					{
						char Text[500];
						sprintf(Text, "hbm = %i, %i %i %i %i %i", hbm1, img.m_width, img.m_height,img.m_dataSize, img.m_bytes, 0);
						MessageBox(0,Text,"bytes vazios",0);
					}
					InvalidateRect(hwnd,NULL,true);
				}
				break;*/
		}
		break;
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

//int WINAPI WinMain(	_In_ HINSTANCE hInstance,	_In_opt_ HINSTANCE hPrevInstance,	_In_ LPSTR lpCmdLine,	_In_ int nShowCmd)
int main(int argc, char* argv[])
{




	WNDCLASSEX WndClass;
	HWND hwnd;
	MSG Msg;
	HACCEL hAccelTable;
	g_hInst = GetModuleHandle(NULL);
	WndClass.cbSize        = sizeof(WNDCLASSEX);
	WndClass.style         = 0;
	WndClass.lpfnWndProc   = WndProc;
	WndClass.cbClsExtra    = 0;
	WndClass.cbWndExtra    = 0;
	WndClass.hInstance     = g_hInst;
	WndClass.hIcon         = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = background;
	WndClass.lpszMenuName  = MAKEINTRESOURCE(IDR_MYMENU);
	WndClass.lpszClassName = g_szClassName;
	WndClass.hIconSm       = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);
	if(!RegisterClassEx(&WndClass))
	{
		MessageBox(0, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK | MB_SYSTEMMODAL);
		return 0;
	}
	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		"A Bitmap Program",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 320, 240,
		NULL, NULL, g_hInst, NULL);
	if(hwnd == NULL)
	{
		MessageBox(0, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK | MB_SYSTEMMODAL);
		return 0;
	}
	ShowWindow(hwnd, 1);
	UpdateWindow(hwnd);
	hAccelTable = LoadAccelerators(g_hInst, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	while(GetMessage(&Msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(Msg.hwnd, hAccelTable, &Msg))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	return Msg.wParam;
}






