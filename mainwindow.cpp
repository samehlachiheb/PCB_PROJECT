// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "clickablelabel.h"
#include "imagewindow.h"
#include "QFileDialog"
#include "QMessageBox"
#include "QTimer"
#include <QLabel>
#include <QFrame>
#include <QAction>
#include <QKeySequence>
#include <QListWidget>
#include <QVBoxLayout>
#include <QSlider>
#include "composant.h" // Assurez-vous que Composant.h est bien inclus
#include <QDebug> // Pour les messages de débogage

// Définitions des placeholders (inchangées)
#ifndef PLACEHOLDER_ORIGINAL_IMAGE
#define PLACEHOLDER_ORIGINAL_IMAGE "Image Originale"
#endif
#ifndef PLACEHOLDER_MASK_IMAGE
#define PLACEHOLDER_MASK_IMAGE "Masque"
#endif
#ifndef PLACEHOLDER_CONTOURS_IMAGE
#define PLACEHOLDER_CONTOURS_IMAGE "Contours & Composants"
#endif
#ifndef PLACEHOLDER_RESULT_IMAGE
#define PLACEHOLDER_RESULT_IMAGE "Composants détectés"
#endif

// Constructeur de la classe MainWindow
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , maskWindow(nullptr)
    , resultWindow(nullptr)
    , m_componentListWidget(nullptr)
    , m_componentCountLabel(nullptr)
    , m_lastExtractedComponentsPixmap(QPixmap())
    , m_lastDetectedComponents(QList<Composant>())
    , m_displayFullResults(false) // Initialisé à false par défaut : les résultats complets ne s'affichent pas
{
    ui->setupUi(this);

    ui->labelImage_2->setToolTip("Click to open in new window");
    ui->labelImage_Mask_2->setToolTip("Click to open in new window");
    ui->labelImage_contours_2->setToolTip("Click to open in new window");
    if (ui->labelResult_2) {
        ui->labelResult_2->setToolTip("Composants extraits sur fond blanc");
    }

    QAction *SelectedImageAction = new QAction(tr("Select"), this);
    SelectedImageAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(SelectedImageAction, &QAction::triggered, this, &MainWindow::loadImageFromFile);
    this->addAction(SelectedImageAction);

    QLabel *originalImageDisplayLabel = ui->labelImage->findChild<QLabel*>("labelImage_2");
    if (originalImageDisplayLabel) {
        originalImageDisplayLabel->setText(PLACEHOLDER_ORIGINAL_IMAGE);
        originalImageDisplayLabel->setAlignment(Qt::AlignCenter);
    }

    QLabel *maskDisplayLabel = ui->labelImage_Mask->findChild<QLabel*>("labelImage_Mask_2");
    if (maskDisplayLabel) {
        maskDisplayLabel->setText(PLACEHOLDER_MASK_IMAGE);
        maskDisplayLabel->setAlignment(Qt::AlignCenter);
    }

    QLabel *contoursDisplayLabel = ui->labelImage_contours->findChild<QLabel*>("labelImage_contours_2");
    if (contoursDisplayLabel) {
        contoursDisplayLabel->setText(PLACEHOLDER_CONTOURS_IMAGE);
        contoursDisplayLabel->setAlignment(Qt::AlignCenter);
    }

    if (ui->labelResult_2) {
        ui->labelResult_2->setText("Composants Extraits");
        ui->labelResult_2->setAlignment(Qt::AlignCenter);
    }



    if (ui->listWidgetComponents) {
        ui->listWidgetComponents->setAlternatingRowColors(true);
        ui->listWidgetComponents->setViewMode(QListView::ListMode);
        ui->listWidgetComponents->setMovement(QListView::Static);
        ui->listWidgetComponents->setResizeMode(QListView::Adjust);
        ui->listWidgetComponents->setFlow(QListView::TopToBottom);
    } else {
        qWarning() << "QListWidget 'listWidgetComponents' non trouvé dans l'UI.";
    }

    ui->SelectedImage->setFixedSize(90, 90);
    ui->SelectedImage->setToolTip("Cliquez pour sélectionner une image");
    afficherMessage(this, "Prêt à extraire votre schéma électronique !",
                    "Info", QMessageBox::Information, 1000);

    connect(ui->SelectedImage, &QPushButton::clicked, this, &MainWindow::loadImageFromFile);

    // Connexions et initialisations pour les sliders
    // Chaque slider est connecté à son slot de mise à jour de valeur ET au slot de mise à jour des composants
    if (ui->sliderBlurKsize) {
        if (ui->value) { ui->value->setText(QString::number(ui->sliderBlurKsize->value())); }
        connect(ui->sliderBlurKsize, &QSlider::valueChanged, this, &MainWindow::updateSliderValue1);
        connect(ui->sliderBlurKsize, &QSlider::valueChanged, this, &MainWindow::updateComponentsView);
    }
    if (ui->sliderSigmaX) {
        if (ui->value_2) { ui->value_2->setText(QString::number(ui->sliderSigmaX->value())); }
        connect(ui->sliderSigmaX, &QSlider::valueChanged, this, &MainWindow::updateSliderValue2);
        connect(ui->sliderSigmaX, &QSlider::valueChanged, this, &MainWindow::updateComponentsView);
    }
    if (ui->sliderClaheClipLimit) {
        if (ui->value_3) { ui->value_3->setText(QString::number(ui->sliderClaheClipLimit->value())); }
        connect(ui->sliderClaheClipLimit, &QSlider::valueChanged, this, &MainWindow::updateSliderValue3);
        connect(ui->sliderClaheClipLimit, &QSlider::valueChanged, this, &MainWindow::updateComponentsView);
    }
    if (ui->sliderSeparationKsize) {
        if (ui->value_4) { ui->value_4->setText(QString::number(ui->sliderSeparationKsize->value())); }
        connect(ui->sliderSeparationKsize, &QSlider::valueChanged, this, &MainWindow::updateSliderValue4);
        connect(ui->sliderSeparationKsize, &QSlider::valueChanged, this, &MainWindow::updateComponentsView);
    }
    if (ui->sliderFillHolesKsize) {
        if (ui->value_5) { ui->value_5->setText(QString::number(ui->sliderFillHolesKsize->value())); }
        connect(ui->sliderFillHolesKsize, &QSlider::valueChanged, this, &MainWindow::updateSliderValue5);
        connect(ui->sliderFillHolesKsize, &QSlider::valueChanged, this, &MainWindow::updateComponentsView);
    }
    if (ui->sliderContourMinArea) {
        if (ui->value_6) { ui->value_6->setText(QString::number(ui->sliderContourMinArea->value())); }
        connect(ui->sliderContourMinArea, &QSlider::valueChanged, this, &MainWindow::updateSliderValue6);
        connect(ui->sliderContourMinArea, &QSlider::valueChanged, this, &MainWindow::updateComponentsView);
    }

    connect(ui->ClearButton,&QPushButton::clicked,this,&MainWindow::clearProcessedImageDisplays);

    // Connexions pour l'ouverture des fenêtres ImageWindow secondaires
    connect(ui->labelImage, &ClickableLabel::clicked, this, [=]() {
        ImageWindow *imgfen = new ImageWindow(this);
        if (!image.empty()) {
            imgfen->showRawImage(image);
            imgfen->setWindowTitle("Image Originale");
            imgfen->show();
        } else {
            QMessageBox::information(this, "Info", "Aucune image chargée à afficher.");
        }
    });

    connect(ui->labelImage_Mask, &ClickableLabel::clicked, this, [=]() {
        if (maskWindow) {
            cv::Mat gray = maskWindow->getPreprocessedGray();
            if (!gray.empty()) {
                ImageWindow *imgfen1 = new ImageWindow(this);
                imgfen1->setWindowTitle("Image Masquée (Prétraitement)");
                imgfen1->showRawImage(gray);
                imgfen1->show();
            } else {
                QMessageBox::information(this, "Info", "Le masque n'a pas encore été généré ou est vide. Cliquez sur 'Traiter' d'abord.");
            }
        } else {
            QMessageBox::information(this, "Info", "Veuillez d'abord cliquer sur 'Traiter' pour générer le masque.");
        }
    });

    connect(ui->labelImage_contours, &ClickableLabel::clicked, this, [=]() {
        if (resultWindow) {
            resultWindow->setWindowTitle("Image avec Contours et Composants");
            resultWindow->show();
        } else {
            QMessageBox::information(this, "Info", "Veuillez d'abord cliquer sur 'Afficher les bords' pour le traitement complet.");
        }
    });

    // Connexion de TraitementButton_2
    if (ui->TraitementButton_2) {
        connect(ui->TraitementButton_2, &QPushButton::clicked, this, &MainWindow::showExtractedComponentsImageAndList);
        ui->TraitementButton_2->setToolTip("Afficher l'image des composants extraits et mettre à jour la liste.");
    } else {
        qWarning() << "QPushButton 'TraitementButton_2' non trouvé dans l'UI.";
    }


    connect(ui->grayButton, &QPushButton::clicked, this, [this]() {
        if (image.empty()) {
            QMessageBox::warning(this, "Erreur", "Veuillez charger une image d'abord.");
            return;
        }
        if (maskWindow) delete maskWindow;
        maskWindow = new ImageWindow(this);
        maskWindow->setmaskImage(image);

        cv::Mat gray = maskWindow->getPreprocessedGray();
        if (!gray.empty()) {
            QImage imgGray(gray.data, gray.cols, gray.rows, gray.step, QImage::Format_Grayscale8);
            QLabel *maskDisplayLabel = ui->labelImage_Mask->findChild<QLabel*>("labelImage_Mask_2");
            if (maskDisplayLabel) {
                maskDisplayLabel->setPixmap(QPixmap::fromImage(imgGray).scaled(maskDisplayLabel->size(),
                                                                               Qt::KeepAspectRatio,
                                                                               Qt::SmoothTransformation));
            }
            afficherMessage(this, "Masque affiché!", "Info", QMessageBox::Information, 1000);
        } else {
            QMessageBox::warning(this, "Traitement Masque", "Le masque généré est vide. Veuillez vérifier les paramètres.");
        }
    });

    // Connecte le bouton "Show edges" (TraitementButton) pour le pipeline complet
    connect(ui->TraitementButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "TraitementButton cliqué. Initialisation du traitement...";
        if (image.empty()) {
            QMessageBox::warning(this, "Erreur", "Veuillez charger une image d'abord.");
            return;
        }
        if (resultWindow) delete resultWindow;
        resultWindow = new ImageWindow(this);
        connect(resultWindow, &ImageWindow::componentsDetected, this, &MainWindow::displayDetectedComponentsInList);
        connect(resultWindow, &ImageWindow::imageProcessed, this, &MainWindow::displayContoursImage);
        connect(resultWindow, &ImageWindow::extractedComponentsImageReady, this, &MainWindow::displayExtractedComponentsImage);

        m_displayFullResults = false; // IMPORTANT : Pas d'affichage complet quand ce bouton est cliqué

        if (ui->sliderBlurKsize) resultWindow->setBlurKsize(ui->sliderBlurKsize->value());
        if (ui->sliderSigmaX) resultWindow->setSigmaX(ui->sliderSigmaX->value());
        if (ui->sliderClaheClipLimit) resultWindow->setClaheClipLimit(ui->sliderClaheClipLimit->value());
        if (ui->sliderSeparationKsize) resultWindow->setSeparationKsize(ui->sliderSeparationKsize->value());
        if (ui->sliderFillHolesKsize) resultWindow->setFillHolesKsize(ui->sliderFillHolesKsize->value());
        if (ui->sliderContourMinArea) resultWindow->setContourMinArea(ui->sliderContourMinArea->value());

        resultWindow->setOriginalImage(image); // Cela lancera updateImageProcessing() dans ImageWindow

        afficherMessage(this, "Traitement complet lancé. Affichage des contours.", "Info", QMessageBox::Information, 1000);
    });

    QPixmap bkgnd("C:/Users/HP/Desktop/Stage pfe/PCB_PROJECT/pcb_blurred_background.png");
    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    this->setAutoFillBackground(true);
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(bkgnd));
    this->setPalette(palette);
}

