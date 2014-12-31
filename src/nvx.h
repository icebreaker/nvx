#ifndef NVX_HPP
#define NVX_HPP

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define NVX_VERSION 0x001
#define NVX_VERSION_STR "0.0.1"

#define NVX_UNUSED(x)	(void)(x)
#define NVX_BIT(x)		(1 << (x))
#define NVX_COUNT(x)	(sizeof(x) / sizeof(x[0]))

#ifndef NVX_MAX_PATH
	#ifdef MAX_MATH
		#define NVX_MAX_PATH MAX_MATH
	#else
		#define NVX_MAX_PATH 256
	#endif
#endif

#ifndef nvx_alloc
	#define nvx_alloc malloc
#endif

#ifndef nvx_free
	#define nvx_free free
#endif

typedef enum
{
	NVX_ARGS_IMAGE_FRONT	= NVX_BIT(1),
#if 0
	NVX_ARGS_IMAGE_BACK		= NVX_BIT(2),
	NVX_ARGS_IMAGE_SIDE		= NVX_BIT(3),
#endif
	NVX_ARGS_MODEL_OUTPUT	= NVX_BIT(4),
	NVX_ARGS_UNIT			= NVX_BIT(5),
	NVX_ARGS_DEPTH			= NVX_BIT(6)
} nvx_args_flags;

typedef struct
{
	char image_front[NVX_MAX_PATH];
#if 0
	char image_back[NVX_MAX_PATH];
	char image_side[NVX_MAX_PATH];
#endif
	char model_output[NVX_MAX_PATH];

	float unit;
	int depth;

	nvx_args_flags flags;
} nvx_args;

#ifndef NVX_IMPLEMENTATION
const char *nvx_get_error(void);

nvx_model *nvx_model_create(const nvx_args *args);
void nvx_model_free(nvx_model *model);

int nvx_convert_image_to_model(const nvx_args *args);
#else
static unsigned long nvx_rgb_hash(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned long h = 5381;
#define NVX_RGB_HASH(y, x) y = ((y << 5) + y) + x
	NVX_RGB_HASH(h, r);
	NVX_RGB_HASH(h, g);
	NVX_RGB_HASH(h, b);
#undef NVX_RGB_HASH
    return h;
}

static char _nvx_error[4096] = {0,};

static void nvx_set_error(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vsnprintf(_nvx_error, NVX_COUNT(_nvx_error), format, args);
	va_end(args);
}

const char *nvx_get_error(void)
{
	return _nvx_error;	 
}

#define NVX_TGA_UNCOMPRESSED 2
#define NVX_TGA_BPP 4

#pragma pack(push, 1)
typedef struct
{
	char  idlength;
	char  colourmaptype;
	char  datatypecode;
	short int colourmaporigin;
	short int colourmaplength;
	char  colourmapdepth;
	short int x_origin;
	short int y_origin;
	short width;
	short height;
	char  bitsperpixel;
	char  imagedescriptor;
} nvx_tga_header;
#pragma pack(pop)

#define MAX_IMAGE_DIM 32
#define NVX_MAX_COLORS MAX_IMAGE_DIM * MAX_IMAGE_DIM
#define NVX_COLORS_INDEX_MASK (NVX_MAX_COLORS - 1)

typedef struct
{
	int w;
	int h;
	int bpp;
	int size;
	unsigned char *pixels;
} nvx_image;

#define nvx_vec3_set(v, x, y, z) v[0] = x; v[1] = y; v[2] = z

typedef float nvx_vec3[3];
typedef unsigned char nvx_color[3];

typedef struct
{
	nvx_vec3 pos;
	nvx_color col;
} nvx_voxel;

typedef struct
{
	nvx_voxel *voxels;
	int count;
	float unit;
} nvx_model;

static nvx_image *nvx_image_load_tga_from_file(const char *filename);
static int nvx_model_write_obj_file(const nvx_model *model, const char *filename);

