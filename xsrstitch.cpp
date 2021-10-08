#include "xsrstitch.h"
#include "ui_xsrstitch.h"


#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QImageReader>
#include <QToolTip>

#include "predialog.h"
#include "fmdialog.h"
#include "chartdialog.h"
#include "selectdirdialog.h"


xSrstitch::xSrstitch(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::xSrstitch)
{

    ui->setupUi(this);
    setFixedSize(850,525);

    QPixmap logo(":/imgs/logo.png");
    logo=logo.scaledToHeight(ui->logoLbl->height());
    ui->logoLbl->setPixmap(logo);

    // disable settings UI
    ui->pSpinBox_Ref->setEnabled(false);
    ui->pSpinBox_Match->setEnabled(false);
    ui->pSpinBox_Range->setEnabled(false);
    ui->pComboBox_selectSubFolder->setEnabled(false);
    ui->pButton_slicePreview->setEnabled(false);

    ui->pButton_Plot->setEnabled(false);
    ui->pButtonShowImgs->setEnabled(false);
    ui->pButton_Stitch->setEnabled(false);
    ui->pButton_Go->setEnabled(false);


    connect(ui->pSpinBox_voxelsize,SIGNAL(valueChanged(double)),this,SLOT(updateSettingsUi()));
    connect(ui->pSpinBox_stackOverlap,SIGNAL(valueChanged(double)),this,SLOT(updateSettingsUi()));
    connect(ui->pRadioLock,SIGNAL(clicked()),this,SLOT(lockUnlock()));
    connect(ui->pSpinBox_Match,SIGNAL(valueChanged(int)),this,SLOT(updateRange(int)));

    QPalette p(ui->pButton_Go->palette());
    pOFF=p;
    pON.setColor(QPalette::Active,QPalette::Button,Qt::green);
    pON.setColor(QPalette::Active,QPalette::ButtonText,Qt::black);

    enableToolTips();
}

xSrstitch::~xSrstitch()
{
    delete ui;
}

void xSrstitch::on_pButton_Close_clicked()
{
    close();
}

//-----------------------------------BUTTONS------------------------------------------------------------------------------------
void xSrstitch::on_pButton_InputDir_clicked()
{
    clearAll();

    bool success=loadDir();
    if(!success)
        return;

    selectDirDialog selDirDlg(&_dirList,ui->pLE_InputDir->text(),this);
    selDirDlg.exec();
    _dirList=*selDirDlg.getList();
    selDirDlg.checkReverse(reverse);

    ui->pTextBrowser->append("Folder contains "+ QString("%1").arg(_dirList.count()) +  " SubFolders");
    ui->pTextBrowser->append("");
    if(reverse)
         ui->pTextBrowser->append("stack reversed");

    if(!_dirList[0].isEmpty())
    {
        for(int i=0; i<_dirList.count(); i++)
        {
            ui->pTextBrowser->append("SubFolder "+ QString("%1").arg(i+1) +" contains: "+ QString("%1").arg(_dirList.at(i).size() )+ " files");
        }
        updateSettingsUi();

        for(int i=0; i<_dirList.count(); i++)
        {
            QString name(_dirList[i].at(0).absolutePath().remove(ui->pLE_InputDir->text()).section("/",1,1));          
            subStackNames << name;
            if(i<_dirList.count()-1)
              ui->pComboBox_selectSubFolder->addItem(name);
        }
        ui->pComboBox_selectSubFolder->setEnabled(true);
        ui->pButton_slicePreview->setEnabled(true);

        ui->pTextBrowser->append("........................................................................");
        ui->pTextBrowser->append("........................finding min & max...............................");
        setEnabled(false);
        findMinMaxGlobal(_dirList,_minGlobal,_maxGlobal, dt);
        setEnabled(true);
        ui->pTextBrowser->append("........................................................................");
        QStringList _dts;
        _dts <<"TIFF8" << "TIFF16" << "TIFF32" << "VOX"<< "QTIMAGE"<< "INVALID";
        ui->pTextBrowser->append("datatype detected:  "+_dts[dt]);
        ui->pTextBrowser->append("global min:  "+ QString("%1").arg(_minGlobal));
        ui->pTextBrowser->append("global max:  "+ QString("%1").arg(_maxGlobal));

        _binGlobal=(_maxGlobal-_minGlobal)/2.0;



        ui->pButton_slicePreview->setEnabled(true);
        ui->pButton_slicePreview->setPalette(pON);

    }
    else
        QMessageBox::warning(this,"Warning","Wrong folder structure detected, try again!");
}

