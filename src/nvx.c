#define NVX_IMPLEMENTATION
#include "nvx.h"

void print_help(const char *name);
int parse_args(nvx_args *args, const int argc, const char *argv[]);

int main(int argc, char *argv[])
{
	nvx_args args;
	memset(&args, 0x00, sizeof(args));

	if(parse_args(&args, argc, (const char **) argv) == -1)
	{
		print_help(argv[0]);
		return EXIT_FAILURE;
	}

	if(nvx_convert_image_to_model(&args) == -1)
	{
		printf("Error: %s\n", nvx_get_error());
		return EXIT_FAILURE;	 
	}

	return EXIT_SUCCESS;
}

void print_help(const char *name)
{
	printf("NVX %s, an image \"voxalizer\".\n", NVX_VERSION_STR);
	printf("Usage: %s [options]\n", name);
	printf("\nOptions:\n");
	printf("  -o\toutput OBJ model\t(required)\n");
	printf("  -f\tfront TGA image\t\t(required)\n");
#if 0
	printf("  -s\tside TGA image\t\t(optional)\n");
	printf("  -b\tback TGA image\t\t(optional)\n");
#endif
	printf("  -u\tunit\t\t\t(default: 1.0)\n");
	printf("  -d\tdepth in units\t\t(default: 1)\n");
	printf("  -h\tdisplay help\n");
}

int parse_args(nvx_args *args, const int argc, const char *argv[])
{
	const char *arg = NULL;
	const char *val = NULL;

	int i = 1;

	if(argc < 5 || (argc - 1) % 2 == 1)
		return -1;

	args->flags = 0;
	args->unit	= 1.0f;
	args->depth = 1.0f;

	while(i < argc)
	{
		if((arg = argv[i++])[0] != '-' || i == argc)
			return -1;

		if((val = argv[i++])[0] == '-')
			return -1;

		switch(arg[1])
		{
			case 'o':
			{
				strncpy(args->model_output, val, NVX_MAX_PATH);
				args->flags |= NVX_ARGS_MODEL_OUTPUT;
			}
			break;

			case 'f':
			{
				strncpy(args->image_front, val, NVX_MAX_PATH);
				args->flags |= NVX_ARGS_IMAGE_FRONT;
			}
			break;

#if 0
			case 's':
			{
				strncpy(args->image_side, val, NVX_MAX_PATH);
				args->flags |= NVX_ARGS_IMAGE_SIDE;
			}
			break;


			case 'b':
			{
				strncpy(args->image_back, val, NVX_MAX_PATH);
				args->flags |= NVX_ARGS_IMAGE_BACK;
			}
			break;
#endif

			case 'u':
			{
				float fval = atof(val);
				if(fval > 0.0)
				{
					args->unit = fval;
					args->flags |= NVX_ARGS_UNIT;
				}
			}
			break;

			case 'd':
			{
				int ival = atoi(val);
				if(ival > 0)
				{
					args->depth = ival;
					args->flags |= NVX_ARGS_DEPTH;
				}	 
			}
			break;

			default:
				return -1;
		}
	}

	if(!((args->flags & NVX_ARGS_MODEL_OUTPUT) && (args->flags & NVX_ARGS_IMAGE_FRONT)))
		return -1;

#if 0
	if((args->flags & NVX_ARGS_IMAGE_BACK) && !(args->flags & NVX_ARGS_IMAGE_SIDE))
		return -1;

	if(args->flags & NVX_ARGS_IMAGE_SIDE && args->flags & NVX_ARGS_DEPTH && args->depth > 1)
	{
		args->depth = 1;
		args->flags &= ~NVX_ARGS_DEPTH;
	}
#endif

	return 0;
}
