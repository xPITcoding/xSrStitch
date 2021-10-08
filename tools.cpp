#include "tools.h"
#include <stdlib.h>
#include "tif_config.h"
#include "tiff.h"
#include "tiffio.h"

#include <QImage>
#include <QMessageBox>
#include <QVector3D>

#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

using namespace std;

void myImageCleanupHandler(void *info)
{
    free(info);
}

void myTiffCleanupHandler(void *info)
{
    _TIFFfree(info);
}


QMatrix3x3 dyad (XYZ v1, XYZ v2)
{
    QMatrix3x3 diad;
    diad(0,0) = v1.x*v2.x;
    diad(1,0) = v1.y*v2.x;
    diad(2,0) = v1.z*v2.x;
    diad(0,1) = v1.x*v2.y;
    diad(0,2) = v1.x*v2.z;
    diad(2,0) = v1.z*v2.x;
    diad(2,1) = v1.z*v2.y;
    diad(1,2) = v1.y*v2.z;
    diad(2,2) = v1.z*v2.z;
    return diad;
}

complex_2d_array* makeFFT(QImage src)
{
    QVector <complex_1d_array> vec_fx;
    QVector <complex_1d_array> vec_fy;

    for (long ly=0;ly<src.height();++ly)
    {
        complex_1d_array fx;
        real_1d_array x;
        x.setlength(src.width());
        for (long lx=0;lx<src.width();++lx)
            x[lx] = ((unsigned short*)src.scanLine(ly))[lx];
        fftr1d(x, src.width(), fx);
        vec_fx.append(fx);
    }

    for (long lx=0;lx<src.width();++lx)
    {
        complex_1d_array x;
        xparams xparam;
        x.setlength(src.height());
        for (long ly=0;ly<src.height();++ly)
            x[ly] = vec_fx[ly][lx];
        fftc1d(x, src.height(), xparam);
        vec_fy.append(x);
    }

    complex_2d_array* buffer = new complex_2d_array;
    buffer->setlength(src.height(),src.width());
    for (long lx=0;lx<src.width();++lx)
        for (long ly=0;ly<src.height();++ly)
            (*buffer)[ly][lx]=vec_fy[ly][lx];

    return buffer;
}

real_2d_array* invFFT(complex_2d_array* data,const long& rows,const long& cols)
{
    real_2d_array* buffer = new real_2d_array;

    buffer->setlength(rows,cols);
    for (long c=0;c<cols;++c) {
        complex_1d_array column;
        column.setlength(rows);
        for (long r=0;r<rows;++r)
            column(r)=(*data)(r,c);
        fftc1dinv(column,rows);

        for (long r=0;r<rows;++r )
            (*data)(r,c)=column(r);
    }

    for (long r=0;r<rows;++r) {
        complex_1d_array row;
        row.setlength(cols);
        for (long c=0;c<cols;++c)
            row(c)=(*data)(r,c);
        fftc1dinv(row,cols);

        for (long c=0;c<cols;++c)
            (*data)(r,c)=row(c);
    }

    for (long r=0;r<rows;++r) {
        for (long c=0;c<cols;++c ) {
            (*buffer)(r,c)=abscomplex((*data)(r,c));
        }
    }

    return buffer;
}

QImage flipImg (QImage in)
{
    QImage out(in.width(),in.height(),QImage::Format_Grayscale16);
    out = in.mirrored(true,true);
    return out;
}

QImage createImg (real_2d_array *data, const long& width, const long& height, long& max_pos_x, long& max_pos_y, long& offsetx, long& offsety, double& maxVal, bool image)
{
    QImage img;

    unsigned short* buffer = (unsigned short*)malloc(width*height*sizeof(unsigned short));
    memset(buffer,0,width*height*sizeof(short));

    // get min max
    float _minGVal = (*data)(0,0);
    float _maxGVal = (*data)(0,0);

    max_pos_x = 0;
    max_pos_y = 0;

    long cx = (width-1) / 2;
    long cy = (height-1) / 2;


    for (long ly=0;ly<height;++ly)
    {
        for (long lx=0;lx<width;++lx)
        {
            _minGVal = std::min(_minGVal,(float)(*data)(ly,lx));

            if ((float)(*data)(ly,lx)> _maxGVal)
            {
                max_pos_x = lx;
                max_pos_y = ly;
                _maxGVal = (*data)(ly,lx);
            }
        }
    }

    if(image)
    {
        for (long ly=0;ly<height;++ly)
        {
            for (long lx=0;lx<width;++lx)
            {
                buffer[((lx+cx) % width) + ((ly+cy) % height)*width] = ((float)(*data)(ly,lx)-_minGVal)/(_maxGVal-_minGVal)*65535.0f;
            }
        }
        img=QImage((uchar*)buffer,width,height,width*2,QImage::Format_Grayscale16,myImageCleanupHandler,buffer);

    }

    max_pos_x>cx  ? offsetx = max_pos_x-width   : offsetx = max_pos_x;
    max_pos_y>cy ?  offsety = max_pos_y-height  : offsety = max_pos_y;

    offsetx+=1;
    offsety+=1;
    maxVal=_maxGVal;

    return img;
}

