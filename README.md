题目要求：
	1、编译出libTiff, libJPEG, zLib库
	2、用libTiff和libJpeg库写一个读写TileTiff的程序，可以进行Tile的JPEG压缩和解压缩

编译环境：
	1. 编译器Visual Studio 2022  
	2. CMake 3.23

问题1：
1. 编译libTiff库，使用git克隆https://github.com/vadz/libtiff的源码进行使用cmake编译.
2. 编译libJPEG库，使用git克隆https://github.com/LuaDist/libjpeg，成功编译出动态库文件。
   中途链接出现了一点问题，本次demo项目采用的是https://download.csdn.net/download/haiyangyunbao813/14004480编译好的静态文件。
3. 编译zLib库 使用git克隆https://github.com/madler/zlib 

编译均是采用cmake编译 参照github上面的README文件。

问题2.
1. 这里是采用本人微信头像图片作为示例图片，使用PS另存为.tif的文件类型（见2023.tif）
2. 使用vs2022 中打开项目文件夹->CMakeYZL(项目) .VS 会自动识别项目中的CMakeLists.txt文件
3. 采用debug的编译模式直接运行项目即可（release还有点库链接问题，出于时间考虑 暂不解决）
4. 项目说明：
1）目录结构
~~~Tree
	<>
	│
	├─3dparty
	│  ├─libjpeg
	│  │  ├─include
	│  │  └─lib
	│  ├─libtiff
	│  │  ├─include
	│  │  └─lib
	│  │      ├─debug
	│  │      └─release
	│  └─zlib
	│      ├─bin
	│      │  ├─debug
	│      │  └─release
	│      └─include
	├─build
	├─out
	└─src
  	  └─DemoYZL
~~~

源码文件在CMakeYZL.cpp中
~~~C++
// TIF文件结构体
struct TiffFile {
    std::vector<uint8_t> data;  // 存储TIF文件的二进制数据
    uint32_t width;             // 图像宽度
    uint32_t height;            // 图像高度
    uint16_t bitsPerPixel;      // 每个像素的位数
    uint16_t samplesPerPixel;   // 每个像素的样本数

    // 构造函数
    TiffFile(uint32_t w, uint32_t h, uint16_t bpp, uint16_t spp)
        : width(w), height(h), bitsPerPixel(bpp), samplesPerPixel(spp) {}
};

// 读取TIF文件信息并保存到结构体
bool ReadTiffFile(const std::string& filename, TiffFile& tiff) 
// 转换为 Tile 格式的 TIFF 文件
bool ConvertToTileTiff(const std::string& inputFilename, const std::string& outputFilename, TiffFile& tiff, int tileWidth, int tileHeight)
// 对TiffFile进行JPEG压缩
bool CompressTiffFile(const TiffFile& tiff, const std::string& outputFilename, int compressionQuality)
// 解压缩
bool DecompressJPEG(const std::string& inputFilename, const std::string& outputFilename)
~~~




