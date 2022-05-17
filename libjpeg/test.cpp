#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif
#pragma warning(disable:4996)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <turbojpeg.h>
#include <iostream>
#include <time.h>
#include <io.h>

#ifdef _WIN32
#define strcasecmp  stricmp
#define strncasecmp  strnicmp
#endif

#define THROW(action, message) { \
  printf("ERROR in line %d while %s:\n%s\n", __LINE__, action, message); \
  retval = -1;  goto bailout; \
}

#define THROW_TJ(action)  THROW(action, tjGetErrorStr2(tjInstance))

#define THROW_UNIX(action)  THROW(action, strerror(errno))

#define DEFAULT_SUBSAMP  TJSAMP_444
#define DEFAULT_QUALITY  95


const char* subsampName[TJ_NUMSAMP] = {
  "4:4:4", "4:2:2", "4:2:0", "Grayscale", "4:4:0", "4:1:1"
};

const char* colorspaceName[TJ_NUMCS] = {
  "RGB", "YCbCr", "GRAY", "CMYK", "YCCK"
};

tjscalingfactor* scalingFactors = NULL;
int numScalingFactors = 0;

int compressJpeg(char* inImagePath, char* outImagePath, unsigned long size)
{
    clock_t start, end;
    tjscalingfactor scalingFactor = { 1, 1 };
    int outSubsamp = -1, outQual = -1;
    //tjtransform xform;
    int flags = 0;
    int width, height;
    char* inFormat, * outFormat;
    FILE* jpegFile = NULL;
    tjhandle tjInstance = NULL;
    int retval = 0, pixelFormat = TJPF_UNKNOWN;
    unsigned char* imgBuf = NULL, * jpegBuf = NULL;

    inFormat = strrchr(inImagePath, '.');
    outFormat = strrchr(outImagePath, '.');

    if (inFormat == NULL || outFormat == NULL || strlen(inFormat) < 2 || strlen(outFormat) < 2)
    {
        std::cout << "usage" << std::endl;
    }

    inFormat = &inFormat[1];
    outFormat = &outFormat[1];

    /* Input image is not a JPEG image.  Load it into memory. */
    if ((imgBuf = tjLoadImage(inImagePath, &width, 1, &height, &pixelFormat, 0)) == NULL)
        THROW_TJ("loading input image");
    if (outSubsamp < 0)
    {
        if (pixelFormat == TJPF_GRAY)
            outSubsamp = TJSAMP_GRAY;
        else
            outSubsamp = TJSAMP_444;
    }

    /*printf("Input Image:  %d x %d pixels\n", width, height);
    printf("Output Image (%s):  %d x %d pixels", outFormat, width, height);*/

    if (!strcmp(outFormat, "jpg"))
    {
        /* Output image format is JPEG.  Compress the uncompressed image. */
        unsigned long jpegSize = 0;

        jpegBuf = NULL;  /* Dynamically allocate the JPEG buffer */

        if (outQual < 0)
            outQual = DEFAULT_QUALITY;
        printf("%s subsampling, quality = %d\n", subsampName[outSubsamp],
            outQual);

        start = clock();

        if ((tjInstance = tjInitCompress()) == NULL)
            THROW_TJ("initializing compressor");
        if (tjCompress2(tjInstance, imgBuf, width, 0, height, pixelFormat, &jpegBuf, &jpegSize, outSubsamp, outQual, flags) < 0)
            THROW_TJ("compressing image");
        tjDestroy(tjInstance);  
        tjInstance = NULL;

        /* Write the JPEG image to disk. */
        if ((jpegFile = fopen(outImagePath, "wb")) == NULL)
            THROW_UNIX("opening output file");
        size_t outSize = fwrite(jpegBuf, jpegSize, 1, jpegFile);

        end = clock();

        std::cout << outImagePath << " jpegSize:" << jpegSize << std::endl;
        std::cout << "time: " << end - start << "ms" << " compression rate: " << (float)jpegSize / (float)size << std::endl;
        std::cout << "compression speed: " << (float)size / ((float)(end - start) * 1000) << "MB/s" << std::endl;

        if (outSize < 1)
            THROW_UNIX("writing output file");
        tjDestroy(tjInstance);  
        tjInstance = NULL;
        fclose(jpegFile);  
        jpegFile = NULL;
        tjFree(jpegBuf);  
        jpegBuf = NULL;
    }
    else
    {
        /* Output image format is not JPEG.  Save the uncompressed image
            directly to disk. */
            /*printf("\n");
            if (tjSaveImage(argv[2], imgBuf, width, 0, height, pixelFormat, 0) < 0)
                THROW_TJ("saving output image");*/
        std::cout << "error" << std::endl;
        return 0;
    }

    return 1;

    bailout:
        tjFree(imgBuf);
        if (tjInstance) tjDestroy(tjInstance);
        tjFree(jpegBuf);
        if (jpegFile) fclose(jpegFile);
        return retval;
}

int main(int argc, char** argv)
{
    char inImagePath[40] = "C:\\image\\bmp\\artificial\\*.bmp";
    char inFile[40] = "C:\\image\\bmp\\artificial\\";
    char outImagePath[40] = "C:\\image\\jpeg\\artificial\\";

    intptr_t handle;
    struct _finddata_t fileInfo;

    int i = 1;
    char num[10] = { 0 };
    itoa(i, num, 10);

    handle = _findfirst(inImagePath, &fileInfo);
    char outFileName[40];
    strcpy(outFileName, outImagePath);
    strcat(outFileName, num);
    strcat(outFileName, ".jpg");
    char inFilename[40] = { 0 };
    strcpy(inFilename, inFile);
    strcat(inFilename, fileInfo.name);
    std::cout << fileInfo.name << " bmpSize:" << fileInfo.size << std::endl;
    if (1 == compressJpeg(inFilename, outFileName, fileInfo.size)) {}

    if (-1 == handle)
    {
        printf("[error]");
        return 0;
    }
    while (0 == _findnext(handle, &fileInfo))
    {
        std::cout << fileInfo.name << " bmpSize:" << fileInfo.size << std::endl;
        memset(outFileName, 0, 20);
        memset(inFilename, 0, 20);
        char numTemp[10] = { 0 };
        i++;
        itoa(i, numTemp, 10);
        strcpy(outFileName, outImagePath);
        strcat(outFileName, numTemp);
        strcat(outFileName, ".jpg");
        strcpy(inFilename, inFile);
        strcat(inFilename, fileInfo.name);
        if (1 == compressJpeg(inFilename, outFileName, fileInfo.size)){}
        else
        {
            std::cout << "error" << std::endl;
        }
    }

    _findclose(handle);

    return 0;
}