QImage polarImage(const QImage& src,const int& width,const int& height)

{
    unsigned short* buffer = (unsigned short*)malloc(width*height*sizeof(unsigned short));
    memset(buffer,0,width*height*sizeof(unsigned short));
    float rr = src.width();
    float cc = src.height();

    // generate a width x height grid and calculate the scaling factors
    float r = width;
    float c = height;
    float fac_r = rr/r;
    float fac_c = cc/c;
    // center
    float MR = r/2.0f;
    float MC = c/2.0f;
    float max_radius = std::min(MC*.9f,MR*.9f);
    float x1,x2,y1,y2,radius1,radius2,angle1,angle2;
    float total_weight;
    float total_sum_I;
    float weight;

    // calculate the value for each pixel in the polar grid
    for (long lr=50;lr<r;++lr) {
            for (long lc=0;lc<c;++lc) {
            // calculate the position of the pixel in the cartesian grid
            angle1 = 2.0f*3.14159f/(float)(c-1)*float(lc);
            radius1 = ((float)(lr)/(float)(r-1))*max_radius;

            x1 = (MC+cos(angle1)*radius1)*fac_c;
            y1 = (MR+sin(angle1)*radius1)*fac_r;

           // perform a simple linear interpolation
            total_weight = 0.0f;
            total_sum_I = 0.0f;
            // respect the size of the cartesian image
            if (x1>1 && x1<(cc-1) && y1>1 && y1<(rr-1))
            {
                for (long lly=floor(y1);lly<ceil(y1)+1;++lly)
                    for (long llx=floor(x1);llx<ceil(x1)+1;++llx)
                    {
                        weight = sqrt(pow(llx-x1,2)+ pow(lly-y1,2));
                        total_weight += 1.0f/weight;
                        total_sum_I += ((float)((unsigned short*)src.scanLine(lly))[llx])/weight;
                    }

                // assign the interpolated value to the polar grid
                if (total_weight>0)
                    buffer[(long)(lr+lc*r)]=total_sum_I/total_weight;
              }
        }
    }

    float _minVal=buffer[0];
    float _maxVal=buffer[0];

    for (long x=0;x<width;++x) {
        for (long y = 0; y < height; ++y)
        {
            //apply brightness gradient over x
            float factor=(float)x/(float)width;
            buffer[x+y*width]=(float)buffer[x+y*width]*factor;

            _minVal = std::min(_minVal,(float)buffer[x+y*width]);
            _maxVal = std::max(_maxVal,(float)buffer[x+y*width]);
        }
    }



    for (long x=0;x<width;++x)
    {

        for (long y=0; y<height; ++y)
        {

            buffer[x+y*width]=((float)buffer[x+y*width]-_minVal)/(_maxVal-_minVal)*65535.0f;
        }
    }

    QImage res((uchar*)buffer,width,height,width*2,QImage::Format_Grayscale16,myImageCleanupHandler,buffer);
    return res;
}

QImage Corr(QImage in1, QImage in2, long& mx, long& my, long& ofx, long& ofy, double& maxVal, bool image)
{
    int w=min(in1.width(),in2.width());
    int h=min(in1.height(),in2.height());
     w%2==0 ? w-- : w=w;
     h%2==0 ? h-- : h=h;
    in1=in1.copy(0,0,w,h);
    in2=in2.copy(0,0,w,h);



    complex_2d_array* fft1 =new complex_2d_array;
    fft1 = makeFFT(in1);
    QImage flip = flipImg(in2);

    complex_2d_array* fft2 =new complex_2d_array;
    fft2 = makeFFT(flip);

    for (int i=0; i<fft1->rows();i++)
        for (int j=0; j<fft1->cols();j++)
        {
            (*fft1)[i][j]*=(*fft2)[i][j];
        }
    real_2d_array *result;
    result = invFFT(fft1,fft1->rows(),fft1->cols());

    QImage out = createImg(result,fft1->cols(),fft1->rows(),mx,my, ofx, ofy, maxVal,image);
    delete(fft1);
    delete(fft2);
    delete(result);

    return out;
}

