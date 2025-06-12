// mainwindow.cpp

// Inclusion des en-têtes nécessaires pour la classe MainWindow
#include "mainwindow.h"                        // L'en-tête de la classe MainWindow elle-même
#include "ui_mainwindow.h"                    // Fichier généré par Qt Designer pour l'interface utilisateur
#include "clickablelabel.h"                  // Une classe de QLabel personnalisée qui émet un signal au clic
#include "imagewindow.h"                    // La fenêtre secondaire pour afficher les images en grand
#include "QFileDialog"                     // Pour ouvrir la boîte de dialogue de sélection de fichier
#include "QMessageBox"                     // Pour afficher des boîtes de message (erreurs, informations)
#include "QTimer"                   // Pour les temporisateurs (par exemple, pour fermer automatiquement une QMessageBox)
#include <QLabel>                // Widget pour afficher du texte ou des images
#include <QFrame>             // Widget générique pour grouper ou encadrer d'autres widgets
#include <QAction>          // Actions dans les menus ou barres d'outils
#include <QKeySequence>     // Pour définir des raccourcis clavier
#include <QListWidget>       // Widget pour afficher une liste d'éléments
#include <QVBoxLayout>      // Gestionnaire de mise en page vertical
#include <QSlider>         // Widget slider pour ajuster des valeurs
#include "composant.h"     // Votre classe personnalisée 'Composant' pour représenter les composants détectés
#include <QDebug>         // Pour les messages de débogage dans la console