typedef struct
{
	const char *extension;
	nvx_image *(*load)(const char *filename);
} nvx_image_loader;

static nvx_image_loader _nvx_image_loaders[] =
{
	{ ".tga", &nvx_image_load_tga_from_file },
	{ NULL, NULL }
};
static int _nvx_image_loaders_count = NVX_COUNT(_nvx_image_loaders) - 1;

typedef struct
{
	const char *extension;
	int (*write)(const nvx_model *model, const char *filename);
} nvx_model_writer;

static nvx_model_writer _nvx_model_writers[] =
{
	{ ".obj", &nvx_model_write_obj_file },
	{ NULL, NULL }
};
static int _nvx_model_writers_count = NVX_COUNT(_nvx_model_writers) - 1;

static nvx_image *nvx_image_load_tga_from_file(const char *filename)
{
	FILE *fp;
	size_t size;
	unsigned char *pixels;

	nvx_tga_header tga;
	nvx_image *image = NULL;

	fp = fopen(filename, "rb");
	if(fp == NULL)
	{
		nvx_set_error("Could not read image header. Image `%s` is invalid.", filename);
		return NULL;
	}

	if(fread(&tga, 1, sizeof(tga), fp) != sizeof(tga))
	{
		nvx_set_error("Couldn't open image `%s`.", filename);
		goto error;
	}

	if(tga.datatypecode != NVX_TGA_UNCOMPRESSED)
	{
		nvx_set_error("Image `%s` is not uncompressed.", filename);
		goto error;
	}
		
	if(tga.bitsperpixel >> 3 != NVX_TGA_BPP)
	{
		nvx_set_error("Image `%s` is not 32-bit (RGBA).", filename);
		goto error;
	}

	if(tga.width == 0 || tga.width > MAX_IMAGE_DIM || tga.height == 0 || tga.height > MAX_IMAGE_DIM)
	{
		nvx_set_error("Image `%s` width or height is larger than 32 pixels.", filename);
		goto error;
	}

	size = tga.width * tga.height * NVX_TGA_BPP * sizeof(unsigned char);

	pixels = nvx_alloc(size);
	if(pixels == NULL)
	{
		nvx_set_error("Failed to allocate enough memory for image `%s`.", filename);
		goto error;
	}

	if(fread(pixels, 1, size, fp) != size)
	{
		nvx_free(pixels);
		nvx_set_error("Could not read image data. Image `%s` is invalid.", filename);
		goto error;
	}
	
	image = nvx_alloc(sizeof(*image));
	if(image == NULL)
	{
		nvx_free(pixels);
		nvx_set_error("Failed to allocate memory for image `%s`.", filename);
		goto error;
	}

	image->size = size;
	image->bpp = NVX_TGA_BPP;
	image->pixels = pixels;
	image->w = tga.width;
	image->h = tga.height;

error:
	fclose(fp);
	return image;
}

static nvx_image *nvx_image_load_from_file(const char *filename)
{
	int i, len;
	const char *extension;
	
	len = strlen(filename) - 4;
	if(len <= 0)
	{
		nvx_set_error("Image %s doesn't have a valid extension.", filename);
		return NULL;
	}
	extension = filename + len;

	for(i=0; i<_nvx_image_loaders_count; i++)
	{
		nvx_image_loader *image_loader = &_nvx_image_loaders[i];

		if(!strncmp(extension, image_loader->extension, 4))
			return image_loader->load(filename);
	}

	nvx_set_error("Image %s is unsupported.", filename);
	return NULL;
}

static void nvx_image_free(nvx_image *image)
{
	if(image != NULL)
	{
		if(image->pixels != NULL)
			free(image->pixels);

		nvx_free(image);
	}
}