MainWindow::~MainWindow() {
    delete ui;
    if (maskWindow) delete maskWindow;
    if (resultWindow) delete resultWindow;
}

void MainWindow::loadImageFromFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Choisir une image", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!fileName.isEmpty()) {
        clearProcessedImageDisplays(); // Nettoie aussi la liste stockée et réinitialise le flag

        image = cv::imread(fileName.toStdString());
        if (!image.empty()) {
            QImage imgQ(image.data, image.cols, image.rows, image.step, QImage::Format_BGR888);

            QLabel *originalImageDisplayLabel = ui->labelImage->findChild<QLabel*>("labelImage_2");
            if (originalImageDisplayLabel) {
                originalImageDisplayLabel->setPixmap(QPixmap::fromImage(imgQ).scaled(originalImageDisplayLabel->size(),
                                                                                     Qt::KeepAspectRatio,
                                                                                     Qt::SmoothTransformation));
            }
            afficherMessage(this, "Image chargée avec succès!", "Info", QMessageBox::Information, 2000);
        } else {
            QMessageBox::warning(this, "Erreur", "Impossible de charger l'image !");
            QLabel *originalImageDisplayLabel = ui->labelImage->findChild<QLabel*>("labelImage_2");
            if (originalImageDisplayLabel) {
                originalImageDisplayLabel->clear();
                originalImageDisplayLabel->setText(PLACEHOLDER_ORIGINAL_IMAGE);
            }
        }
    }
}