void xSrstitch::on_pButton_OutputDir_clicked()
{
    QDir dir = QFileDialog::getExistingDirectory(this, "Select Image Folder","C:/C_WORK/stitchtest",QFileDialog::ShowDirsOnly);
    ui->pLE_OutputDir->setText(dir.absolutePath());

    if(ui->pButtonShowImgs->isEnabled())
    {
        ui->pButton_Stitch->setEnabled(true);
        ui->pButton_Stitch->setPalette(pON);
    }
}

void xSrstitch::on_pButton_slicePreview_clicked()
{    
    if(ui->pSpinBox_voxelsize->value()==0.0 && ui->pSpinBox_stackOverlap->value()==0.0 && ui->pSpinBox_Ref->value()==0 && ui->pSpinBox_Match->value()==0)
    {
        QMessageBox::warning(this,"Warning","fill stitching parameters");
    }
    else
    {
        int refmax=ui->pSpinBox_Ref->maximum();       
        int refval=ui->pSpinBox_Ref->value();
        int matchval=ui->pSpinBox_Ref->value();



        QImage prev1,prev2;
        if(dt==VOX)
        {
            QString fname1= _dirList[ui->pComboBox_selectSubFolder->currentIndex()].at(0).filePath();
            prev1=loadEverything(dt,fname1,_minGlobal,_maxGlobal,-1,vInfo,refmax-refval,reverse);

            QString fname2=_dirList[ui->pComboBox_selectSubFolder->currentIndex()+1].at(0).filePath();
            prev2=loadEverything(dt,fname2,_minGlobal,_maxGlobal,-1,vInfo,matchval,reverse);
        }
        else
        {
            QString fname1= _dirList[ui->pComboBox_selectSubFolder->currentIndex()].at(refmax-refval).filePath();
            prev1=loadEverything(dt,fname1,_minGlobal,_maxGlobal,-1);

            QString fname2=_dirList[ui->pComboBox_selectSubFolder->currentIndex()+1].at(matchval).filePath();
            prev2=loadEverything(dt,fname2,_minGlobal,_maxGlobal,-1);
        }

        preDialog dlg(&_dirList,_minGlobal,_maxGlobal,_binGlobal,dt,ui->pSpinBox_Ref->value(),ui->pSpinBox_Ref->maximum(),ui->pSpinBox_Match->value(),ui->pSpinBox_Match->maximum(),ui->pComboBox_selectSubFolder->currentIndex(),vInfo,reverse);
        dlg.exec();
        int refPos;
        int matchPos;
        dlg.getCurrentValues(_minGlobal,_maxGlobal,_binGlobal,refPos,matchPos,FMscale);
        ui->pSpinBox_Ref->setValue(refPos);
        ui->pSpinBox_Match->setValue(matchPos);
        ui->pTextBrowser->append("new global min:  "+ QString("%1").arg(_minGlobal));
        ui->pTextBrowser->append("new global max:  "+ QString("%1").arg(_maxGlobal));
        ui->pTextBrowser->append("binarization threshhold:  "+ QString("%1").arg(_binGlobal));

        ui->pButtonShowImgs->setPalette(pOFF);
        ui->pButton_Go->setEnabled(true);
        ui->pButton_Go->setPalette(pON);
    }
}