/*complex_2d_array* CorrCreateFFT(QImage in)
{
    int w=in.width();
    int h=in.height();
    w%2==1 ? w-- : w=w;
    h%2==1 ? h-- : h=h;
    in=in.copy(0,0,w,h);


    QImage flip = flipImg(in);
    complex_2d_array* fft2 =new complex_2d_array;
    fft2 = makeFFT(flip);
    return fft2;
}


QImage CorrFFTin(QImage in1, complex_2d_array* fft2, long& mx, long& my, long& ofx, long& ofy, double& maxVal)
{
    int w=in1.width();
    int h=in1.height();
    w%2==1 ? w-- : w=w;
    h%2==1 ? h-- : h=h;
    in1=in1.copy(0,0,w,h);

    complex_2d_array* fft1 =new complex_2d_array;
    fft1 = makeFFT(in1);

    for (int i=0; i<fft1->rows();i++)
        for (int j=0; j<fft1->cols();j++)
        {
            (*fft1)[i][j]*=(*fft2)[i][j];
        }
    real_2d_array *result;
    result = invFFT(fft1,fft1->rows(),fft1->cols());

    QImage out = createImg(result,fft1->cols(),fft1->rows(),mx,my, ofx, ofy, maxVal);


    delete(fft1);
    delete(result);


    return out;
}
*/

QImage prepareImageFM(QImage& img, float scale)
{
    int scaled=(float)img.width()*scale;
    QImage imgFM = img.copy(img.width()/2-scaled/2,img.height()/2-scaled/2,scaled,scaled);
    imgFM=imgFM.convertToFormat(QImage::Format_Grayscale8);
    QImage mask(":/imgs/FM_mask.png");
    mask=mask.copy(mask.width()/8,mask.height()/8,mask.width()-mask.width()/4,mask.height()-mask.height()/4);
    mask=mask.scaledToWidth(scaled);
    float  val=0;
    for(int x=0; x<mask.width();x++)
    {
        for(int y=0; y<mask.height();y++)
        {
           val = qGray(imgFM.pixel(x,y))* (qGray((float)mask.pixel(x,y))/255.0f);
           imgFM.setPixel(x,y,qRgb(val,val,val));
        }
    }
    imgFM=imgFM.convertToFormat(QImage::Format_Grayscale16);
    return imgFM;
}


QImage rotateImage(QImage input, float rot)
{
    float angleInRad=-rot/180.0f*M_PI;
    if (input.format()!=QImage::Format_Grayscale16)
        input=input.convertToFormat(QImage::Format_Grayscale16);
    QImage output(input.width(),input.height(),QImage::Format_Grayscale16);
    if (fabs(rot)>0.0001)
    {
        output.fill(65535);
        QMatrix2x2 rotationMatrix;
        rotationMatrix(0,0)=cos(angleInRad);
        rotationMatrix(0,1)=-sin(angleInRad);
        rotationMatrix(1,0)=sin(angleInRad);
        rotationMatrix(1,1)=cos(angleInRad);
        QVector3D point(0,0,1);
        float cx=output.width()/2.0f, cy=output.height()/2.0f;
        QVector3D center(cx,cy,0);
        float swx, swy, ewx,ewy, sumBuffer, sumWeight, weight;
        for(int x=0; x<output.width(); x++)
            for(int y=0; y<output.height(); y++)
            {
                point.setX(x-cx);
                point.setY(y-cy);
                point = matmul(rotationMatrix,point);
                point+=center;
                swx=max(0.0f,min((float)output.width(),floor(point.x())));
                swy=max(0.0f,min((float)output.height(),floor(point.y())));
                ewx=max(0.0f,min((float)output.width(),ceil(point.x())+1));
                ewy=max(0.0f,min((float)output.height(),ceil(point.y())+1));
                sumBuffer =0.0f;
                sumWeight =0.0f;
                for(int lx=swx; lx<ewx;lx++)
                    for(int ly=swy; ly<ewy;ly++)
                    {
                        weight = 1.0/(sqrt(pow(point.x()-lx,2)+pow(point.y()-ly,2))+0.1);
                        sumWeight+=weight;
                        sumBuffer+=weight*qGray(input.pixel(lx,ly));
                    }
                sumBuffer/=sumWeight;
                output.setPixel(x,y, qRgb(sumBuffer,sumBuffer,sumBuffer));
            }
        return output;
    }
    else
        return input;
}