static int nvx_model_write_obj_mat_file(nvx_color *colors, short count, const char *filename)
{
	FILE *fp;
	short i;

	fp = fopen(filename, "w");
	if(fp == NULL)
	{
		nvx_set_error("Couldn't open material `%s` for writing. Read-only?", filename);
		return -1;
	}

	fprintf(fp, "# Generated by NVX %s\n", NVX_VERSION_STR);

	for(i=0; i<count; i++)
	{
		float r = colors[i][0] / 255.0f;
		float g = colors[i][1] / 255.0f;
		float b = colors[i][2] / 255.0f;

		fprintf(fp, "\nnewmtl color_%d\n", i);
		fprintf(fp, "Kd %.04f %.04f %.04f\n", r, g, b);
	}

	fclose(fp);
	return 0;
}

static int nvx_model_write_obj_file(const nvx_model *model, const char *filename)
{
	FILE *fp;
	int i, j, len;
	float nu, pu;
	char material_filename[NVX_MAX_PATH + 1];
	const char *material_filename_only;
	short colors_index[NVX_MAX_COLORS];
	nvx_color colors[NVX_MAX_COLORS];
	short colors_count;

	fp = fopen(filename, "w");
	if(fp == NULL)
	{
		nvx_set_error("Couldn't open model `%s` for writing. Read-only?", filename);
		return -1;
	}
	
	len = strlen(filename);
	strncpy(material_filename, filename, len);
	material_filename[len-0] = '\0';
	material_filename[len-1] = 'l';
	material_filename[len-2] = 't';
	material_filename[len-3] = 'm';

	material_filename_only = material_filename + len;
	while(--material_filename_only != material_filename)
	{
		char c = *material_filename_only;
		if(c == '/' || c == '\\')
		{
			material_filename_only++;
			break;	 
		}
	}

	fprintf(fp, "# Generated by NVX %s\n\n", NVX_VERSION_STR);

	fprintf(fp, "mtllib %s\n\n", material_filename_only);

	nu = -model->unit;
	pu = model->unit;

	for(i=0; i<model->count; i++)
	{
		nvx_voxel *voxel = &model->voxels[i];

		float x = voxel->pos[0];
		float y = voxel->pos[1];
		float z = voxel->pos[2];

		fprintf(fp, "v %.02f %.02f %.02f\n", nu + x, nu + y, pu + z);
		fprintf(fp, "v %.02f %.02f %.02f\n", pu + x, nu + y, pu + z);
		fprintf(fp, "v %.02f %.02f %.02f\n", nu + x, pu + y, pu + z);
		fprintf(fp, "v %.02f %.02f %.02f\n", pu + x, pu + y, pu + z);
		fprintf(fp, "v %.02f %.02f %.02f\n", pu + x, nu + y, nu + z);
		fprintf(fp, "v %.02f %.02f %.02f\n", nu + x, nu + y, nu + z);
		fprintf(fp, "v %.02f %.02f %.02f\n", pu + x, pu + y, nu + z);
		fprintf(fp, "v %.02f %.02f %.02f\n", nu + x, pu + y, nu + z);
	}

	fprintf(fp, "\n");

	memset(colors_index, 0xFF, sizeof(colors_index));
	colors_count = 0;

	for(i=0; i<model->count; i++)
	{
		nvx_voxel *voxel = &model->voxels[i];

		unsigned char r = voxel->col[0];
		unsigned char g = voxel->col[1];
		unsigned char b = voxel->col[2];

		short index_index;
		short color_index;

		index_index = nvx_rgb_hash(r, g, b) & NVX_COLORS_INDEX_MASK;
		color_index = colors_index[index_index];
		if(color_index == -1)
		{
			color_index = colors_count++;
			colors_index[index_index] = color_index;

			colors[color_index][0] = r;
			colors[color_index][1] = g;
			colors[color_index][2] = b;
		}

		j = i << 3;

		fprintf(fp, "\nusemtl color_%d\n\n", color_index);

		fprintf(fp, "f %d %d %d\n", j + 1, j + 2, j + 3);
		fprintf(fp, "f %d %d %d\n", j + 2, j + 4, j + 3);
		fprintf(fp, "f %d %d %d\n", j + 5, j + 6, j + 7);
		fprintf(fp, "f %d %d %d\n", j + 6, j + 8, j + 7);
		fprintf(fp, "f %d %d %d\n", j + 3, j + 6, j + 1);
		fprintf(fp, "f %d %d %d\n", j + 3, j + 8, j + 6);
		fprintf(fp, "f %d %d %d\n", j + 7, j + 2, j + 5);
		fprintf(fp, "f %d %d %d\n", j + 7, j + 4, j + 2);
		fprintf(fp, "f %d %d %d\n", j + 3, j + 7, j + 8);
		fprintf(fp, "f %d %d %d\n", j + 3, j + 4, j + 7);
		fprintf(fp, "f %d %d %d\n", j + 2, j + 6, j + 5);
		fprintf(fp, "f %d %d %d\n", j + 2, j + 1, j + 6);
	}

	fclose(fp);

	return nvx_model_write_obj_mat_file(colors, colors_count, material_filename);
}