void MainWindow::updateSliderValue1(int value) { if (ui->value) ui->value->setText(QString::number(value)); }
void MainWindow::updateSliderValue2(int value) { if (ui->value_2) ui->value_2->setText(QString::number(value)); }
void MainWindow::updateSliderValue3(int value) { if (ui->value_3) ui->value_3->setText(QString::number(value)); }
void MainWindow::updateSliderValue4(int value) { if (ui->value_4) ui->value_4->setText(QString::number(value)); }
void MainWindow::updateSliderValue5(int value) { if (ui->value_5) ui->value_5->setText(QString::number(value)); }
void MainWindow::updateSliderValue6(int value) { if (ui->value_6) ui->value_6->setText(QString::number(value)); }

/**
 * @brief Slot pour déclencher le traitement complet de l'image dans l'instance de ImageWindow
 * et mettre à jour les affichages dans MainWindow.
 * Est appelé lorsque les sliders de paramètres sont modifiés.
 */
void MainWindow::updateComponentsView() {
    qDebug() << "updateComponentsView appelé (par slider).";
    if (!resultWindow || image.empty()) {
        qDebug() << "resultWindow non initialisé ou image vide. Ne traite pas.";
        return;
    }

    // IMPORTANT : Quand un slider est modifié, on veut afficher les résultats complets
    m_displayFullResults = true;

    if (ui->sliderBlurKsize) resultWindow->setBlurKsize(ui->sliderBlurKsize->value());
    if (ui->sliderSigmaX) resultWindow->setSigmaX(ui->sliderSigmaX->value());
    if (ui->sliderClaheClipLimit) resultWindow->setClaheClipLimit(ui->sliderClaheClipLimit->value());
    if (ui->sliderSeparationKsize) resultWindow->setSeparationKsize(ui->sliderSeparationKsize->value());
    if (ui->sliderFillHolesKsize) resultWindow->setFillHolesKsize(ui->sliderFillHolesKsize->value());
    if (ui->sliderContourMinArea) resultWindow->setContourMinArea(ui->sliderContourMinArea->value());

    resultWindow->updateImageProcessing(); // Cela lancera updateImageProcessing() dans ImageWindow
    afficherMessage(this, "Sliders modifiés. Affichage complet mis à jour.", "Info", QMessageBox::Information, 700);
}