// Constructeur de la classe MainWindow
// Initialise les membres de la classe et configure l'interface utilisateur.
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)    // Appelle le constructeur de la classe de base QMainWindow
    , ui(new Ui::MainWindow)       // Initialise l'interface utilisateur générée par Qt Designer
    , maskWindow(nullptr)     // Pointeur vers la fenêtre du masque, initialisé à nul
    , resultWindow(nullptr)      // Pointeur vers la fenêtre des résultats, initialisé à nul
    , m_componentListWidget(nullptr)    // Pointeur vers le QListWidget des composants, initialisé à nul (à vérifier si utilisé ou si ui->listWidgetComponents est directement utilisé)
    , m_componentCountLabel(nullptr)     // Pointeur vers le QLabel pour le compte des composants, initialisé à nul (à vérifier si utilisé)
    , m_lastExtractedComponentsPixmap(QPixmap())    // Initialise le QPixmap stocké pour les composants extraits
    , m_lastDetectedComponents(QList<Composant>())     // Initialise la liste stockée des composants détectés
    , m_displayFullResults(false) // **Flag important** : Initialisé à false. Les résultats complets ne s'affichent pas par défaut.
{
    ui->setupUi(this);    // Configure l'interface utilisateur à partir du fichier .ui
    ui->centralwidget->setToolTip("");

    // Configuration des infobulles pour les QLabel d'images
    ui->labelImage_2->setToolTip("Click to open in new window");
    ui->labelImage_Mask_2->setToolTip("Click to open in new window");
    ui->labelImage_contours_2->setToolTip("Click to open in new window");

    if (ui->labelResult_2) { // Vérifie si labelResult_2 existe dans l'UI
        ui->labelResult_2->setToolTip("Extracted components on a white background");
    }

    // Création d'une action pour la sélection d'image avec un raccourci clavier (Ctrl+N)
    QAction *SelectedImageAction = new QAction(tr("Select"), this);
    SelectedImageAction->setShortcut(QKeySequence("Ctrl+N"));
    // Connexion de l'action au slot loadImageFromFile pour charger une image
    connect(SelectedImageAction, &QAction::triggered, this, &MainWindow::loadImageFromFile);
    this->addAction(SelectedImageAction); // Ajoute l'action à la fenêtre principale

    // Configuration des textes de remplacement initiaux pour les QLabel d'images
    QLabel *originalImageDisplayLabel = ui->labelImage->findChild<QLabel*>("labelImage_2");
    if (originalImageDisplayLabel) {
        originalImageDisplayLabel->setText(PLACEHOLDER_ORIGINAL_IMAGE);
        originalImageDisplayLabel->setAlignment(Qt::AlignCenter); // Centre le texte
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

    // Placeholder spécifique pour les composants extraits
    if (ui->labelResult_2) {
        ui->labelResult_2->setText("Extracted Components (click to display)"); // Texte initial indiquant l'action requise
        ui->labelResult_2->setAlignment(Qt::AlignCenter);
    }

    // Configuration du QListWidget pour les composants
    if (ui->listWidgetComponents) {
        ui->listWidgetComponents->setAlternatingRowColors(true); // Active les couleurs alternées pour une meilleure lisibilité
        ui->listWidgetComponents->setViewMode(QListView::ListMode); // Affiche les éléments en liste
        ui->listWidgetComponents->setMovement(QListView::Static); // Les éléments ne peuvent pas être déplacés par l'utilisateur
        ui->listWidgetComponents->setResizeMode(QListView::Adjust); // Le redimensionnement ajuste la taille des éléments
        ui->listWidgetComponents->setFlow(QListView::TopToBottom); // Les éléments s'affichent de haut en bas
    }

    // Configuration du bouton de sélection d'image
    ui->SelectedImage->setFixedSize(90, 90); // Taille fixe pour le bouton
    ui->SelectedImage->setToolTip("Click to select an image"); // Infobulle
    // Affiche un message d'information initial à l'utilisateur
    afficherMessage(this, "Ready to extract your electronic schematic!",
                    "Info", QMessageBox::Information, 1000); // Durée de 1 seconde

    // Connexion du bouton de sélection d'image au slot loadImageFromFile
    connect(ui->SelectedImage, &QPushButton::clicked, this, &MainWindow::loadImageFromFile);

    // Connexions des sliders et initialisation des valeurs affichées
    // Chaque slider est connecté à deux slots :
    // 1. Un slot pour mettre à jour la valeur affichée à côté du slider.
    // 2. Le slot `updateComponentsView` pour déclencher le traitement de l'image avec les nouveaux paramètres.
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

    // Connexion du bouton "Clear" pour réinitialiser les affichages
    connect(ui->ClearButton,&QPushButton::clicked,this,&MainWindow::clearProcessedImageDisplays);

    // Connexions pour l'ouverture des fenêtres `ImageWindow` secondaires au clic sur les QLabel d'images
    // Ces connexions utilisent des expressions lambda pour passer le contexte actuel de `this`.
    connect(ui->labelImage, &ClickableLabel::clicked, this, [=]() {
        ImageWindow *imgfen = new ImageWindow(this); // Crée une nouvelle fenêtre d'image
        if (!image.empty()) { // Vérifie si une image est chargée
            imgfen->showRawImage(image); // Affiche l'image originale
            imgfen->setWindowTitle("Original Image"); // Définit le titre de la fenêtre
            imgfen->show(); // Affiche la fenêtre
        } else {
            QMessageBox::information(this, "Info", "No image loaded to display."); // Message si pas d'image
        }
    });

    connect(ui->labelImage_Mask, &ClickableLabel::clicked, this, [=]() {
        if (maskWindow) { // Vérifie si la fenêtre de masque existe
            cv::Mat gray = maskWindow->getPreprocessedGray(); // Récupère l'image pré-traitée
            if (!gray.empty()) {
                ImageWindow *imgfen1 = new ImageWindow(this);
                imgfen1->setWindowTitle("Preprocessed Image");
                imgfen1->showRawImage(gray);
                imgfen1->show();
            } else {
                QMessageBox::information(this, "Info", "Click 'Process' first."); // Message si le traitement n'est pas fait
            }
        }
    });

    connect(ui->labelImage_contours, &ClickableLabel::clicked, this, [=]() {
        if (resultWindow) { // Vérifie si la fenêtre de résultats existe
            resultWindow->setWindowTitle("Image with Edges and Components");
            resultWindow->show(); // Affiche la fenêtre des résultats complets (contours)
        } else {
            QMessageBox::information(this, "Info", "Please click 'Show Edges' first for full processing."); // Message si le traitement complet n'est pas fait
        }
    });

    // Connexion du bouton "TraitementButton_2" (bouton "Traiter" pour les composants extraits)
    if (ui->TraitementButton_2) {
        connect(ui->TraitementButton_2, &QPushButton::clicked, this, &MainWindow::showExtractedComponentsImageAndList);
        ui->TraitementButton_2->setToolTip("Show image of extracted components and update list.");
    } else {
        qWarning() << "QPushButton 'ProcessingButton_2' not found in the UI."; // Avertissement si le bouton n'est pas trouvé
    }

    // Connexion du bouton "grayButton" (pour afficher l'image pré-traitée en niveaux de gris)
    connect(ui->grayButton, &QPushButton::clicked, this, [this]() {
        if (image.empty()) {
            QMessageBox::warning(this, "Error", "Please upload an image first."); // Vérifie si une image est chargée
            return;
        }
        if (maskWindow) delete maskWindow; // Supprime l'ancienne fenêtre si elle existe
        maskWindow = new ImageWindow(this); // Crée une nouvelle fenêtre ImageWindow
        maskWindow->setmaskImage(image); // Définit l'image originale pour le traitement du masque

        cv::Mat gray = maskWindow->getPreprocessedGray(); // Obtient l'image pré-traitée en niveaux de gris
        if (!gray.empty()) {
            QImage imgGray(gray.data, gray.cols, gray.rows, gray.step, QImage::Format_Grayscale8); // Convertit cv::Mat en QImage
            QLabel *maskDisplayLabel = ui->labelImage_Mask->findChild<QLabel*>("labelImage_Mask_2");
            if (maskDisplayLabel) {
                maskDisplayLabel->setPixmap(QPixmap::fromImage(imgGray).scaled(maskDisplayLabel->size(),
                                                                               Qt::KeepAspectRatio,
                                                                               Qt::SmoothTransformation)); // Affiche l'image
            }
            afficherMessage(this, "Preprocessed image displayed!", "Info", QMessageBox::Information, 1000);
        } else {
            QMessageBox::warning(this, "Processing", "Processing is empty. Please check the settings."); // Message d'erreur
        }
    });

    // Connecte le bouton "Show edges" (TraitementButton) pour le pipeline complet
    connect(ui->TraitementButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "TraitementButton cliqué. Initialisation du traitement...";
        if (image.empty()) {
            QMessageBox::warning(this, "Error", "Please upload an image first.");
            return;
        }
        if (resultWindow) delete resultWindow; // Supprime l'ancienne fenêtre de résultats
        resultWindow = new ImageWindow(this); // Crée une nouvelle fenêtre ImageWindow pour les résultats
        // Connexions des signaux d'ImageWindow vers les slots de MainWindow pour la mise à jour de l'UI
        connect(resultWindow, &ImageWindow::componentsDetected, this, &MainWindow::displayDetectedComponentsInList);
        connect(resultWindow, &ImageWindow::imageProcessed, this, &MainWindow::displayContoursImage);
        connect(resultWindow, &ImageWindow::extractedComponentsImageReady, this, &MainWindow::displayExtractedComponentsImage);

        // NOUVEAU : Réinitialise le flag d'affichage complet.
        // Cela signifie que même si les calculs sont faits, l'image finale des composants
        // et la liste ne seront PAS affichées tant que `TraitementButton_2` n'est pas cliqué.
        m_displayFullResults = false;

        // Définit les paramètres de traitement dans `resultWindow` à partir des valeurs des sliders
        if (ui->sliderBlurKsize) resultWindow->setBlurKsize(ui->sliderBlurKsize->value());
        if (ui->sliderSigmaX) resultWindow->setSigmaX(ui->sliderSigmaX->value());
        if (ui->sliderClaheClipLimit) resultWindow->setClaheClipLimit(ui->sliderClaheClipLimit->value());
        if (ui->sliderSeparationKsize) resultWindow->setSeparationKsize(ui->sliderSeparationKsize->value());
        if (ui->sliderFillHolesKsize) resultWindow->setFillHolesKsize(ui->sliderFillHolesKsize->value());
        if (ui->sliderContourMinArea) resultWindow->setContourMinArea(ui->sliderContourMinArea->value());

        resultWindow->setOriginalImage(image); // Lance le traitement dans ImageWindow
        // (Cela déclenchera `updateImageProcessing()` dans `ImageWindow` et enverra les signaux `imageProcessed`, `componentsDetected`, `extractedComponentsImageReady`).

        afficherMessage(this, "Full processing started. Displaying outlines.", "Info", QMessageBox::Information, 1000);
    });

    // Configuration de l'image de fond de la fenêtre principale
    QPixmap bkgnd("C:/Users/HP/Desktop/Stage pfe/PCB_PROJECT/pcb_blurred_background.png");
    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation); // Redimensionne l'image
    this->setAutoFillBackground(true); // Permet le remplissage automatique du fond
    QPalette palette; // Crée une palette
    palette.setBrush(QPalette::Window, QBrush(bkgnd)); // Définit le pinceau de fond
    this->setPalette(palette); // Applique la palette à la fenêtre
}