static int nvx_model_write_file(const nvx_model *model, const char *filename)
{
	int i, len;
	const char *extension;
	
	len = strlen(filename) - 4;
	if(len <= 0)
	{
		nvx_set_error("Model %s doesn't have a valid extension.", filename);
		return -1;
	}
	extension = filename + len;

	for(i=0; i<_nvx_model_writers_count; i++)
	{
		nvx_model_writer *model_writer = &_nvx_model_writers[i];

		if(!strncmp(extension, model_writer->extension, 4))
			return model_writer->write(model, filename);
	}

	nvx_set_error("Model %s is unsupported.", filename);
	return -1;
}

nvx_model *nvx_model_create(const nvx_args *args)
{
	int i, j, count = 0;
	float x, y, z, xx, yy, zz, unit_size;
	unsigned char r, g, b;
	nvx_model *model;
	nvx_voxel *voxels;

	nvx_image *image = nvx_image_load_from_file(args->image_front);
	if(image == NULL)
		return NULL;

	voxels = nvx_alloc(sizeof(*voxels) * image->w * image->h * args->depth);
	if(voxels == NULL)
	{
		nvx_image_free(image);
		nvx_set_error("Failed to allocate enough memory for voxels.");
		return NULL;
	}

	unit_size = args->unit * 2.0;

	xx = (image->w / 2.0) * unit_size;
	yy = (image->h / 2.0) * unit_size;
	zz = (args->depth / 2.0) * unit_size;

	for(i=0; i<image->size; i+=image->bpp)
	{
		if(image->pixels[i + 3] == 0)
			continue;

		r = image->pixels[i + 2];
		g = image->pixels[i + 1];
		b = image->pixels[i + 0];

		x = (i >> 2) % image->w;
		y = (i >> 2) / image->w;
		
		x *= unit_size;
		y *= unit_size;

		x -= xx;
		y -= yy;

		for(j=0; j<args->depth; j++)
		{
			nvx_voxel *voxel = &voxels[count++];

			z = -j * unit_size + zz;

			nvx_vec3_set(voxel->pos, x, y, z);
			nvx_vec3_set(voxel->col, r, g, b);
		}
	}

	model = nvx_alloc(sizeof(*model));
	if(model == NULL)
	{
		nvx_free(voxels);
		nvx_set_error("Failed to allocate enough memory for model.");
		return NULL;
	}

	model->voxels = voxels;
	model->count = count;
	model->unit = args->unit;

	return model;
}

void nvx_model_free(nvx_model *model)
{
	if(model != NULL)
	{
		if(model->voxels != NULL)
			nvx_free(model->voxels);

		nvx_free(model);
	}
}

int nvx_convert_image_to_model(const nvx_args *args)
{
	int ret;
	nvx_model *model;

	model = nvx_model_create(args);
	if(model == NULL)
		return -1;

	ret = nvx_model_write_file(model, args->model_output);

	nvx_model_free(model);
	return ret;
}
#endif

#endif
