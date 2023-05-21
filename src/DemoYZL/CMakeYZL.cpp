// CMakeYZL.cpp: 定义应用程序的入口点。
//

#include <vector>
#include <iostream>
#include <algorithm>
#include "tiffio.h"
#include "jpeglib.h"
#include "zconf.h"
#include "zlib.h"

using namespace std;

// TIF文件结构体
struct TiffFile {
    std::vector<uint8_t> data;  // 存储TIF文件的二进制数据
    uint32_t width;             // 图像宽度
    uint32_t height;            // 图像高度
    uint16_t tileWidth;
    uint16_t tileHeight;
    uint16_t bitsPerPixel;      // 每个像素的位数
    uint16_t samplesPerPixel;   // 每个像素的样本数

    // 构造函数
    TiffFile(uint32_t w, uint32_t h, uint16_t tw, uint16_t th, uint16_t bpp, uint16_t spp)
        : width(w), height(h), tileWidth(tw), tileHeight(th), bitsPerPixel(bpp), samplesPerPixel(spp) {
    }
};


// 判断是否为 Tile 类型的 TIFF 文件
bool IsTileTiff(const std::string& filename) {
    TIFF* tif = TIFFOpen(filename.c_str(), "r");
    if (!tif) {
        std::cout << "Failed to open TIFF file: " << filename << std::endl;
        return false;
    }

    uint32_t tileWidth = 0;
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileWidth);

    TIFFClose(tif);

    return (tileWidth != 0);
}

// 读取TIF文件信息并保存到结构体
bool ReadTiffFile(const std::string& filename, TiffFile& tiff) {
    // 打开TIF文件
    TIFF* tif = TIFFOpen(filename.c_str(), "r");
    if (!tif) {
        return false;
    }

    // 读取图像信息
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &tiff.width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &tiff.height);
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tiff.tileWidth);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &tiff.tileHeight);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &tiff.bitsPerPixel);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &tiff.samplesPerPixel);

    // 计算图像数据大小
    tsize_t imageSize = TIFFScanlineSize(tif) * tiff.height;
    tiff.data.resize(imageSize);

    // 逐行读取图像数据
    for (uint32_t row = 0; row < tiff.height; ++row)
    {
        TIFFReadScanline(tif, &tiff.data[row * TIFFScanlineSize(tif)], row);
    }

    // 关闭TIF文件
    TIFFClose(tif);
    return true;
}

// 转换为 Tile 格式的 TIFF 文件
bool ConvertToTileTiff(const std::string& inputFilename, const std::string& outputFilename, TiffFile& tiff, int tileWidth, int tileHeight)
{
    // 读取输入的 TIFF 文件
    if (!ReadTiffFile(inputFilename, tiff))
    {
        std::cout << "Failed to read input TIFF file." << std::endl;
        return false;
    }

    // 打开目标 TIFF 文件以写入模式
    TIFF* targetTiff = TIFFOpen(outputFilename.c_str(), "w");
    if (!targetTiff)
    {
        std::cout << "Failed to create target TIFF file." << std::endl;
        return false;
    }

    // 设置目标 TIFF 的基本属性
    TIFFSetField(targetTiff, TIFFTAG_IMAGEWIDTH, tiff.width);
    TIFFSetField(targetTiff, TIFFTAG_IMAGELENGTH, tiff.height);
    TIFFSetField(targetTiff, TIFFTAG_BITSPERSAMPLE, tiff.bitsPerPixel);
    TIFFSetField(targetTiff, TIFFTAG_SAMPLESPERPIXEL, tiff.samplesPerPixel);
    TIFFSetField(targetTiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(targetTiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(targetTiff, TIFFTAG_TILEWIDTH, tileWidth);
    TIFFSetField(targetTiff, TIFFTAG_TILELENGTH, tileHeight);

    // 计算 Tile 的行数和列数
    int tileRows = (tiff.height + tileHeight - 1) / tileHeight;
    int tileCols = (tiff.width + tileWidth - 1) / tileWidth;

    // 创建 Tile 缓冲区
    std::vector<uint8_t> tileBuffer(tileWidth * tileHeight * tiff.samplesPerPixel);

    // 循环写入 Tile 数据
    for (int row = 0; row < tileRows; row++)
    {
        for (int col = 0; col < tileCols; col++)
        {
            // 计算当前 Tile 的起始像素位置
            int startX = col * tileWidth;
            int startY = row * tileHeight;

            // 计算当前 Tile 的实际宽度和高度
            int width = std::min(tileWidth, static_cast<int>(tiff.width) - startX);
            int height = std::min(tileHeight, static_cast<int>(tiff.height) - startY);

            // 将源 TIFF 数据复制到 Tile 缓冲区
            for (int y = 0; y < height; y++)
            {
                const uint8_t* sourceRow = tiff.data.data() + ((startY + y) * tiff.width * tiff.samplesPerPixel);
                uint8_t* targetRow = tileBuffer.data() + (y * tileWidth * tiff.samplesPerPixel);
                std::memcpy(targetRow, sourceRow + (startX * tiff.samplesPerPixel), width * tiff.samplesPerPixel);
            }

            // 写入 Tile 数据到目标 TIFF 文件
            TIFFWriteTile(targetTiff, tileBuffer.data(), startX, startY, 0, 0);
        }
    }

    // 关闭目标 TIFF 文件
    TIFFClose(targetTiff);

    return true;
}

// 对TiffFile进行JPEG压缩
bool CompressTiffFile(const TiffFile& tiff, const std::string& outputFilename, int compressionQuality)
{
    // 创建输出的JPEG文件
    FILE* jpegFile = fopen(outputFilename.c_str(), "wb");
    if (!jpegFile)
    {
        std::cout << "Failed to create output JPEG file." << std::endl;
        return false;
    }

    // 初始化libjpeg压缩结构体和错误处理器
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, jpegFile);

    // 设置压缩参数
    cinfo.image_width = tiff.width;
    cinfo.image_height = tiff.height;
    cinfo.input_components = tiff.samplesPerPixel;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, compressionQuality, TRUE);

    // 开始压缩
    jpeg_start_compress(&cinfo, TRUE);

    // 创建行缓冲区
    JSAMPROW rowBuffer = new JSAMPLE[tiff.width * tiff.samplesPerPixel];

    // 每行的字节数
    const int bytesPerRow = tiff.width * tiff.samplesPerPixel;

    // 逐行读取 TIFF 图像数据并进行压缩
    for (int row = 0; row < tiff.height; ++row)
    {
        // 填充行缓冲区
        // 这里假设图像数据以连续的行存储在 tiff.data 中，每行数据的字节大小为 bytesPerRow
        std::memcpy(rowBuffer, tiff.data.data() + row * bytesPerRow, bytesPerRow);

        // 将行数据传递给 libjpeg 进行压缩
        jpeg_write_scanlines(&cinfo, &rowBuffer, 1);
    }
    // 完成压缩
    jpeg_finish_compress(&cinfo);
    // 清理资源
    delete[] rowBuffer;
    jpeg_destroy_compress(&cinfo);
    fclose(jpegFile);

    return true;
}

