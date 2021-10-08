#ifndef TOOLS_H
#define TOOLS_H


#include <QImage>
#include <QDialog>
#include <QMatrix3x3>
#include <QFileInfoList>
#include <QVector3D>

#include "linalg.h"
#include "fasttransforms.h"


using namespace alglib;

struct VOXInfo
{
    QString endian ="little";
    QVector3D volSize;
    float voxelSize;
    QString sourceType;
};

enum datatype
{
    TIFF8       =0x00,
    TIFF16      =0x01,
    TIFF32      =0x02,
    VOX         =0x03,
    QTIMAGE     =0x04,
    INVALID     =0x05
};

struct FMDATA
{
    QImage I1;
    //complex_2d_array* I1Polar;
    //complex_2d_array* I1Masked;
    QImage I1Polar;
    QImage I1Masked;
    QImage I2;
    QImage I2result;
    long mx;
    long my;
    long ofx;
    long ofy;
    float rot;
    long double corrValue;
    float scale;
};

struct TiffInfo
{
    QString fname;
    double _min=DBL_MAX;
    double _max=DBL_MIN;
    int bit;
};

struct transformData
{
    int startSlice;
    int endSlice;
    int transX;
    int transY;
    float angle;
};

struct saveData
{
    transformData info;
    QString fname;
    int number;
    double _min;
    double _max;
    QString dir;
    datatype type;
    bool reversed;
    int sliceNr;
    VOXInfo* vInfo;
    float pxSize;
};

struct XYZ
{
public:
    long x,y,z;
    XYZ(long xx,long yy, long zz){x=xx;y=yy;z=zz;}
};


QMatrix3x3 dyad (XYZ v1, XYZ v2);
complex_2d_array* makeFFT(QImage);
real_2d_array* invFFT(complex_2d_array* ,const long& ,const long&);
QImage flipImg (QImage);
QImage createImg (real_2d_array *, const long&, const long&, long&, long&, long&, long&, double&, bool);
QImage polarImage(const QImage&,const int&,const int&);
QImage Corr(QImage, QImage, long&, long&, long&, long&, double&, bool);
QImage CorrFFTin(QImage in1, complex_2d_array* fft2, long& mx, long& my, long& ofx, long& ofy, double& maxVal);
complex_2d_array* CorrCreateFFT(QImage in);

QImage prepareImageFM(QImage& img, float scale);
inline QVector3D matmul(const QMatrix2x2& m,const QVector3D& p);

QImage translateImage(QImage input, long ofx, long ofy);
QImage rotateImage(QImage input, float rot);

QImage makeFM (QImage refPol, QImage refOrg, QImage match, long& mx, long& my, long& ofx, long& ofy, float& rot);
FMDATA* makeFMmulti (FMDATA* _data);

long double correlate(QImage img1, QImage img2);

TiffInfo* readTIFFtag(TiffInfo* input);
void findMinMaxGlobal(QList<QFileInfoList> _List,double &_globalMin,double &_globalMax, datatype &dt);
QImage OverlayImages(QImage I1, QImage I2);

QImage TransformImage(QImage input, int transX, int transY, float angle);
void myImageCleanupHandler(void*);
void myTiffCleanupHandler(void *info);
QImage loadTIFFLibImage(const QString& fname, double _min, double _max, double _bin=-1);
void saveTIFF(QImage img, QString fname, float pxSize);
void saveTransform(saveData* sd);
QImage loadEverything(datatype dt, QString fname, double _min, double _max, double _bin, VOXInfo* vInfo=nullptr, int sliceNumber=-1,bool reversed=false);
VOXInfo* readVOXheader(QString fname);
QImage loadVOX(QString fname, VOXInfo vInfo, int sliceNumber, double bin=-1);
QImage binarize16 (QImage img, double bin);

#endif // TOOLS_H