void xSrstitch::on_pButton_Go_clicked()
{
    _tData.clear();
    maxList.clear();
    showIMGs.clear();
    ui->pButton_Plot->setEnabled(false);
    ui->pButtonShowImgs->setEnabled(false);

    if(ui->pSpinBox_voxelsize->value()==0.0 && ui->pSpinBox_stackOverlap->value()==0.0 && ui->pSpinBox_Ref->value()==0 && ui->pSpinBox_Match->value()==0)
    {
        QMessageBox::warning(this,"Warning","fill stitching parameters");
    }
    else
    {
        ui->pButton_Go->setEnabled(false);
        maxList.clear();

        transformData temp;
        temp.startSlice=0;
        temp.endSlice=ui->pSpinBox_Ref->maximum()-ui->pSpinBox_Ref->value();
        temp.transX=0;
        temp.transY=0;
        temp.angle= 0.0f;
        _tData.append(temp);
        ui->pTextBrowser->append("\n");
        ui->pTextBrowser->append("........................................................................");
        for(int a=0; a< _dirList.count()-1; a++)
        {
            QImage img1;
            QString fname1;
            if(dt==VOX)
            {
                fname1= _dirList[a].at(0).filePath();

                img1=loadEverything(dt,fname1,_minGlobal,_maxGlobal,_binGlobal,vInfo,ui->pSpinBox_Ref->maximum()-ui->pSpinBox_Ref->value(),reverse);
            }
            else
            {
                fname1= _dirList[a].at(ui->pSpinBox_Ref->maximum()-ui->pSpinBox_Ref->value()).filePath();
                img1=loadEverything(dt,fname1,_minGlobal,_maxGlobal,_binGlobal);

            }
            ui->pTextBrowser->append("reference slice: "+ fname1);

            QImage org1=img1;       
            img1=prepareImageFM(img1,FMscale);
            QImage org1mask=img1;
            long mx1, my1, ofx1, ofy1;
            double maxGV;
            img1= Corr(img1, img1, mx1, my1, ofx1, ofy1,maxGV,true);
            QImage pol1= polarImage(img1,img1.width(),img1.height());
            QList<FMDATA*> _dataList;

            for (int i=0; i<ui->pSpinBox_Range->value()*2+1; i++)
            {

                QString fname2;
                if(dt==VOX)
                   fname2=_dirList[a+1].at(0).filePath();
                else
                   fname2=_dirList[a+1].at(ui->pSpinBox_Match->value()-ui->pSpinBox_Range->value()+i).filePath();

                QFileInfo test(fname2);
                if(test.exists())
                {
                    FMDATA* _data = new FMDATA;
                    if(dt==VOX)
                         _data->I2=loadEverything(dt,fname2,_minGlobal,_maxGlobal,_binGlobal,vInfo,ui->pSpinBox_Match->value()-ui->pSpinBox_Range->value()+i,reverse);
                    else
                         _data->I2=loadEverything(dt,fname2,_minGlobal,_maxGlobal,_binGlobal);
                    //_data->I2.save("C:/Users/jonas/Desktop/img2.png");
                    //_data->I1Masked=orgFFT;
                    //_data->I1Polar=polFFT;
                    _data->I1Masked=org1mask;
                    _data->I1Polar=pol1;
                    _data->I1=org1;
                    _data->scale=FMscale;
                    _dataList.append(_data);
                }
                else
                    ui->pTextBrowser->append("couldnt open file!  "+fname2);
            }
            if(_dataList.isEmpty())
                return;

            QFutureWatcher<FMDATA*> scryer;
            QObject::connect(&scryer,SIGNAL(progressRangeChanged(int,int)),ui->progressBar,SLOT(setRange(int,int)));
            QObject::connect(&scryer,SIGNAL(progressValueChanged(int)),ui->progressBar,SLOT(setValue(int)));
                scryer.setFuture(QtConcurrent::mapped(_dataList,makeFMmulti));

            do
            {
                QCoreApplication::processEvents();
            }
            while(scryer.isRunning());

            ui->progressBar->setValue(ui->progressBar->maximum());

            QPointF oldPoint(ui->pSpinBox_Match->value()-ui->pSpinBox_Range->value(),_dataList[0]->corrValue);
            QPointF newPoint;
            QVector<QPointF>* tempList=new QVector<QPointF>();
            int j=0;
            for (int i=0; i<_dataList.count(); i++)
            {
                newPoint.setX(ui->pSpinBox_Match->value()-ui->pSpinBox_Range->value() +i);
                newPoint.setY(_dataList[i]->corrValue);
                tempList->append(newPoint);               
                if (newPoint.y()>oldPoint.y())
                   {
                    oldPoint=newPoint;
                    j=i;
                   }
            }
            maxList.append(tempList);

            transformData temp;
            if (a==_dirList.count()-2)
            {
                temp.startSlice=oldPoint.x()+1;
                temp.endSlice=ui->pSpinBox_Match->maximum();
            }
            else
            {
                temp.startSlice=oldPoint.x()+1;
                temp.endSlice=ui->pSpinBox_Match->maximum()-ui->pSpinBox_Ref->value();
            }
            //  breakpoints.append(bp);
            temp.transX=_dataList[j]->ofx;
            temp.transY=_dataList[j]->ofy;
            temp.angle= _dataList[j]->rot;
            _tData.append(temp);
            if(oldPoint.x()==0 || oldPoint.x()==maxList.at(a)->count()-1)
                QMessageBox::warning(this,"Warning","Correlation maximum at slice "+QString("%1").arg(oldPoint.x())+ ". Please adjust matching subStack slice.");
            ui->pTextBrowser->append("Correlation maximum at slice:  "+ QString("%1").arg(oldPoint.x())+ "  val:  "+QString("%1").arg(oldPoint.y()));
            ui->pTextBrowser->append("rotation angle:  "+ QString("%1").arg(_dataList[j]->rot)+
                                    "   translation X:  "+ QString("%1").arg(_dataList[j]->ofx)+
                                    "   translation Y:  "+ QString("%1").arg(_dataList[j]->ofy));
            QList<QImage>* tempImgList =new QList<QImage>;
            tempImgList->append(_dataList[j]->I1);
            tempImgList->append(_dataList[j]->I2result);
            tempImgList->append(OverlayImages(_dataList[j]->I1,_dataList[j]->I2result));
            showIMGs.append(tempImgList);
            ui->pTextBrowser->append("Step  "+ QString("%1").arg(a+1)+"/"+QString("%1").arg(_dirList.count()-1)+"  done...");
            ui->pTextBrowser->append("........................................................................");
            _dataList.clear();
        }


        ui->pButton_Go->setPalette(pOFF);
        ui->pButton_Plot->setEnabled(true);
        ui->pButtonShowImgs->setEnabled(true);
        if(!ui->pLE_OutputDir->text().isEmpty())
        {
            ui->pButton_Stitch->setEnabled(true);
            ui->pButton_Stitch->setPalette(pON);
        }

        for(int b=0; b<_tData.count();b++)
        {
            ui->pTextBrowser->append("SubStack "+ QString("%1").arg(b+1)+" from: "+QString("%1").arg(_tData[b].startSlice)+" to: "+QString("%1").arg(_tData[b].endSlice));
        }
        ui->pTextBrowser->append("........................................................................");
        ui->pTextBrowser->append("......................... READY TO STITCH...............................");
        ui->pTextBrowser->append("........................................................................");        
        ui->pButton_Go->setEnabled(true);
    }

}