QImage translateImage(QImage input, long ofx, long ofy)
{
    QImage output(input.width(),input.height(), QImage::Format_Grayscale16);
        output.fill(qRgb(0,0,0));
        for(int wi=0; wi<input.width();wi++)
            for(int hi=0; hi<input.height();hi++)
            {
               if((wi+ofx)>-1 && (wi+ofx)<input.width() && (hi+ofy)>-1 && (hi+ofy)<input.height())
                   output.setPixel(wi+ofx,hi+ofy,input.pixel(wi,hi));
            }
    return output;
}

inline QVector3D matmul(const QMatrix2x2& m,const QVector3D& p)
{
    QVector3D res;
    res.setX(m(0,0)*p[0]+m(0,1)*p[1]);
    res.setY(m(1,0)*p[0]+m(1,1)*p[1]);
    res.setZ(p[2]);
    return res;
}
/*
QImage makeFM (complex_2d_array* refPol, complex_2d_array* refOrg, QImage match, long& mx, long& my, long& ofx, long& ofy, float& rot)
{
    QImage matchOrg=match;
    match=prepareImageFM(match);
    QImage matchMask=match;
    double maxGV;
    QImage corrI2= Corr(match, match, mx, my, ofx, ofy,maxGV);
    QImage polarI2 = polarImage(corrI2,corrI2.width(),corrI2.height());  
    QImage polCorr = CorrFFTin(polarI2,refPol,mx, my, ofx, ofy,maxGV);
   // std::cout << mx << "  " << my << "  " << ofx << "  " << ofy << "  " << endl;
    rot= +(360.0f/((float)polCorr.height())*(float)ofy);
    if (fabs(rot)>180)
    {
        rot < 0 ? rot+=360 : rot-=360;
    }   
    //matchMask.save("C:/Users/jonas/Desktop/matchMask_prerot.png");
    matchMask= rotateImage(matchMask,rot);
    //matchMask.save("C:/Users/jonas/Desktop/matchMask_postrot.png");

    CorrFFTin(matchMask,refOrg,mx,my,ofx,ofy,maxGV);
    matchOrg=rotateImage(matchOrg,rot);
    //matchOrg.save("C:/Users/jonas/Desktop/matchorg_postrot.png");

    matchOrg=translateImage(matchOrg,ofx,ofy);
    //matchOrg.save("C:/Users/jonas/Desktop/matchorg_postrotposttrans.png");

    return matchOrg;
}*/

QImage makeFM (QImage refPol,QImage refOrg, QImage match, long& mx, long& my, long& ofx, long& ofy, float& rot, float scale)
{
    QImage matchOrg=match;
    match=prepareImageFM(match,scale);
    QImage matchMask=match;
    double maxGV;
    QImage corrI2= Corr(match, match, mx, my, ofx, ofy,maxGV,true);
    QImage polarI2 = polarImage(corrI2,corrI2.width(),corrI2.height());
    //refPol.save("C:/Users/jonas/Desktop/refPol.png");
    //polarI2.save("C:/Users/jonas/Desktop/I2Pol.png");
    Corr(refPol,polarI2,mx, my, ofx, ofy,maxGV,false);
    //polCorr.save("C:/Users/jonas/Desktop/polCorr.png");
    //std::cout << mx << "  " << my << "  " << ofx << "  " << ofy << "  " << endl;
    rot= +(360.0f/((float)polarI2.height()-1.0f)*(float)ofx);
    if (fabs(rot)>180)
    {
        rot < 0 ? rot+=360 : rot-=360;
    }
    rot=-rot;
    //std::cout << "rot  " << rot;
    if(fabs(rot)>0.001)
         matchMask= rotateImage(matchMask,rot);

    Corr(refOrg,matchMask,mx,my,ofx,ofy,maxGV,false);

    if(fabs(rot)>0.001)
        matchOrg=rotateImage(matchOrg,rot);

    if(ofx!=0 || ofy!=0)
        matchOrg=translateImage(matchOrg,ofy,ofx);

    return matchOrg;
}
FMDATA* makeFMmulti (FMDATA* _data)
{
    _data->I2result = makeFM(_data->I1Polar,_data->I1Masked,_data->I2,_data->mx,_data->my, _data->ofx, _data->ofy, _data->rot,_data->scale);
    _data->corrValue=correlate(_data->I1,_data->I2result);

    return _data;
}


