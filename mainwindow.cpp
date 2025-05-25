
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "clickablelabel.h"
#include "imagewindow.h"
#include"QFileDialog"
#include"QMessageBox"
#include"QTimer"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , maskWindow(nullptr)
    , resultWindow(nullptr)
{
    ui->setupUi(this);



    afficherMessage(this, "Prêt à extraire votre schéma électronique !",
                    "Info", QMessageBox::Information, 1000);

    ui->SaveMaskButton->setEnabled(false);
    ui->SavecontoursButton->setEnabled(false);



    // Affichage des valeurs initiales des sliders
    ui->ksize_value->setText(QString::number(ui->sliderKsize->value()));
    ui->low_value->setText(QString::number(ui->sliderLowThreshold->value()));
    ui->high_value->setText(QString::number(ui->sliderHighThreshold->value()));

    // connecter la bouton de chargement de l'image avec sa fonction loadImageFromFile
    connect(ui->SelectedImage, &QPushButton::clicked, this, &MainWindow::loadImageFromFile);

    // Mise à jour des textes des sliders
    connect(ui->sliderKsize, &QSlider::valueChanged, this, &MainWindow::updateSliderValue1);
    connect(ui->sliderLowThreshold, &QSlider::valueChanged, this, &MainWindow::updateSliderValue2);
    connect(ui->sliderHighThreshold, &QSlider::valueChanged, this, &MainWindow::updateSliderValue3);

    // connecter labelimage (comme une bouton) pour afficher
    //l'image originale dans une nouvelle fentre
    connect(ui->labelImage, &ClickableLabel::clicked, this, [=]() {
        ImageWindow *imgfen = new ImageWindow(this);
        if (!image.empty()) {
            imgfen->showRawImage(image);  // image originale (cv::Mat)
            imgfen->setWindowTitle("Image originale");
            imgfen->show();
        }
    });
    // connecter labelImage_Mask (comme une bouton) pour afficher
    //l'image avec masque dans une nouvelle fentre

    connect(ui->labelImage_Mask, &ClickableLabel::clicked, this, [=]() {
        if (maskWindow) {
            cv::Mat gray = maskWindow->getPreprocessedGray(); // récupère l'image de masque
            if (!gray.empty()) {
                ImageWindow *imgfen1 = new ImageWindow(this);
                imgfen1->setWindowTitle("Image avec masque");
                imgfen1->showRawImage(gray);
                imgfen1->show();
            }
        }
    });
    // connecter labelImage_contours (comme une bouton) pour afficher
    //l'image avec contours dans une nouvelle fentre

    connect(ui->labelImage_contours, &ClickableLabel::clicked, this, [=]() {
        if (resultWindow) {
            resultWindow->setWindowTitle("Image avec contours");
            resultWindow->show();
        }
    });

    // Affichage du masque dans mainwindow
    connect(ui->grayButton, &QPushButton::clicked, this, [=]() {
        if (maskWindow) delete maskWindow;
        maskWindow = new ImageWindow(this);
        maskWindow->setWindowTitle("Résultat du masque");
        maskWindow->setmaskImage(image);

        cv::Mat gray = maskWindow->getPreprocessedGray();
        if (!gray.empty()) {
            QImage imgGray(gray.data, gray.cols, gray.rows, gray.step,
                           QImage::Format_Grayscale8);
            ui->labelImage_Mask->setPixmap(QPixmap::fromImage(imgGray).scaled(ui->labelImage_Mask->size(),
                                                                              Qt::KeepAspectRatio,
                                                                              Qt::SmoothTransformation));

            afficherMessage(this, "Masque affiché!",
                            "Info", QMessageBox::Information, 1000);
            ui->SaveMaskButton->setEnabled(true);

        }

    });

    // Affichage du résultat du traitement dans mainwindow
    connect(ui->TraitementButton, &QPushButton::clicked, this, [=]() {
        if (resultWindow) delete resultWindow;

        resultWindow = new ImageWindow(this);
        resultWindow->setWindowTitle("Résultat du traitement");
        resultWindow->setOriginalImage(image);

        resultWindow->setKsize(ui->sliderKsize->value());
        resultWindow->setLowThreshold(ui->sliderLowThreshold->value());
        resultWindow->setHighThreshold(ui->sliderHighThreshold->value());

        // Connexion sliders -> mise à jour dynamique des résultats
        connect(ui->sliderKsize, &QSlider::valueChanged, this, &MainWindow::updateContoursView);
        connect(ui->sliderLowThreshold, &QSlider::valueChanged, this, &MainWindow::updateContoursView);
        connect(ui->sliderHighThreshold, &QSlider::valueChanged, this, &MainWindow::updateContoursView);

        updateContoursView(); //Mise à jour le résultat au changement des valeurs de paramètres avec l'affichage

        afficherMessage(this, "Contours détectés et affichés!",
                        "Info", QMessageBox::Information, 1000);
        ui->SavecontoursButton->setEnabled(true);


    });
    //connecter les boutons pour enregitrer le résultat
    connect(ui->SaveMaskButton, &QPushButton::clicked, this, [=]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Enregistrer l'image", "", "Images (*.png *.jpg)");
        if (!fileName.isEmpty() && maskWindow) {
            cv::Mat result = maskWindow->getPreprocessedGray();
            if (!result.empty()) {
                cv::imwrite(fileName.toStdString(), result);

                afficherMessage(this, "votre image est enregistrée avec succès !",
                                "Info", QMessageBox::Information, 1000);
            }
        }
    });
    //Enregistrer le résultat de contours
    connect(ui->SavecontoursButton, &QPushButton::clicked, this, [=]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Enregistrer l'image", "", "Images (*.png *.jpg)");
        if (!fileName.isEmpty() && resultWindow) {
            cv::Mat result = resultWindow->getContoursImage();
            if (!result.empty()) {
                cv::imwrite(fileName.toStdString(), result);
                afficherMessage(this, "votre image est enregistrée avec succès !",
                                "Info", QMessageBox::Information, 1000);            }
        }
    });



    //afficher une image en background
    QPixmap bkgnd("C:/Users/HP/Desktop/Stage pfe/PCB_PROJECT/background.png");
    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    this->setAutoFillBackground(true);

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(bkgnd));
    this->setPalette(palette);





}