void xSrstitch::on_pButton_Plot_clicked()
{
    chartDialog plotDlg(&maxList,subStackNames);
    plotDlg.exec();
}


void xSrstitch::on_pButtonShowImgs_clicked()
{
    FMdialog FMdlg(&showIMGs,subStackNames);
    FMdlg.exec();
}

void xSrstitch::on_pButton_Stitch_clicked()
{
    CopyTransfer();
}

//---------------------------------FUNCTIONS-------------------------------------------------------------------------
bool xSrstitch::loadDir()
{   
    bool success=true;
    QList <QByteArray> _qtImageFormats=QImageReader::supportedImageFormats();
    QStringList _nameFilter;
    for (QList <QByteArray>::iterator it=_qtImageFormats.begin();it!=_qtImageFormats.end();++it)
        _nameFilter << QString("*.")+QString(*it);
    _nameFilter << "*.tif" << "*.tiff" << "*.VOX";


   QString dirName = QFileDialog::getExistingDirectory(this, "Select Image Folder","C:/C_WORK/stitchtest",QFileDialog::ShowDirsOnly);
   if (dirName.isEmpty()) return false;
   QDir dir(dirName);

   ui->pLE_InputDir->setText(dir.absolutePath());

   QFileInfoList DirEntries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);

   QList<QFileInfo> itList;
    bool VOXfound=false;
   for(int i=0; i<DirEntries.count();i++)
   {
        QDirIterator it(DirEntries.at(i).filePath(),QStringList(),QDir::AllEntries,QDirIterator::Subdirectories);

        while(it.hasNext())
        {
            it.next();
            if (it.fileInfo().isFile())
            {
                itList << it.fileInfo();
                VOXfound|=it.fileInfo().suffix().toUpper()=="VOX";
            }

        }
        _dirList.append(itList);
        itList.clear();
   }

   for(int i=0;i<_dirList.count();i++)
   {
       if(_dirList[0][0].suffix()!=_dirList[i][0].suffix())
       {
           QMessageBox::warning(this,"Warning!","wrong directory structure detected");
           _dirList.clear();
           return false;
       }
   }
   if(VOXfound)
   {
       for (int i=0;i<_dirList.count();i++)
       {
           for (int j=0;j<_dirList[i].count(); j++ )
           {
                if(_dirList[i][j].suffix().toUpper()!="VOX")
                {
                    _dirList[i].removeAt(j);
                    j--;
                }
           }
       }
       dt=VOX;
       vInfo=new VOXInfo;
       vInfo=readVOXheader(_dirList[0][0].filePath());
   }
   return success;


}