long double correlate(QImage img1, QImage img2)
{
    long double mean1=0.0;
    long double mean2=0.0;

    for (int i=0; i<img1.width(); i++)
    {
        for(int j=0; j<img1.height(); j++)
        {
             mean1+= ((quint16*)img1.scanLine(j))[i];
             mean2+= ((quint16*)img2.scanLine(j))[i];
        }
    }
    mean1/=(long double)img1.width()*(long double)img1.height();
    mean2/=(long double)img2.width()*(long double)img2.height();


    long double zaehler=0.0;
    long double nenn1=0.0;
    long double nenn2=0.0;
    long double val1=0.0;
    long double val2=0.0;

    for (int i=0; i<img1.width(); i++)
    {
        for(int j=0; j<img1.height(); j++)
        {
            val1 = ((quint16*)img1.scanLine(j))[i];
            val2 = ((quint16*)img2.scanLine(j))[i];
            zaehler+= (val1-mean1)*(val2-mean2);
            nenn1+= (val1-mean1)*(val1-mean1);
            nenn2+= (val2-mean2)*(val2-mean2);
        }
    }

    long double result= zaehler/sqrt(nenn1*nenn2);

    return result;


}

void findMinMaxGlobal(QList<QFileInfoList> _List,double &_globalMin,double &_globalMax, datatype &dt)
{
   QStringList option;option << "TIF" << "TIFF" << "VOX";
   switch (option.indexOf(_List[0][0].suffix().toUpper())) {
   case 0:
   case 1:
       // mach
       {
           TiffInfo* first = new TiffInfo;
           first->fname=_List[0][0].absoluteFilePath();
           first=readTIFFtag(first);
           if(first->bit==32)
           {
               dt=TIFF32;
               QList<TiffInfo*> _TagList;
               for(int i=0; i<_List.count();i++)
                   for(int j=0; j<_List[i].count();j++)
                   {
                       TiffInfo* temp = new TiffInfo;
                       temp->fname=_List[i][j].absoluteFilePath();
                       _TagList.append(temp);
                   }



               QFutureWatcher<TiffInfo*> scryer;
               scryer.setFuture(QtConcurrent::mapped(_TagList,readTIFFtag));

               do
               {
                   QCoreApplication::processEvents();
               }
               while(scryer.isRunning());

               _globalMin = DBL_MAX;
               _globalMax = DBL_MIN;
               for (QList<TiffInfo*>::iterator it=_TagList.begin();it!=_TagList.end();++it)
               {
                   _globalMax=std::max(_globalMax,(*it)->_max);
                   _globalMin=std::min(_globalMin,(*it)->_min);
                   delete (*it);
               }
           }
           else
           {
                _globalMin=first->_min;
                _globalMax=first->_max;
                first->bit==8 ? dt=TIFF8 : dt=TIFF16;
           }
       }
       break;
   case 2:
       // VOX
       _globalMin=0;_globalMax=65535;
       dt=VOX;
       break;
   default:
      // Qt
       QImage first(_List[0][0].absoluteFilePath());
       if(first.format()== QImage::Format_Grayscale16)
       {
            _globalMin=0;
            _globalMax=65535;
       }
       else
       {
            _globalMin=0;
            _globalMax=255;
       }

       dt=QTIMAGE;
       break;
   }
}


TiffInfo* readTIFFtag(TiffInfo* input)
{
        char* _fileName = (char*)malloc(input->fname.length()+1);
        std::memset(_fileName,0,input->fname.length()+1);
        std::memcpy(_fileName,input->fname.toLocal8Bit().constData(),input->fname.length());
        TIFF *tif = TIFFOpen(_fileName,"r");
        if (tif)
        {
            uint32_t imageWidth, imageLength;
            uint16_t bitsPerPixel;
            uint32_t row;
            uint64_t offset;

            tdata_t buf;


            long sizeOfDatatype;

            TIFFGetField(tif,TIFFTAG_IMAGEWIDTH,&imageWidth);
            TIFFGetField(tif,TIFFTAG_IMAGELENGTH,&imageLength);
            TIFFGetField(tif,TIFFTAG_BITSPERSAMPLE,&bitsPerPixel);

            input->bit=bitsPerPixel;

            sizeOfDatatype=bitsPerPixel/8;

            switch(bitsPerPixel)
            {
            case 8:
            {
                input->_min = 0;
                input->_max = 255;
            }
                break;

            case 16:
            {
                input->_min = 0;
                input->_max = 65535;
            }
                break;

            case 32:
            {
                //32BIT
                unsigned char* pBuffer = (unsigned char*)_TIFFmalloc(imageWidth*imageLength*sizeOfDatatype);
                offset = (quint64)pBuffer;
                buf = _TIFFmalloc(TIFFScanlineSize(tif));
                for (row = 0; row < imageLength; row++)
                {
                    TIFFReadScanline(tif, buf, row);
                    memcpy((void*)offset,buf,imageWidth*sizeOfDatatype);
                    offset+=imageWidth*sizeOfDatatype;
                }
                TIFFClose(tif);
                _TIFFfree(buf);


                float _size = imageWidth*imageLength;
                float* pFBuffer=(float*)(pBuffer);

                for (long x=0;x<_size;++x)
                {
                    input->_min = std::min(input->_min,(double)pFBuffer[x]);
                    input->_max = std::max(input->_max,(double)pFBuffer[x]);
                }

                _TIFFfree(pBuffer);
            }

            break;

            default:
                break;
            }
         free(_fileName);
        }
         return input;

}

