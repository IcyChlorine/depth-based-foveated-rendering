#include "MainApplication.h"


// 这就是真正的main函数. 连接时会被lib文件链接为main函数——因此 函数名称不能改
int sample_main(int argc, const char** argv)
{
	MainApplication theApp;
	return theApp.run(
		PROJECT_NAME,
		argc, argv,
#ifdef __VR_PART
		SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT,
		//VR_SIZE_WIDTH, VR_SIZE_HEIGHT,
#else
		SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT,
#endif
		SAMPLE_MAJOR_VERSION, SAMPLE_MINOR_VERSION
	);
}

// DO NOT REMOVE! 否则会出现【无法解析的外部符号】。
void sample_print(int level, const char* fmt)
{
	
}