MainWindow::~MainWindow() {
    delete ui;
}

// Fonction pour charger l'image à traiter
void MainWindow::loadImageFromFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Choisir une image", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!fileName.isEmpty()) {
        image = cv::imread(fileName.toStdString());
        if (!image.empty()) {
            QImage imgQ(image.data, image.cols, image.rows, image.step, QImage::Format_BGR888);
            ui->labelImage->setPixmap(QPixmap::fromImage(imgQ).scaled(ui->labelImage->size(),
                                                                      Qt::KeepAspectRatio,
                                                                      Qt::SmoothTransformation));

            afficherMessage(this, "Image chargée avec succès!",
                                      "Info", QMessageBox::Information, 2000   );//2secondes

        } else {
            QMessageBox::warning(this, "Erreur", "Impossible de charger l'image !");

        }
    }
}

// Mise à jour des labels de sliders
//1-KernelSize
void MainWindow::updateSliderValue1(int value) {
    ui->ksize_value->setText(QString::number(value));
}
//2-seuil bas
void MainWindow::updateSliderValue2(int value) {
    ui->low_value->setText(QString::number(value));
}
//2-seuil haut
void MainWindow::updateSliderValue3(int value) {
    ui->high_value->setText(QString::number(value));
}

// Mise à jour dynamique le résultat de contours avec sliders
void MainWindow::updateContoursView() {

    if (!resultWindow) return;

    resultWindow->setKsize(ui->sliderKsize->value());
    resultWindow->setLowThreshold(ui->sliderLowThreshold->value());
    resultWindow->setHighThreshold(ui->sliderHighThreshold->value());
    resultWindow->updateImage();

    cv::Mat contours = resultWindow->getContoursImage();
    if (!contours.empty()) {
        QImage imgContours(contours.data, contours.cols, contours.rows, contours.step, QImage::Format_BGR888);
        //affucher l'image avec contours dans mainwindow
        ui->labelImage_contours->setPixmap(QPixmap::fromImage(imgContours).scaled(
            ui->labelImage_contours->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    }
}

// Fonction pour afficher une image en arrière-plan : elle est appelée automatiquement
//grâce au système d'événements de Qt lors du redimensionnement de la fenêtre.

void MainWindow::resizeEvent(QResizeEvent *event) {
    QPixmap bkgnd("C:/Users/HP/Desktop/Stage pfe/PCB_PROJECT/background.png");
    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPalette palette;
    palette.setBrush(QPalette::Window, bkgnd);
    this->setPalette(palette);
    QMainWindow::resizeEvent(event);

}
//Fonction pour afficher des messages
void MainWindow::afficherMessage(QWidget *parent, const QString &texte,
                     const QString &titre, QMessageBox::Icon style, int duree) {
    QMessageBox *message = new QMessageBox(parent);
    message->setWindowTitle(titre);
    message->setText(texte);
    message->setIcon(style);
    message->setStandardButtons(QMessageBox::NoButton);
    message->setStyleSheet("QMessageBox { color: black; }");
    message->show();

    // Fermer automatiquement après la durée, et le supprimer ensuite
    QTimer::singleShot(duree, message, SLOT(close()));
    QTimer::singleShot(duree + 100, message, SLOT(deleteLater()));

}

