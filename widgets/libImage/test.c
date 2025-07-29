#include "image.h"
#include <libcc.h>

#define MAIN_BIN 0

_CC_FORCE_INLINE_ uint32_t _gray(uint32_t pixel) {
	byte_t r,g,b,a,gray;
	a = colorR(pixel);
	r = colorR(pixel);
	g = colorG(pixel);
	b = colorB(pixel);
	//gray = (byte_t)((r * 77 + 151 * g + 28 * b) >> 8);
    gray = (r + ((uint16_t)g << 1) + b) >> 2;
	return _CC_A8R8G8B8(a, gray, gray, gray);
}

void _cc_image_to_gray(_cc_image_t *src) {
    uint32_t x = 0, y = 0;
    uint32_t pixel;
    for (y = 0; y < src->height; ++y) {
        for (x = 0; x < src->width; ++x) {
            pixel = _cc_get_pixel(src, x, y);
            _cc_set_pixel(src, x, y, _gray(pixel), 0);
        }
    }
}

void _cc_image_to_reverse(_cc_image_t *src) {
    uint32_t x = 0, y = 0;
    uint32_t col;
    _cc_rgba_t pixel;
    for (y = 0; y < src->height; ++y) {
        for (x = 0; x < src->width; ++x) {
            col = _gray(_cc_get_pixel(src, x, y));
            pixel.r = 255 - colorR(col);
            pixel.g = 255 - colorG(col);
            pixel.b = 255 - colorB(col);
            pixel.a = colorA(col);
            _cc_set_pixel(src, x, y, _CC_A8R8G8B8(pixel.a, pixel.r, pixel.g, pixel.b), 0);
        }
    }
}

/**
 * This is a point that will break the space into Black or white
 * In real words, if the distance between WHITE and BLACK is D;
 * then we should be this percent far from WHITE to be in the black region.
 * Example: If this value is 0.5, the space is equally split.
 */
bool_t should_be_black(uint32_t pixel, double SPACE_BREAKING_POINT) {
    byte_t alpha = colorA(pixel);
    byte_t redValue = colorR(pixel);
    byte_t blueValue = colorB(pixel);
    byte_t greenValue = colorG(pixel);
	double distanceFromWhite,distanceFromBlack,distance;

    //if this pixel is transparent let me use TRANSPARENT_IS_BLACK
    if (alpha == 0x00)
        return true;
    // distance from the white extreme
    distanceFromWhite = sqrt(pow((float)0xff - redValue, 2) + pow((float)0xff - blueValue, 2) + pow((float)0xff - greenValue, 2));
    // distance from the black extreme //this should not be computed and might be as well a function of distanceFromWhite and the whole distance
    distanceFromBlack = sqrt(pow((float)-redValue, 2) + pow((float)-blueValue, 2) + pow((float)-greenValue, 2));
    // distance between the extremes //this is a constant that should not be computed :p
    distance = distanceFromBlack + distanceFromWhite;
    // distance between the extremes
    //printf("%f:%f:%f\n", distanceFromWhite, distance, SPACE_BREAKING_POINT);
    return ((distanceFromWhite / distance) > SPACE_BREAKING_POINT);
}

void _cc_image_to_binary(_cc_image_t *src, double SPACE_BREAKING_POINT) {
    uint32_t x = 0, y = 0;
    uint32_t pixel;
    for (y = 0; y < src->height; ++y) {
        for (x = 0; x < src->width; ++x) {
            pixel = _cc_get_pixel(src, x, y);
            if (should_be_black(_gray(pixel), SPACE_BREAKING_POINT))
                _cc_set_pixel(src, x, y, 0xFF000000, 0);
            else
                _cc_set_pixel(src, x, y, 0xffffffff, 0);
        }
    }
}

#if MAIN_BIN
void to(const char* src, const char* dest) {
     _cc_image_t *img = _cc_image_from_file(src);
     if (img) {
		//printf("src:%s\ndst:%s\nSPACE_BREAKING_POINT:%f\n",src,dest,SPACE_BREAKING_POINT);
        _cc_image_to_gray(img);
        if (!_cc_write_PNG(dest, img)) {
            printf("write PNG fail.\n");
        }
        _cc_free_image(img);
     }
}

const tchar_t *imageExtension[] = {
    _T(".jpg"),
    _T(".jpeg"),
    _T(".png"),
    _T(".bmp"),
    _T(".tga"),
    _T(".pcx")
};

static bool_t is_filler(tchar_t *name) {
    int32_t i = 0;
    tchar_t *p;

    p = _tcsrchr(name, _T('.'));
    if (p == nullptr) {
        return true;
    }
    
    for (i = 0; i < _cc_countof(imageExtension); i++) {
        if (_tcsicmp(imageExtension[i], p) == 0) {
            return false;
        }
    }

    return true;
}

void finder(const tchar_t* source_path, const tchar_t* save_path) {
    tchar_t fpath[_CC_MAX_PATH_];
    tchar_t spath[_CC_MAX_PATH_];
    DIR* dir;
    struct dirent* d;

    dir = opendir(source_path);
    if (dir == nullptr) {
        return;
    }

    while ((d = readdir(dir)) != nullptr) {
        //
        if (d->d_type == DT_DIR &&
            ((d->d_name[0] == '.' && d->d_name[1] == 0) ||
             (d->d_name[0] == '.' && d->d_name[1] == '.' &&
              d->d_name[2] == 0))) {
            continue;
        }

         if (is_filler(d->d_name))
            continue;

        _sntprintf(fpath, _cc_countof(fpath), _T("%s/%s"), source_path, d->d_name);
        _sntprintf(spath, _cc_countof(spath), _T("%s/%s"), save_path, d->d_name);

        if (d->d_type == DT_DIR) {
            finder(fpath, spath);
        } else {
            to(fpath, spath);
        }
    }
    closedir(dir);
}

/*Print the usage message.*/
static int print_usage(void) {
    return fprintf(stdout,
                   _T("Usage:bitmap -R,-r[File or dir]...-o[File or dir]\n"));
}

const char *get_argv(int argc, char* const argv[], int *point) {
    int i = *point;
    if (argv[i][2] == 0) {
        if ((i + 1) < argc) {
            *point = i + 1;
            return argv[i + 1];
        }
    }
    return &argv[i][2];
}

int main(int argc, char* const argv[]) {
    int i;
    const char* src = nullptr;
    const char* dest = nullptr;
    const char* point = nullptr;
    int r = 0;
    double SPACE_BREAKING_POINT = 0;

    if (argc <= 3) {
        print_usage();
        return 0;
    }

    for (i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case 'm':
                case 'M':
                    point = get_argv(argc, argv, &i);
                    break;
                case 'f':
                case 'F':
                    src = get_argv(argc, argv, &i);
                    r = 0;
                    break;
                case 'r':
                case 'R':
                    r = 1;
                    src = get_argv(argc, argv, &i);
                    break;
                case 'o':
                case 'O':
                    dest = get_argv(argc, argv, &i);
                    break;
            }
        }
    }

    if (point != nullptr) {
        SPACE_BREAKING_POINT = atof(point) / 30.0f;
    }

    if (src == nullptr || dest == nullptr) {
        print_usage();
        return 1;
    }

    if (r) {
		printf("src:%s\ndst:%s\n",src,dest);
        finder(src, dest);
    } else {
        to(src, dest);
    }

    return 0;
}
#endif