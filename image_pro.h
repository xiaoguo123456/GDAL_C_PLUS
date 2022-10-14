#pragma once
#include "gdal_priv.h"
#include <iostream>
using namespace std;

/**
* 获取图像信息
*/
void GetRasterInformation(const char* file);
bool ImageProcess(const char* pszSrcFile, const char* pszDstFile, const char* pszFormat);