// Destructeur de la classe MainWindow
// Libère la mémoire allouée dynamiquement.
MainWindow::~MainWindow() {
    delete ui; // Supprime l'objet UI
    if (maskWindow) delete maskWindow; // Supprime la fenêtre du masque si elle existe
    if (resultWindow) delete resultWindow; // Supprime la fenêtre des résultats si elle existe
}

/**
 * @brief Slot pour charger une image à partir d'un fichier sélectionné par l'utilisateur.
 * Ouvre une boîte de dialogue de sélection de fichier.
 */
void MainWindow::loadImageFromFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Choose an image", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!fileName.isEmpty()) { // Si un fichier a été sélectionné
        clearProcessedImageDisplays(); // Nettoie tous les affichages précédents et réinitialise les flags

        image = cv::imread(fileName.toStdString()); // Charge l'image avec OpenCV
        if (!image.empty()) { // Si l'image a été chargée avec succès
            // Convertit l'image OpenCV (BGR888) en QImage
            QImage imgQ(image.data, image.cols, image.rows, image.step, QImage::Format_BGR888);

            QLabel *originalImageDisplayLabel = ui->labelImage->findChild<QLabel*>("labelImage_2");
            if (originalImageDisplayLabel) {
                // Affiche l'image originale dans le QLabel approprié, redimensionnée pour s'adapter.
                originalImageDisplayLabel->setPixmap(QPixmap::fromImage(imgQ).scaled(originalImageDisplayLabel->size(),
                                                                                     Qt::KeepAspectRatio,
                                                                                     Qt::SmoothTransformation));
            }
            afficherMessage(this, "Image loaded successfully!", "Info", QMessageBox::Information, 2000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to load image!"); // Message d'erreur si le chargement échoue
            QLabel *originalImageDisplayLabel = ui->labelImage->findChild<QLabel*>("labelImage_2");
            if (originalImageDisplayLabel) {
                originalImageDisplayLabel->clear(); // Efface l'image
                originalImageDisplayLabel->setText(PLACEHOLDER_ORIGINAL_IMAGE); // Remet le placeholder
            }
        }
    }
}