QImage OverlayImages(QImage I1, QImage I2)
{
    QImage Overlay(I1.width(),I2.height(),QImage::Format_RGB888);
    Overlay.fill(Qt::black);
    for (int i=0; i<Overlay.width(); i++)
    {
        for (int j=0; j<Overlay.height(); j++)
        {
            Overlay.setPixel(i,j, qRgb(I1.pixel(i,j),I2.pixel(i,j),I2.pixel(i,j)));
        }
    }
    return Overlay;
}

QImage TransformImage(QImage input, int transX, int transY, float angle)
{
    if(fabs(angle)>0.001)
        input=rotateImage(input,angle);
    if(transX!=0 || transY!=0)
        input=translateImage(input,transY,transX);
    return input;
}

QImage loadTIFFLibImage(const QString& fname, double _minGVal, double _maxGVal, double _bin)
{
    QImage img;
    char* _fileName = (char*)malloc(fname.length()+1);
    std::memset(_fileName,0,fname.length()+1);
    std::memcpy(_fileName,fname.toLocal8Bit().constData(),fname.length());
    TIFF* tif = TIFFOpen(_fileName,"r");
    if (tif)
    {
        uint32_t imageWidth, imageLength;
        uint16_t bitsPerPixel;
        uint32_t row;
        uint64_t offset;
        tdata_t buf;

        TIFFGetField(tif,TIFFTAG_IMAGEWIDTH,&imageWidth);
        TIFFGetField(tif,TIFFTAG_IMAGELENGTH,&imageLength);
        TIFFGetField(tif,TIFFTAG_BITSPERSAMPLE,&bitsPerPixel);

        switch(bitsPerPixel)
        {
        case 32:
            {
                float* pBuffer = (float*)_TIFFmalloc(imageWidth*imageLength*sizeof(float));
                offset = (uint64_t)pBuffer;
                buf = _TIFFmalloc(TIFFScanlineSize(tif));
                for (row = 0; row < imageLength; row++)
                {
                    TIFFReadScanline(tif, buf, row);
                    memcpy((void*)offset,buf,imageWidth*sizeof(float));
                    offset+=imageWidth*sizeof(float);
                }
                TIFFClose(tif);
                _TIFFfree(buf);

                unsigned long _size = imageWidth*imageLength;
                quint16* pU16Buffer = (quint16*)malloc(imageWidth*imageLength*2);
                if (_bin!=-1)
                {
                    for (unsigned long i=0;i<_size;++i)
                    {
                        pU16Buffer[i]=(pBuffer[i]>_bin)*65535.0f;
                    }
                }
                else
                {
                    float scale16 = 65535.0f/(_maxGVal-_minGVal);
                    for (unsigned long i=0;i<_size;++i)
                    {
                        pU16Buffer[i]=(pBuffer[i]-_minGVal)*scale16;
                    }

                }
                img = QImage((uchar*)pU16Buffer,imageWidth,imageLength,imageWidth*2,QImage::Format_Grayscale16,myImageCleanupHandler,pU16Buffer);
                _TIFFfree(pBuffer);
            }
            break;
        case 8:
            {
                char* pBuffer = (char*)_TIFFmalloc(imageWidth*imageLength);
                offset = (uint64_t)pBuffer;
                buf = _TIFFmalloc(TIFFScanlineSize(tif));
                for (row = 0; row < imageLength; row++)
                {
                    TIFFReadScanline(tif, buf, row);
                    memcpy((void*)offset,buf,imageWidth);
                    offset+=imageWidth;
                }
                TIFFClose(tif);
                _TIFFfree(buf);

                unsigned long _size = imageWidth*imageLength;
                quint16* pU16Buffer = (quint16*)malloc(imageWidth*imageLength*2);
                if (_bin!=-1)
                {
                    for (unsigned long i=0;i<_size;++i)
                    {
                        pU16Buffer[i]=(pBuffer[i]>_bin)*65535.0f;
                    }
                }
                else
                {
                    float scale16 = 65535.0f/(_maxGVal-_minGVal);
                    for (unsigned long i=0;i<_size;++i)
                    {
                        pU16Buffer[i]=(pBuffer[i]-_minGVal)*scale16;
                    }

                }
                img = QImage((uchar*)pU16Buffer,imageWidth,imageLength,imageWidth*2,QImage::Format_Grayscale16,myImageCleanupHandler,pU16Buffer);
                _TIFFfree(pBuffer);
            }
            break;
        case 16:
            {
                ushort* pBuffer = (ushort*)_TIFFmalloc(imageWidth*imageLength*sizeof(ushort));
                offset = (uint64_t)pBuffer;
                buf = _TIFFmalloc(TIFFScanlineSize(tif));
                for (row = 0; row < imageLength; row++)
                {
                    TIFFReadScanline(tif, buf, row);
                    memcpy((void*)offset,buf,imageWidth*sizeof(ushort));
                    offset+=imageWidth*sizeof(ushort);
                }
                TIFFClose(tif);
                _TIFFfree(buf);

                unsigned long _size = imageWidth*imageLength;

                if (_bin!=-1)
                {
                    for (unsigned long i=0;i<_size;++i)
                    {
                        pBuffer[i]=(pBuffer[i]>_bin)*65535.0f;
                    }
                }
                img = QImage((uchar*)pBuffer,imageWidth,imageLength,imageWidth*2,QImage::Format_Grayscale16,myTiffCleanupHandler,pBuffer);
            }
            break;
        default:
            break;
        }
    }

    return img;
}