void xSrstitch::updateSettingsUi()
{
    if (!_dirList.isEmpty())
    {
        //ui->pSpinBox_Ref->setEnabled(true);
        ui->pSpinBox_Ref->setRange(0,_dirList.at(0).count()-1);

        float rangeinmm=(float)_dirList.at(0).count()*(float)ui->pSpinBox_voxelsize->value()/1000.0f;
        float vorschubinMM=(float)ui->pSpinBox_stackOverlap->value();
        float refSlice=(rangeinmm-vorschubinMM)/((float)ui->pSpinBox_voxelsize->value()/1000.0f)/2.0f;
        ui->pSpinBox_Ref->setValue(refSlice);


       //ui->pSpinBox_Match->setEnabled(true);
        ui->pSpinBox_Match->setRange(0,_dirList.at(1).count()-1);
        ui->pSpinBox_Match->setValue(refSlice);

        //ui->pSpinBox_Range->setEnabled(true);
        ui->pSpinBox_Range->setRange(ui->pSpinBox_Match->minimum(),ui->pSpinBox_Match->maximum());
        ui->pSpinBox_Range->setValue(refSlice);


        if(dt==VOX)
        {
            ui->pSpinBox_voxelsize->setValue(vInfo->voxelSize*1000.0f);

            ui->pSpinBox_Ref->setRange(0,vInfo->volSize.z()-1);
            float rangeinmm=(float)vInfo->volSize.z()*vInfo->voxelSize;
            float vorschubinMM=(float)ui->pSpinBox_stackOverlap->value();
            float refSlice=(rangeinmm-vorschubinMM)/(vInfo->voxelSize)/2.0f;
            ui->pSpinBox_Ref->setValue(refSlice);


           //ui->pSpinBox_Match->setEnabled(true);
            ui->pSpinBox_Match->setRange(0,vInfo->volSize.z()-1);
            ui->pSpinBox_Match->setValue(refSlice);

            //ui->pSpinBox_Range->setEnabled(true);

            ui->pSpinBox_Range->setValue(refSlice-1);


        }
    //ui->pTextBrowser->append(QString("%1").arg(ui->pSpinBox_Ref->maximum())+"  ref");
    //ui->pTextBrowser->append(QString("%1").arg(ui->pSpinBox_Match->maximum())+"  match");
    }
}


void xSrstitch::updateRange(int val)
{
    ui->pSpinBox_Range->setMaximum(val);
}

void xSrstitch::lockUnlock()
{
    if(ui->pRadioLock->isChecked())
    {
        ui->pSpinBox_Match->setEnabled(true);
        ui->pSpinBox_Ref->setEnabled(true);
        ui->pSpinBox_Range->setEnabled(true);
    }
    else
    {
        ui->pSpinBox_Match->setEnabled(false);
        ui->pSpinBox_Ref->setEnabled(false);
        ui->pSpinBox_Range->setEnabled(false);
    }
}