// Slots pour mettre à jour les valeurs affichées à côté de chaque slider.
// Simplement met à jour le texte d'un QLabel avec la valeur du slider.
void MainWindow::updateSliderValue1(int value) { if (ui->value) ui->value->setText(QString::number(value)); }
void MainWindow::updateSliderValue2(int value) { if (ui->value_2) ui->value_2->setText(QString::number(value)); }
void MainWindow::updateSliderValue3(int value) { if (ui->value_3) ui->value_3->setText(QString::number(value)); }
void MainWindow::updateSliderValue4(int value) { if (ui->value_4) ui->value_4->setText(QString::number(value)); }
void MainWindow::updateSliderValue5(int value) { if (ui->value_5) ui->value_5->setText(QString::number(value)); }
void MainWindow::updateSliderValue6(int value) { if (ui->value_6) ui->value_6->setText(QString::number(value)); }

/**
 * @brief Slot pour déclencher le traitement complet de l'image dans l'instance de ImageWindow
 * et mettre à jour les affichages dans MainWindow.
 * Ce slot est appelé lorsque les sliders de paramètres sont modifiés.
 */
void MainWindow::updateComponentsView() {
    qDebug() << "updateComponentsView appelé (par slider).";
    if (!resultWindow || image.empty()) { // Vérifie si `resultWindow` est initialisé et si une image est chargée
        qDebug() << "resultWindow uninitialized or empty image. Do not process.";
        return; // N'effectue pas le traitement si les prérequis ne sont pas remplis
    }

    // IMPORTANT : Si `m_displayFullResults` est false, les sliders ne forcent PAS l'affichage complet.
    // L'affichage complet est activé **uniquement** par le bouton `TraitementButton_2`.
    // Cependant, le traitement sous-jacent est toujours effectué pour mettre à jour les données
    // (pixmap et liste de composants) en arrière-plan, afin qu'elles soient prêtes lorsque l'utilisateur clique.

    // Définit les paramètres de traitement dans `resultWindow` avec les valeurs actuelles des sliders.
    if (ui->sliderBlurKsize) resultWindow->setBlurKsize(ui->sliderBlurKsize->value());
    if (ui->sliderSigmaX) resultWindow->setSigmaX(ui->sliderSigmaX->value());
    if (ui->sliderClaheClipLimit) resultWindow->setClaheClipLimit(ui->sliderClaheClipLimit->value());
    if (ui->sliderSeparationKsize) resultWindow->setSeparationKsize(ui->sliderSeparationKsize->value());
    if (ui->sliderFillHolesKsize) resultWindow->setFillHolesKsize(ui->sliderFillHolesKsize->value());
    if (ui->sliderContourMinArea) resultWindow->setContourMinArea(ui->sliderContourMinArea->value());

    // Déclenche le traitement de l'image dans l'objet `resultWindow`.
    // Cela entraînera l'émission des signaux `componentsDetected` et `extractedComponentsImageReady`
    // (qui sont connectés aux slots `displayDetectedComponentsInList` et `displayExtractedComponentsImage`).
    resultWindow->updateImageProcessing();

    // Conditionnel : Affiche un message d'information si les résultats sont déjà visibles.
    // Cela évite d'afficher un message à chaque déplacement de slider si l'utilisateur n'a pas encore demandé l'affichage des résultats finaux.
    if (m_displayFullResults) {
        afficherMessage(this, "Sliders modified. Full display updated.", "Info", QMessageBox::Information, 700);
    }
}