void saveTIFF(QImage img, QString fname, float px)
{

    char* _fileName = (char*)malloc(fname.length()+1);
    std::memset(_fileName,0,fname.length()+1);
    std::memcpy(_fileName,fname.toLocal8Bit().constData(),fname.length());
    TIFF *tif = TIFFOpen(_fileName,"w");

    uint32_t imageWidth   = img.width();
    uint32_t imageLength  = img.height();
    uint16_t bitsPerPixel = 16;
    uint32_t row;



    if (tif)
    {
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH,imageWidth);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH,imageLength);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,bitsPerPixel);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL,1);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, (unsigned int) - 1));
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,1);
        if(px!=-1)
        {
            TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT,RESUNIT_CENTIMETER);
            TIFFSetField(tif, TIFFTAG_XRESOLUTION,10000.0f/px);
            TIFFSetField(tif, TIFFTAG_YRESOLUTION,10000.0f/px);
        }

        for (row = 0; row < imageLength; row++)
        {
            TIFFWriteScanline(tif, (void*)img.scanLine(row), row, 0);

        }
        TIFFClose(tif);
    }
}

void saveTransform(saveData* sd)
{
    //QImage img = loadTIFFLibImage(sd->fname,sd->_min,sd->_max);
    QImage img = loadEverything(sd->type,sd->fname,sd->_min,sd->_max,-1,sd->vInfo,sd->sliceNr,sd->reversed);
    if(sd->info.transX!=0 || sd->info.transY!=0 || sd->info.angle !=0)
    {
        img=TransformImage(img,sd->info.transX,sd->info.transY,sd->info.angle);
    }
    saveTIFF(img,sd->dir+ "/" +QString("%1").arg(sd->number,4,10,QLatin1Char('0'))+".tif",sd->pxSize);
    //return sd;
}

QImage loadEverything(datatype dt, QString fname, double _min, double _max, double _bin, VOXInfo* vInfo, int sliceNumber,bool reversed)
{
    QImage img;
    switch(dt)
    {
        case TIFF8:
        case TIFF16:
        case TIFF32:
        {
        img=loadTIFFLibImage(fname,_min,_max,_bin);
        }
    break;
        case VOX:
        {            
            if (reversed)
                sliceNumber=vInfo->volSize.z()-sliceNumber;
            img=loadVOX(fname,*vInfo,sliceNumber,_bin);
        }
        //load VOX
    break;
        case QTIMAGE:
        {
        img.load(fname);
        if(_bin!=-1)
            img=binarize16(img,_bin);

        }
        break;
    }
    return img;

}