bool DecompressJPEG(const std::string& inputFilename, const std::string& outputFilename)
{
    // 打开输入的 JPEG 文件
    FILE* jpegFile = fopen(inputFilename.c_str(), "rb");
    if (!jpegFile)
    {
        std::cout << "Failed to open input JPEG file." << std::endl;
        return false;
    }

    // 初始化 libjpeg 解压缩结构体和错误处理器
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, jpegFile);

    // 读取 JPEG 文件头信息
    jpeg_read_header(&cinfo, TRUE);

    // 开始解压缩
    jpeg_start_decompress(&cinfo);

    // 获取图像的宽度和高度
    const int width = cinfo.output_width;
    const int height = cinfo.output_height;

    // 每行的字节数
    const int bytesPerRow = width * cinfo.output_components;

    // 创建行缓冲区
    JSAMPROW rowBuffer = new JSAMPLE[bytesPerRow];

    // 创建输出的 TIFF 文件
    TIFF* tif = TIFFOpen(outputFilename.c_str(), "w");
    if (!tif) {
        std::cout << "Failed to create output TIFF file." << std::endl;
        jpeg_destroy_decompress(&cinfo);
        fclose(jpegFile);
        delete[] rowBuffer;
        return false;
    }

    // 设置 TIFF 文件的基本信息
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, cinfo.output_components);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

    // 逐行解压缩 JPEG 并写入 TIFF
    while (cinfo.output_scanline < height)
    {
        // 从 JPEG 解压缩到行缓冲区
        jpeg_read_scanlines(&cinfo, &rowBuffer, 1);

        // 将行数据写入 TIFF
        TIFFWriteScanline(tif, rowBuffer, cinfo.output_scanline - 1, 0);
    }

    // 完成解压缩
    jpeg_finish_decompress(&cinfo);

    // 清理资源
    delete[] rowBuffer;
    jpeg_destroy_decompress(&cinfo);
    fclose(jpegFile);
    TIFFClose(tif);

    return true;
}

int main()
{
    const char* inputFileName = "2023.tif";
    const char* outputFileName = "outTile.tif";

    std::string compressedFilename = "compressed.tif";
    std::string decompressedFilename = "decompressed.tif";

    TiffFile tiff(0, 0, 0, 0,0,0);

    if (IsTileTiff(inputFileName))
    {
        if (ReadTiffFile(inputFileName, tiff))
        {
            std::cout << "Width: " << tiff.width << std::endl;
            std::cout << "Height: " << tiff.height << std::endl;
            std::cout << "Tile Width: " << tiff.tileWidth << std::endl;
            std::cout << "Tile Length: " << tiff.tileHeight << std::endl;
            std::cout << "Bits per Pixel: " << tiff.bitsPerPixel << std::endl;
            std::cout << "Samples per Pixel: " << tiff.samplesPerPixel << std::endl;
        }
    }
    else
    {
        int tileWidth = 256;
        int tileHeight = 256;
        if (ConvertToTileTiff(inputFileName, outputFileName, tiff, tileWidth, tileHeight))
        {
            std::cout << "Width: " << tiff.width << std::endl;
            std::cout << "Height: " << tiff.height << std::endl;
            std::cout << "Tile Width: " << tileWidth << std::endl;
            std::cout << "Tile Length: " << tileHeight << std::endl;
            std::cout << "Bits per Pixel: " << tiff.bitsPerPixel << std::endl;
            std::cout << "Samples per Pixel: " << tiff.samplesPerPixel << std::endl;
        }
    }

    // 对TIF文件进行压缩
    if (CompressTiffFile(tiff, compressedFilename, 100))
    {
        std::cout << "TIF file compressed successfully." << std::endl;
    }
    else
    {
        std::cout << "Failed to compress TIF file." << std::endl;
    }

    // 对TIF文件进行解压缩
    if (DecompressJPEG(compressedFilename, decompressedFilename))
    {
        std::cout << "TIF file DecompressJPEG successfully." << std::endl;
    }
    else
    {
        std::cout << "Failed to DecompressJPEG TIF file." << std::endl;
    }

    return 0;
}