void xSrstitch::CopyTransfer()
{
    long count=0;
    long stackSize=0;


    for(int k=1; k<_tData.count();k++)
    {
        _tData[k].transX+=_tData[k-1].transX;
        _tData[k].transY+=_tData[k-1].transY;
        _tData[k].angle+=_tData[k-1].angle;        
    }

    QList<saveData*> _saveList;



    for (int i=0; i<_dirList.count(); i++)
    {
        dt==VOX ? stackSize=vInfo->volSize.z() : stackSize=_dirList[i].count();

        for (int j=0; j<stackSize;j++ )
        {
            if(j>=_tData[i].startSlice && j<=_tData[i].endSlice)
            {
                saveData* sD = new saveData;
                if(dt==VOX)
                {
                    sD->vInfo=vInfo;
                    sD->sliceNr=j;
                    sD->fname=_dirList[i].at(0).absoluteFilePath();
                }
                else
                {
                    sD->fname=_dirList[i].at(j).absoluteFilePath();
                }
                sD->type=dt;
                sD->info=_tData[i];
                sD->number=count;
                sD->_min=_minGlobal;
                sD->_max=_maxGlobal;
                sD->dir=ui->pLE_OutputDir->text();
                if(ui->pSpinBox_voxelsize!=0)
                    sD->pxSize=ui->pSpinBox_voxelsize->value();
                else
                    sD->pxSize=-1;

                _saveList.append(sD);
                count++;
            }
        }
    }


    QFutureWatcher<void> scryer;
    QObject::connect(&scryer,SIGNAL(progressRangeChanged(int,int)),ui->progressBar,SLOT(setRange(int,int)));
    QObject::connect(&scryer,SIGNAL(progressValueChanged(int)),ui->progressBar,SLOT(setValue(int)));

    QFuture<void> zukunft=QtConcurrent::map(_saveList,saveTransform);

        //scryer.setFuture(QtConcurrent::map(_saveList,saveTransform));
    scryer.setFuture(zukunft);

    do
    {
        QCoreApplication::processEvents();
    }
    while(scryer.isRunning());

    ui->progressBar->setValue(ui->progressBar->maximum());
    ui->pTextBrowser->append("....................................................................done");


}

void xSrstitch::clearAll()
{

    _maxGlobal=0.0;
    _minGlobal=0.0;
    _binGlobal=0.0;
    FMscale=0;
    _dirList.clear();
    _tData.clear();
    maxList.clear();
    showIMGs.clear();
    subStackNames.clear();

    dt=INVALID;
    vInfo=nullptr;
    reverse=false;

    ui->pComboBox_selectSubFolder->clear();
    ui->pTextBrowser->clear();
}

void xSrstitch::enableToolTips()
{
    QString input("Select folder that contains subStack folder. SubStack folders can contain subfolders.");
    ui->pButton_InputDir->setToolTip(input);
    ui->pL_InputDir->setToolTip(input);

    QString output("Select folder for save location.");
    ui->pButton_OutputDir->setToolTip(output);
    ui->pL_OutputDir->setToolTip(output);

    QString vox("Enter Voxelsize in micrometers.");
    ui->pL_voxelsize->setToolTip(vox);
    ui->pSpinBox_voxelsize->setToolTip(vox);

    QString overlap("Enter position of next subStack in millimeters.");
    ui->pL_stackOverlap->setToolTip(overlap);
    ui->pSpinBox_stackOverlap->setToolTip(overlap);

    QString ref("Slice number counted from the bottom of the reference subStack. Will serve as reference template.");
    ui->pL_Ref->setToolTip(ref);
    ui->pSpinBox_Ref->setToolTip(ref);

    QString match("Slice number counted from the top of the matching subStack. Set to slice which approximately represents the reference slice.");
    ui->pL_Match->setToolTip(match);
    ui->pSpinBox_Match->setToolTip(match);

    QString rang("Number of slices below and above match slice, which are checked to find the best fit.");
    ui->pL_Range->setToolTip(rang);
    ui->pSpinBox_Range->setToolTip(rang);

    QString preview("Select stack for preview, subsequent subStack is selected automatically.");
    ui->pL_selectSubFolder->setToolTip(preview);
    ui->pComboBox_selectSubFolder->setToolTip(preview);

    QString preB("Start slice preview!");
    ui->pButton_slicePreview->setToolTip(preB);

    QString plot("Plot correlation between reference and matching slices.");
    ui->pButton_Plot->setToolTip(plot);

    QString show("Display best matching slices.");
    ui->pButtonShowImgs->setToolTip(show);

    QString calc("Determing best matching position, rotation and translation for each subStack");
    ui->pButton_Go->setToolTip(calc);

    QString stitch("Stitch subStacks and save images as tif-files");
    ui->pButton_Stitch->setToolTip(stitch);





}