VOXInfo* readVOXheader(QString fname)
{
    VOXInfo* vInfo=new VOXInfo;
    QFile f(fname);
    if (f.open(QFile::ReadOnly))
    {
        QDataStream d(&f);
        QTextStream t(d.device());
         t.seek(0);

        // the pattern defines the relevant tags that are evaluated
        const char* pattern[]={"VolumeCount","Endian","VolumeSize","VolumePosition","VolumeScale","Field","ModelMatrix","##"};
        QString s;
        QString command;
        float _bits;
        quint16 blkCount=0;

        int befNr=-1;
        // we always assume INT16 bit for the moment
        do
        {
            befNr=-1;
            s=t.readLine();
            if (s.contains("##")) ++blkCount;
            command=s.section(" ",0,0);
            s=s.right(s.length()-command.length()-1);

            for (int i=0;i<8;++i)
                if (command.compare(pattern[i])==0)
                    befNr=i;

            switch (befNr)
            {
            case 0 : // VolumeCount, only one object per file is supported
                if (s.toInt()!=1)
                   QMessageBox::warning(0,"Warning","multiple objects found ... only the 1st will be loaded");
                break;
            case 1 : // Endian
                s.toLower()=="l" ? vInfo->endian="little" : vInfo->endian="big";
                break;
            case 2 : // VolumeSize

                vInfo->volSize.setX(s.section(" ",0,0).toFloat());
                vInfo->volSize.setY(s.section(" ",1,1).toFloat());
                vInfo->volSize.setZ(s.section(" ",2,2).toFloat());
                break;
            case 3 : // VolumePosition .. not Used in the moment
                break;
            case 4 : // VolumeScale
                vInfo->voxelSize=s.section(" ",0,0).toFloat();
                break;
            case 5 : // Field
                {
                QString l = s.right(s.length()-s.indexOf("Size ")).section(" ",1,1);
                _bits = l.toFloat();

                QString f = s.right(s.length()-s.indexOf("Format ")).section(" ",1,1);
                if (f=="ui")
                    _bits==16 ? vInfo->sourceType="ushort" : vInfo->sourceType="uchar";
                else
                    _bits==16 ? vInfo->sourceType="short" : vInfo->sourceType="char";
                }
                break;
            case 6 : // ModelMatrix not Used by now
                s=s.mid(1,s.length()-2);
                break;
            case 7 :
                break;
            }
        }
        while (!t.atEnd() && blkCount<3);
   }
    return vInfo;
}

QImage loadVOX(QString fname, VOXInfo vInfo, int sliceNumber, double bin)
{
    QImage res;
    sliceNumber--;
    long dimx = vInfo.volSize.x();
    long dimy = vInfo.volSize.y();
    long dimz = vInfo.volSize.z();
    long size=dimx*dimy;
    long bytes = vInfo.sourceType.contains("short") ? 2 : 1;
    const char* pattern[]={"char","uchar","short","ushort"};
    int typeId=-1;
    for (int i=0;i<4;++i)
        if (vInfo.sourceType.toLower()==pattern[i])
            typeId=i;
    char* buffer = (char*)malloc(dimx*dimy*bytes);
    QFile f(fname);
    if (f.open(QFile::ReadOnly))
    {
        long fsize=f.size();
        QDataStream d(&f);
        d.setByteOrder(vInfo.endian.contains("little") ? QDataStream::LittleEndian : QDataStream::BigEndian);
        d.device()->seek(fsize-dimx*dimy*bytes*(dimz-sliceNumber));
        if (bytes==1)
        {
            d.readRawData(buffer,dimx*dimy);
            QImage img((uchar*)buffer,dimx,dimy,dimx,QImage::Format_Grayscale8,myImageCleanupHandler,buffer);
            res=img.convertToFormat(QImage::Format_Grayscale16);
        }
        else
        {
            ushort *pSBuffer=(ushort*)buffer;
            for (long i=0;i<size;++i)
            {
                d >> pSBuffer[i];
                if(bin!=-1)
                {
                    pSBuffer[i]>bin ? pSBuffer[i]=65535 : pSBuffer[i]=0;
                }

            }
            res = QImage((uchar*)buffer,dimx,dimy,dimx*2,QImage::Format_Grayscale16,myImageCleanupHandler,buffer);
        }
        f.close();

    }
    return res;

}

QImage binarize16 (QImage img, double bin)
{
    ushort* buffer=(ushort*)malloc(img.width()*img.height()*sizeof(ushort));
    for(int x=0; x<img.width(); x++)
    {
        for(int y=0; y<img.height(); y++)
        {
            ((quint16*)img.scanLine(y))[x]>bin ? buffer[x+y*img.width()]=65535 : buffer[x+y*img.width()]=0;
        }

    }
    QImage out((uchar*)buffer,img.width(),img.height(),img.width()*2,QImage::Format_Grayscale16,myImageCleanupHandler,buffer);
    return out;
}