/**
 * @brief Slot pour afficher l'image des contours et des composants dans le QLabel dédié.
 * Ce slot est connecté au signal `imageProcessed` de `ImageWindow`.
 * @param resultPixmap Le QPixmap de l'image traitée avec les contours.
 */
void MainWindow::displayContoursImage(const QPixmap& resultPixmap) {
    QLabel *contoursDisplayLabel = ui->labelImage_contours->findChild<QLabel*>("labelImage_contours_2");
    if (contoursDisplayLabel) {
        // Redimensionne et affiche le pixmap dans le QLabel.
        contoursDisplayLabel->setPixmap(resultPixmap.scaled(
            contoursDisplayLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
    }
}

/**
 * @brief Slot pour afficher l'image des composants extraits sur fond blanc.
 * Ce slot est connecté au signal `extractedComponentsImageReady` de `ImageWindow`.
 * Il stocke toujours le pixmap, mais ne l'affiche que si `m_displayFullResults` est vrai.
 * @param extractedComponentsPixmap Le QPixmap de l'image des composants extraits.
 */
void MainWindow::displayExtractedComponentsImage(const QPixmap& extractedComponentsPixmap) {
    m_lastExtractedComponentsPixmap = extractedComponentsPixmap; // Stocke toujours le pixmap, qu'il soit affiché ou non

    if (m_displayFullResults) { // Affichage conditionnel basé sur le flag
        qDebug() << "displayExtractedComponentsImage: m_displayFullResults est TRUE. Affichage de l'image extraite.";
        if (ui->labelResult_2) {
            // Affiche l'image extraite dans le QLabel
            ui->labelResult_2->setPixmap(extractedComponentsPixmap.scaled(
                ui->labelResult_2->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        } else {
            qWarning() << "QLabel 'labelResult_2' non trouvé dans l'UI.";
        }
    } else {
        qDebug() << "displayExtractedComponentsImage: m_displayFullResults est FALSE. Effacement de l'image extraite.";
        // Si l'affichage complet n'est pas actif, vide le label et remet le texte de placeholder.
        if (ui->labelResult_2) {
            ui->labelResult_2->clear();
            ui->labelResult_2->setText("Extracted Components (click to display)");
            ui->labelResult_2->setAlignment(Qt::AlignCenter);
        }
    }
}

/**
 * @brief Slot pour afficher la liste des composants détectés dans le QListWidget.
 * Ce slot est connecté au signal `componentsDetected` de `ImageWindow`.
 * Il stocke toujours la liste, mais ne l'affiche que si `m_displayFullResults` est vrai.
 * @param components La QList de `Composant` détectés.
 */
void MainWindow::displayDetectedComponentsInList(const QList<Composant>& components) {
    m_lastDetectedComponents = components; // Stocke toujours la liste des composants, qu'elle soit affichée ou non

    if (m_displayFullResults) { // Affichage conditionnel basé sur le flag
        qDebug() << "displayDetectedComponentsInList: m_displayFullResults est TRUE. Affichage de la liste et du compteur.";
        if (ui->listWidgetComponents) {
            ui->listWidgetComponents->clear(); // Efface les éléments précédents de la liste

            // Ajoute chaque composant à la liste
            for (const Composant& comp : components) {
                QListWidgetItem* item = new QListWidgetItem();
                item->setText(comp.getDetails()); // Définit le texte de l'élément (détails du composant)
                item->setIcon(QIcon(comp.getImage())); // Définit l'icône de l'élément (image du composant)
                ui->listWidgetComponents->addItem(item); // Ajoute l'élément à la liste
            }
        } else {
            qWarning() << "QListWidget 'listWidgetComponents' not initialized in MainWindow.";
        }
    } else {
        qDebug() << "displayDetectedComponentsInList: m_displayFullResults is FALSE. Clearing list and counter.";
        // Si l'affichage complet n'est pas actif, vide la liste.
        if (ui->listWidgetComponents) {
            ui->listWidgetComponents->clear();
        }
    }
}

/**
 * @brief NOUVEAU SLOT : Déclenché par le bouton `TraitementButton_2` pour forcer l'affichage
 * des résultats complets (image des composants extraits et liste des composants).
 * C'est le seul endroit qui active le flag `m_displayFullResults`.
 */
void MainWindow::showExtractedComponentsImageAndList() {
    qDebug() << "showExtractedComponentsImageAndList called (by TraitementButton_2).";

    // IMPORTANT : Active le mode d'affichage complet des résultats.
    // Désormais, les slots `displayExtractedComponentsImage` et `displayDetectedComponentsInList`
    // afficheront réellement les données qu'ils reçoivent.
    m_displayFullResults = true;

    // Assurez-vous que le traitement est à jour avec les derniers paramètres des sliders
    // et que les données (pixmap et liste) sont stockées et ensuite affichées.
    // Cela déclenchera `displayExtractedComponentsImage` et `displayDetectedComponentsInList`
    // qui, grâce à `m_displayFullResults = true`, afficheront les résultats.
    if (!resultWindow || image.empty()) {
        // Si `resultWindow` n'est pas encore initialisé (par exemple, si "Show Edges" n'a pas été cliqué),
        // ou si aucune image n'est chargée, nous devons l'initialiser et le configurer.
        if (!resultWindow) {
            resultWindow = new ImageWindow(this); // Crée une nouvelle instance de ImageWindow
            // Reconnecte tous les signaux nécessaires de `resultWindow`
            connect(resultWindow, &ImageWindow::componentsDetected, this, &MainWindow::displayDetectedComponentsInList);
            connect(resultWindow, &ImageWindow::imageProcessed, this, &MainWindow::displayContoursImage);
            connect(resultWindow, &ImageWindow::extractedComponentsImageReady, this, &MainWindow::displayExtractedComponentsImage);
        }
        if (image.empty()) {
            QMessageBox::information(this, "Info", "Please load an image first before displaying components.");
            qDebug() << "Aucun résultat stocké à afficher."; // Message de débogage interne
            m_displayFullResults = false; // Réinitialise le flag si aucune image n'est présente
            return;
        }

        // Applique les paramètres actuels des sliders à `resultWindow` avant de lancer le traitement.
        if (ui->sliderBlurKsize) resultWindow->setBlurKsize(ui->sliderBlurKsize->value());
        if (ui->sliderSigmaX) resultWindow->setSigmaX(ui->sliderSigmaX->value());
        if (ui->sliderClaheClipLimit) resultWindow->setClaheClipLimit(ui->sliderClaheClipLimit->value());
        if (ui->sliderSeparationKsize) resultWindow->setSeparationKsize(ui->sliderSeparationKsize->value());
        if (ui->sliderFillHolesKsize) resultWindow->setFillHolesKsize(ui->sliderFillHolesKsize->value());
        if (ui->sliderContourMinArea) resultWindow->setContourMinArea(ui->sliderContourMinArea->value());

        resultWindow->setOriginalImage(image); // Lance le traitement (ce qui déclenchera les signaux connectés)
    } else {
        // Si `resultWindow` existe déjà et l'image est chargée,
        // il suffit de déclencher une mise à jour du traitement.
        // Cela forcera `ImageWindow` à recalculer et à émettre à nouveau ses signaux,
        // ce qui mettra à jour les affichages puisque `m_displayFullResults` est maintenant vrai.
        resultWindow->updateImageProcessing();
    }

    afficherMessage(this,"Display of extracted components and list updated!", "Info", QMessageBox::Information, 1000);
}

/**
 * @brief Gestionnaire de l'événement de redimensionnement de la fenêtre.
 * Permet de redimensionner l'image de fond pour qu'elle s'adapte à la nouvelle taille de la fenêtre.
 * @param event L'événement de redimensionnement.
 */
void MainWindow::resizeEvent(QResizeEvent *event) {
    QPixmap bkgnd("C:/Users/HP/Desktop/Stage pfe/PCB_PROJECT/pcb_blurred_background.png"); // Charge l'image de fond
    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation); // Redimensionne l'image
    QPalette palette; // Crée une palette
    palette.setBrush(QPalette::Window, bkgnd); // Définit le pinceau de fond
    this->setPalette(palette); // Applique la palette
    QMainWindow::resizeEvent(event); // Appelle la méthode de la classe de base
}

/**
 * @brief Fonction utilitaire pour afficher un message temporaire à l'utilisateur.
 * Affiche une QMessageBox qui se ferme automatiquement après une durée spécifiée.
 * @param parent Le widget parent de la boîte de message.
 * @param texte Le texte du message à afficher.
 * @param titre Le titre de la boîte de message.
 * @param style L'icône de la boîte de message (QMessageBox::Icon).
 * @param duree La durée (en millisecondes) pendant laquelle le message reste affiché.
 */
void MainWindow::afficherMessage(QWidget *parent, const QString &texte,
                                 const QString &titre, QMessageBox::Icon style, int duree) {
    QMessageBox *message = new QMessageBox(parent); // Crée une nouvelle boîte de message
    message->setWindowTitle(titre); // Définit le titre
    message->setText(texte); // Définit le texte
    message->setIcon(style); // Définit l'icône
    message->setStandardButtons(QMessageBox::NoButton); // Pas de boutons standard (OK, Cancel, etc.)
    message->setStyleSheet("QMessageBox { color: black; }"); // Applique un style CSS (texte noir)
    message->show(); // Affiche la boîte de message
    QTimer::singleShot(duree, message, SLOT(close())); // Ferme la boîte après 'duree' millisecondes
    QTimer::singleShot(duree + 100, message, SLOT(deleteLater())); // Supprime l'objet de la mémoire après la fermeture
}

/**
 * @brief Slot pour effacer tous les affichages d'images traitées et réinitialiser l'état.
 * Ce slot est appelé lors du chargement d'une nouvelle image ou en cliquant sur le bouton "Clear".
 */
void MainWindow::clearProcessedImageDisplays() {
    qDebug() << "clearProcessedImageDisplays appelé. Réinitialisation des affichages.";

    // Réinitialise les QLabel d'affichage des images avec leurs placeholders
    QLabel *maskDisplayLabel = ui->labelImage_Mask->findChild<QLabel*>("labelImage_Mask_2");
    if (maskDisplayLabel) {
        maskDisplayLabel->setPixmap(QPixmap()); // Efface l'image
        maskDisplayLabel->setText(PLACEHOLDER_MASK_IMAGE); // Remet le placeholder
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

    // Réinitialise le QLabel des composants extraits avec son placeholder et efface le pixmap stocké
    if (ui->labelResult_2) {
        ui->labelResult_2->setPixmap(QPixmap());
        ui->labelResult_2->setText("Extracted Components (click to display)"); // Remet le placeholder initial
    }
    m_lastExtractedComponentsPixmap = QPixmap(); // Efface le pixmap stocké en mémoire

    // Efface le QListWidget des composants
    if (ui->listWidgetComponents) {
        ui->listWidgetComponents->clear();
    }
    m_lastDetectedComponents.clear(); // Efface la liste stockée des objets Composant

    // **MODIFIÉ : Réinitialise le flag d'affichage complet à `false`.**
    // Cela garantit que les résultats ne s'affichent pas automatiquement après un effacement.
    m_displayFullResults = false;

    // Ferme et supprime les fenêtres secondaires si elles existent
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

    image.release(); // Libère la mémoire de l'image OpenCV originale
}
