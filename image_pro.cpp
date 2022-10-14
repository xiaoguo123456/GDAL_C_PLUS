#include "image_pro.h"

void GetRasterInformation(const char* file)
{
    //注册文件格式
    GDALAllRegister();

    //使用只读方式打开图像
    GDALDataset* poDataset = (GDALDataset*)GDALOpen(file, GA_ReadOnly);
    if (poDataset == NULL)
    { 
        cout << "File:" << "E:\\DataSet\\test.tif" << "不能打开！" << endl;
        return;
    }

    //输出图像格式信息
    cout << "Driver: " << poDataset->GetDriver()->GetDescription()
        << "/" << poDataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME)
        << endl;

    //输出图像的大小和波段个数
    cout << "Size is " << poDataset->GetRasterXSize()
        << "x" << poDataset->GetRasterYSize()
        << "x" << poDataset->GetRasterCount()
        << endl;

    //输出图像的投影信息
    if (poDataset->GetProjectionRef() != NULL)
    {
        cout << "Projection is " << poDataset->GetProjectionRef() << endl;
    }

    //输出图像的坐标和分辨率信息
    double adfGeoTransform[6];
    if (poDataset->GetGeoTransform(adfGeoTransform) == CE_None)
    {
        printf("Origin = (%.6f,%.6f)\n", adfGeoTransform[0], adfGeoTransform[3]);
        printf("PixelSize = (%.6f,%.6f)\n", adfGeoTransform[1], adfGeoTransform[5]);
    }

    //读取第一个波段
    GDALRasterBand* poBand = poDataset->GetRasterBand(1);
    GDALDataType type = poBand->GetRasterDataType();//获取数据类型
    cout << "第一个波段的数据类型为：" << GDALGetDataTypeName(type) << endl;
    //获取该波段的最大值最小值，如果获取失败，则进行统计
    int bGotMin, bGotMax;
    double adfMinMax[2];
    adfMinMax[0] = poBand->GetMinimum(&bGotMin);
    adfMinMax[1] = poBand->GetMaximum(&bGotMax);

    if (!(bGotMin && bGotMax))
    {
        GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);
    }

    printf("Min = %.3fd , Max = %.3f\n", adfMinMax[0], adfMinMax[1]);

    //关闭文件
    GDALClose(poDataset);
}

bool ImageProcess(const char* pszSrcFile, const char* pszDstFile, const char* pszFormat)
{
    //注册GDAL驱动
    GDALAllRegister();

    //获取输出图像驱动
    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if (poDriver == NULL)   //输出文件格式错误
        return false;

    //打开输入图像
    GDALDataset* poSrcDS = (GDALDataset*)GDALOpen(pszSrcFile, GA_ReadOnly);
    if (poSrcDS == NULL)    //输入文件打开失败
        return false;

    //获取输入图像的宽高波段书
    int nXSize = poSrcDS->GetRasterXSize();
    int nYSize = poSrcDS->GetRasterYSize();
    int nBands = poSrcDS->GetRasterCount();

    //获取输入图像仿射变换参数
    double adfGeotransform[6] = { 0 };
    poSrcDS->GetGeoTransform(adfGeotransform);
    //获取输入图像空间参考
    const char* pszProj = poSrcDS->GetProjectionRef();

    GDALRasterBand* poBand = poSrcDS->GetRasterBand(1);
    if (poBand == NULL)    //获取输入文件中的波段失败
    {
        GDALClose((GDALDatasetH)poSrcDS);
        return false;
    }

    //创建输出图像，输出图像是1个波段
    GDALDataset* poDstDS = poDriver->Create(pszDstFile, nXSize, nYSize, 1, GDT_Byte, NULL);
    if (poDstDS == NULL)    //创建输出文件失败
    {
        GDALClose((GDALDatasetH)poSrcDS);
        return false;
    }

    //设置输出图像仿射变换参数，与原图一致
    poDstDS->SetGeoTransform(adfGeotransform);
    //设置输出图像空间参考，与原图一致
    poDstDS->SetProjection(pszProj);

    int nBlockSize = 256;     //分块大小

    //分配输入分块缓存
    unsigned char* pSrcData = new unsigned char[nBlockSize * nBlockSize * nBands];
    //分配输出分块缓存
    unsigned char* pDstData = new unsigned char[nBlockSize * nBlockSize];

    //定义读取输入图像波段顺序
    int* pBandMaps = new int[nBands];
    for (int b = 0; b < nBands; b++)
        pBandMaps[b] = b + 1;

    //循环分块并进行处理
    for (int i = 0; i < nYSize; i += nBlockSize)
    {
        for (int j = 0; j < nXSize; j += nBlockSize)
        {
            //定义两个变量来保存分块大小
            int nXBK = nBlockSize;
            int nYBK = nBlockSize;

            //如果最下面和最右边的块不够256，剩下多少读取多少
            if (i + nBlockSize > nYSize)     //最下面的剩余块
                nYBK = nYSize - i;
            if (j + nBlockSize > nXSize)     //最右侧的剩余块
                nXBK = nXSize - j;

            //读取原始图像块
            poSrcDS->RasterIO(GF_Read, j, i, nXBK, nYBK, pSrcData, nXBK, nYBK, GDT_Byte, nBands, pBandMaps, 0, 0, 0, NULL);

            //再这里填写你自己的处理算法
            //pSrcData 就是读取到的分块数据，存储顺序为，先行后列，最后波段
            //pDstData 就是处理后的二值图数据，存储顺序为先行后列

            memcpy(pDstData, pSrcData, sizeof(unsigned char) * nXBK * nYBK);
            //上面这句是一个测试，将原始图像的第一个波段数据复制到输出的图像里面

            //写到结果图像
            poDstDS->RasterIO(GF_Write, j, i, nXBK, nYBK, pDstData, nXBK, nYBK, GDT_Byte, 1, pBandMaps, 0, 0, 0, NULL);
        }
    }

    //释放申请的内存
    delete[]pSrcData;
    delete[]pDstData;
    delete[]pBandMaps;

    //关闭原始图像和结果图像
    GDALClose((GDALDatasetH)poSrcDS);
    GDALClose((GDALDatasetH)poDstDS);

    return true;
}