void MainWindow::displayContoursImage(const QPixmap& resultPixmap) {
    QLabel *contoursDisplayLabel = ui->labelImage_contours->findChild<QLabel*>("labelImage_contours_2");
    if (contoursDisplayLabel) {
        contoursDisplayLabel->setPixmap(resultPixmap.scaled(
            contoursDisplayLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    }
}

// MODIFIÉ : Ce slot stocke le pixmap, et l'affiche si m_displayFullResults est vrai
void MainWindow::displayExtractedComponentsImage(const QPixmap& extractedComponentsPixmap) {
    m_lastExtractedComponentsPixmap = extractedComponentsPixmap; // Stocke toujours le pixmap

    if (m_displayFullResults) { // NOUVEAU : Affichage conditionnel
        qDebug() << "displayExtractedComponentsImage: m_displayFullResults est TRUE. Affichage de l'image extraite.";
        if (ui->labelResult_2) {
            ui->labelResult_2->setPixmap(extractedComponentsPixmap.scaled(
                ui->labelResult_2->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        } else {
            qWarning() << "QLabel 'labelResult_2' non trouvé dans l'UI.";
        }
    } else {
        qDebug() << "displayExtractedComponentsImage: m_displayFullResults est FALSE. Effacement de l'image extraite.";
        // Si l'affichage complet n'est pas actif, vide le label pour les composants extraits
        if (ui->labelResult_2) {
            ui->labelResult_2->clear();
            ui->labelResult_2->setText("Composants Extraits (cliquez pour afficher)");
            ui->labelResult_2->setAlignment(Qt::AlignCenter);
        }
    }
}

// MODIFIÉ : Ce slot stocke la liste, et l'affiche si m_displayFullResults est vrai
void MainWindow::displayDetectedComponentsInList(const QList<Composant>& components) {
    m_lastDetectedComponents = components; // Stocke toujours la liste

    if (m_displayFullResults) { // NOUVEAU : Affichage conditionnel
        qDebug() << "displayDetectedComponentsInList: m_displayFullResults est TRUE. Affichage de la liste et du compteur.";
        if (ui->listWidgetComponents) {
            ui->listWidgetComponents->clear();

            for (const Composant& comp : components) {
                QListWidgetItem* item = new QListWidgetItem();
                item->setText(comp.getDetails());
                item->setIcon(QIcon(comp.getImage()));
                ui->listWidgetComponents->addItem(item);
            }
        } else {
            qWarning() << "QListWidget 'listWidgetComponents' non initialisé dans MainWindow.";
        }


    } else {
        qDebug() << "displayDetectedComponentsInList: m_displayFullResults est FALSE. Effacement de la liste et du compteur.";
        // Si l'affichage complet n'est pas actif, vide la liste et réinitialise le compteur
        if (ui->listWidgetComponents) {
            ui->listWidgetComponents->clear();
        }

    }
}

// NOUVEAU SLOT : Déclenché par TraitementButton_2 pour forcer l'affichage des résultats complets
void MainWindow::showExtractedComponentsImageAndList() {
    qDebug() << "showExtractedComponentsImageAndList appelé (par TraitementButton_2).";

    if (m_lastExtractedComponentsPixmap.isNull() && m_lastDetectedComponents.isEmpty()) {
        QMessageBox::information(this, "Info", "Veuillez d'abord lancer un traitement (bouton 'Afficher les bords') pour générer des résultats.");
        qDebug() << "Aucun résultat stocké à afficher.";
        return;
    }

    m_displayFullResults = true; // Active l'affichage complet

    // Appelle les slots d'affichage avec les données stockées.
    // Comme m_displayFullResults est maintenant vrai, ils vont effectivement afficher.
    displayExtractedComponentsImage(m_lastExtractedComponentsPixmap);
    displayDetectedComponentsInList(m_lastDetectedComponents);

    afficherMessage(this, "Affichage des composants extraits et de la liste mis à jour!", "Info", QMessageBox::Information, 1000);
}


void MainWindow::resizeEvent(QResizeEvent *event) {
    QPixmap bkgnd("C:/Users/HP/Desktop/Stage pfe/PCB_PROJECT/pcb_blurred_background.png");
    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPalette palette;
    palette.setBrush(QPalette::Window, bkgnd);
    this->setPalette(palette);
    QMainWindow::resizeEvent(event);
}

void MainWindow::afficherMessage(QWidget *parent, const QString &texte,
                                 const QString &titre, QMessageBox::Icon style, int duree) {
    QMessageBox *message = new QMessageBox(parent);
    message->setWindowTitle(titre);
    message->setText(texte);
    message->setIcon(style);
    message->setStandardButtons(QMessageBox::NoButton);
    message->setStyleSheet("QMessageBox { color: black; }");
    message->show();
    QTimer::singleShot(duree, message, SLOT(close()));
    QTimer::singleShot(duree + 100, message, SLOT(deleteLater()));
}

// MODIFIÉ : Réinitialise aussi la liste stockée et le flag d'affichage
void MainWindow::clearProcessedImageDisplays() {
    qDebug() << "clearProcessedImageDisplays appelé. Réinitialisation des affichages.";
    QLabel *maskDisplayLabel = ui->labelImage_Mask->findChild<QLabel*>("labelImage_Mask_2");
    if (maskDisplayLabel) {
        maskDisplayLabel->setPixmap(QPixmap());
        maskDisplayLabel->setText(PLACEHOLDER_MASK_IMAGE);
    }

    QLabel *contoursDisplayLabel = ui->labelImage_contours->findChild<QLabel*>("labelImage_contours_2");
    if (contoursDisplayLabel) {
        contoursDisplayLabel->setPixmap(QPixmap());
        contoursDisplayLabel->setText(PLACEHOLDER_CONTOURS_IMAGE);
    }

    QLabel *originalImageDisplayLabel = ui->labelImage->findChild<QLabel*>("labelImage_2");
    if (originalImageDisplayLabel) {
        originalImageDisplayLabel->setPixmap(QPixmap());
        originalImageDisplayLabel->setText(PLACEHOLDER_ORIGINAL_IMAGE);
    }

    if (ui->labelResult_2) {
        ui->labelResult_2->setPixmap(QPixmap());
        ui->labelResult_2->setText("Composants Extraits");
    }
    m_lastExtractedComponentsPixmap = QPixmap(); // Efface le pixmap stocké

    if (ui->listWidgetComponents) {
        ui->listWidgetComponents->clear();
    }

    m_lastDetectedComponents.clear(); // Efface la liste stockée
    m_displayFullResults = false; // Réinitialise le flag d'affichage complet

    if (maskWindow) {
        maskWindow->close();
        delete maskWindow;
        maskWindow = nullptr;
    }
    if (resultWindow) {
        resultWindow->close();
        delete resultWindow;
        resultWindow = nullptr;
    }

    image.release();
}
