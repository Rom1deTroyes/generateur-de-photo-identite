/*
Copyright (c) 2013 Clement Roblot


This file is part of Generateur de photo d'identite.

Generateur de photo d'identite is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Generateur de photo d'identite is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Generateur de photo d'identite.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "mainwindow.h"
#include "ui_mainwindow.h"



//TODO
//- documenter le code
//- faire un fichier readme qui roxx avec des images d'exemple
//- lorsqu'on dezoom beaucoup, on sort de l'image et on plante???


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    image = new ClicableQGraphicsView();
    image->keep_image_ratio(true);
    image->set_background_color(QColor(255, 255, 255));
    ui->gauche->addWidget(image);


    //on lit le fichier de description des visages
    QFile data(":/references/haarcascade_frontalface_alt.xml");
    data.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&data);

    //et on le colle dans un nouveau fichier teporaire
    QFile file(QDir::tempPath()+"/haarcascade_frontalface_alt.xml");
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);
    out << in.readAll();
    file.close();

    //on passe enfin le fichier à opencv
    QString fichier_visage = QDir::tempPath()+"/haarcascade_frontalface_alt.xml";
    if(!face_cascade.load(fichier_visage.toStdString())){

        qDebug("Erreur lors du chargement du fichier de caractéristique des visages.\n");
    }

    web = cvCreateCameraCapture(-1);

    timer = new QTimer(this);

    if(!web){

        cout << "erreur d'ouverture de la webcam" << endl;
    }else{
        connect(timer, SIGNAL(timeout()), this, SLOT(prendreImage()));
        timer->start(100);
    }
}

MainWindow::~MainWindow()
{

    QFile::remove(QDir::tempPath()+"/haarcascade_frontalface_alt.xml");
    delete ui;
}


void MainWindow::prendreImage(void){

    timer->stop();

    raw = cvQueryFrame(web);

    detectAndDisplay();

    timer->start();
}



Rect MainWindow::detectAndDisplay(void)
{
  std::vector<Rect> faces;
  Mat frame_gray;

  raw.copyTo(affichage);

  cvtColor( affichage, frame_gray, CV_BGR2GRAY );
  equalizeHist( frame_gray, frame_gray );

  //-- Detect faces
  face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0, Size(80, 80) );

  //-- Draw the face
  if(faces.size() == 1){

    //Add more heigth to include the haire
    //OFFSETHAUTEUR
    int heightToMoveUp = OFFSETHAUTEUR*faces[0].height/100.0;
    if(heightToMoveUp >= faces[0].y) heightToMoveUp = faces[0].y-1; //protect against going out of the picture

    faces[0].y -= heightToMoveUp;
    faces[0].height += heightToMoveUp;

    //Point center( faces[0].x + faces[0].width/2, faces[0].y + faces[0].height/2 );
    //ellipse( affichage, center, Size( faces[0].width/2, faces[0].height/2), 0, 0, 360, Scalar( 255, 0, 0 ), 2, 8, 0 );
    rectangle(affichage, faces[0], Scalar( 255, 0, 0 ), 5);
  }

  //-- Show what you got
  IplImage ipl_img = affichage;
  image->display(&ipl_img);

  if(faces.size() == 1){
    return faces[0];
  }else{
    Rect tmp;
    tmp.x = -1;
    tmp.y = -1;
    tmp.width = -1;
    tmp.height = -1;
    return tmp;
  }

}

void MainWindow::on_bouttonEnregistrer_clicked()
{
    timer->stop();

    Rect visage;
    visage = detectAndDisplay();

    if( (visage.x == -1) && (visage.y == -1) && (visage.width == -1) && (visage.height = -1) ){ //si on a pas trouvé de visage

        reca = new RecadragePhoto(raw, this);
        reca->show();
        connect(reca, SIGNAL(configFinie(Rect)), this, SLOT(recadrageFini(Rect)));

    }else{  //si on en a trouvé un

        composer(raw, visage);
    }
}



void MainWindow::recadrageFini(Rect visage){

    composer(raw, visage);
}

void MainWindow::composer(Mat image, Rect visage){

    timer->stop();

    fenetreConception = new ConceptionPlanche(image, visage, this);
    fenetreConception->show();
    connect(fenetreConception, SIGNAL(finished(int)), this, SLOT(rependreEnregistrement()));
}



void MainWindow::on_chargerImage_clicked()
{

    //Sinon on ouvre une fenètre de selection du fichier
    QStringList file;
    QFileDialog *dialog;
    dialog = new QFileDialog( this, QString::fromUtf8("Choisi un fichier image").toUtf8());
    dialog->setAcceptMode(QFileDialog::AcceptOpen);

    dialog->show();


    if(dialog->exec() == QDialog::Accepted){    //si on valide dans la fenètre de selection de fichier

        file = dialog->selectedFiles();

        timer->stop();

        raw = cvLoadImage(file.value(0).toUtf8());

        if( raw.data == NULL )
        {
          // Error, the file is not an image ?
        }
        else
        {
          Rect visage;
          visage = detectAndDisplay();

          composer(raw, visage);
        }
    }
}

void MainWindow::on_actionPrendre_une_image_triggered()
{
    on_bouttonEnregistrer_clicked();
}

void MainWindow::rependreEnregistrement(void){

    qDebug("retour à la fenetre principle");
    timer->start();
}

void MainWindow::on_actionCharger_une_image_triggered()
{
    on_chargerImage_clicked();
}
