// imagewindow.cpp
#include "imagewindow.h"
#include "ui_imagewindow.h" // Incluez si ImageWindow a un fichier .ui
#include <QImage>
#include <QPixmap>
#include <QMessageBox>
#include <QFileDialog>
#include <QAction>
#include <QKeySequence>
#include <QDebug>
#include <vector>
#include <filesystem> // Pour créer des répertoires (C++17)

// Assurez-vous d'inclure les headers OpenCV nécessaires
#include <opencv2/imgproc.hpp> // Pour cvtColor, GaussianBlur, threshold, adaptiveThreshold, findContours, drawContours, morphologyEx
#include <opencv2/highgui.hpp> // Pour imwrite
#include <opencv2/imgcodecs.hpp> // Pour imread

namespace fs = std::filesystem;
using namespace cv;
using namespace std;

/**
 * @brief Constructeur de la classe ImageWindow.
 * Initialise les paramètres de traitement par défaut et configure l'interface utilisateur.
 * @param parent Pointeur vers le widget parent.
 */
ImageWindow::ImageWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ImageWindow), // Initialise l'objet UI si un fichier .ui est utilisé
    // Initialisation des paramètres de traitement avec des valeurs par défaut
    m_blurKsize(1),
    m_sigmaX(5),
    m_claheClipLimit(15),
    m_separationKsize(3),
    m_fillHolesKsize(1),
    m_contourMinArea(50)
{
    ui->setupUi(this); // Configure l'interface utilisateur de cette fenêtre

    // Configuration du bouton de sauvegarde et de son raccourci clavier
    // Vérifie si le bouton "SaveButton" est défini dans le fichier .ui de ImageWindow
    if (ui->SaveButton) {
        ui->SaveButton->setToolTip("Sauvegarder (Ctrl+S)");
        connect(ui->SaveButton, &QPushButton::clicked, this, &ImageWindow::saveImage);
    } else {
        qWarning() << "SaveButton non trouvé dans l'UI de ImageWindow. Le raccourci clavier fonctionnera toujours.";
    }

    QAction *saveAction = new QAction(tr("Sauvegarder"), this);
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(saveAction, &QAction::triggered, this, &ImageWindow::saveImage);
    this->addAction(saveAction);
}

/**
 * @brief Destructeur de la classe ImageWindow.
 * Libère les ressources allouées.
 */
ImageWindow::~ImageWindow()
{
    delete ui; // Supprime l'objet UI
}

/**
 * @brief Helper pour convertir une cv::Mat en QPixmap.
 * Cette fonction est essentielle pour afficher les images OpenCV dans les widgets Qt.
 * @param mat La cv::Mat à convertir.
 * @return La QPixmap résultante. Retourne un QPixmap nul si la Mat est vide ou a un format non supporté.
 */
QPixmap ImageWindow::cvMatToQPixmap(const cv::Mat& mat) {
    if (mat.empty()) {
        qDebug() << "cvMatToQPixmap: Input Mat is empty.";
        return QPixmap();
    }

    cv::Mat rgb;
    // Convertit l'image en RGB si elle est en niveaux de gris (1 canal) ou BGR (3 canaux).
    // Qt s'attend à des images RGB pour QImage::Format_RGB888.
    if (mat.channels() == 1) {
        cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB);
    } else if (mat.channels() == 3) {
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB); // Conversion BGR vers RGB pour Qt
    } else {
        qWarning() << "cvMatToQPixmap: Nombre de canaux non supporté : " << mat.channels();
        return QPixmap();
    }

    // Crée une QImage à partir des données RGB de la Mat
    QImage qimg(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);
    // QPixmap::fromImage fait une copie des données, ce qui est sûr pour l'affichage asynchrone
    return QPixmap::fromImage(qimg);
}

/**
 * @brief Définit l'image originale à traiter et lance le pipeline complet de détection de composants.
 * Cette fonction est le point d'entrée principal pour le traitement d'une nouvelle image.
 * @param originalImage L'image OpenCV d'entrée.
 */
void ImageWindow::setOriginalImage(const cv::Mat& originalImage)
{
    m_originalImage = originalImage.clone(); // Clone l'image pour éviter les modifications externes
    if (!m_originalImage.empty()) {
        updateImageProcessing(); // Lance le pipeline complet
    } else {
        qDebug() << "ImageWindow::setOriginalImage: L'image fournie est vide.";
    }
}

/**
 * @brief Définit l'image à afficher dans le QLabel principal de cette fenêtre (interne).
 * @param pixmap L'image QPixmap à afficher.
 */
void ImageWindow::setImage(const QPixmap& pixmap) {
    if (!pixmap.isNull() && ui->label) { // ui->label est le QLabel dans ImageWindow.ui
        // Redimensionne le pixmap pour s'adapter au QLabel tout en conservant les proportions
        QPixmap scaledPixmap = pixmap.scaled(ui->label->size(),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
        ui->label->setPixmap(scaledPixmap); // Définit le pixmap du QLabel
        ui->label->setAlignment(Qt::AlignCenter); // Centre l'image dans le QLabel
    } else if (!ui->label) {
        qWarning() << "QLabel 'label' non trouvé dans l'UI de ImageWindow.";
    }
}

/**
 * @brief Effectue un prétraitement sur l'image fournie pour créer un "masque"
 * (image en niveaux de gris prétraitée).
 * Le résultat est stocké dans m_preprocessedMaskImage et affiché dans cette fenêtre.
 * @param img L'image d'entrée pour le masquage.
 */
void ImageWindow::setmaskImage(const cv::Mat& img) {
    if (img.empty()) {
        qDebug() << "ImageWindow::setmaskImage: L'image fournie est vide.";
        m_preprocessedMaskImage = cv::Mat(); // Assurez-vous que l'image interne est vide
        return;
    }

    cv::Mat processed_gray;
    cv::cvtColor(img, processed_gray, cv::COLOR_BGR2GRAY);

    // Appliquer le flou gaussien selon les paramètres
    int ksize_val = m_blurKsize * 2 + 1;
    double sigmaX_val = m_sigmaX / 10.0;
    if (sigmaX_val < 0.1) sigmaX_val = 0.1; // Évite un sigmaX trop petit
    if (ksize_val > 0) { // Assure que la taille du noyau est valide
        cv::GaussianBlur(processed_gray, processed_gray, cv::Size(ksize_val, ksize_val), sigmaX_val);
    }

    // Appliquer CLAHE pour l'amélioration du contraste
    Ptr<CLAHE> clahe = createCLAHE();
    clahe->setClipLimit(m_claheClipLimit / 10.0);
    clahe->apply(processed_gray, processed_gray);

    m_preprocessedMaskImage = processed_gray; // Stocke l'image prétraitée en niveaux de gris
    setImage(cvMatToQPixmap(m_preprocessedMaskImage)); // Affiche le masque dans cette ImageWindow (si ouverte)
}

/**
 * @brief Affiche une image OpenCV brute dans cette fenêtre sans traitement supplémentaire.
 * @param img L'image OpenCV brute à afficher.
 */
void ImageWindow::showRawImage(const cv::Mat& img) {
    if (img.empty()) {
        qDebug() << "ImageWindow::showRawImage: L'image fournie est vide.";
        m_currentProcessedImage = cv::Mat();
        return;
    }
    m_currentProcessedImage = img.clone(); // Stocke l'image brute comme image actuelle
    setImage(cvMatToQPixmap(m_currentProcessedImage)); // Affiche l'image brute
}

/**
 * @brief Affiche une QPixmap brute dans cette fenêtre sans traitement supplémentaire.
 * @param pixmap Le QPixmap à afficher.
 */
void ImageWindow::showPixmap(const QPixmap& pixmap) {
    // Réutilise la méthode setImage qui fait déjà le redimensionnement et l'affichage
    setImage(pixmap);
}


// Slots publics pour définir les paramètres du pipeline de traitement
void ImageWindow::setBlurKsize(int value)        { m_blurKsize = value; updateImageProcessing(); }
void ImageWindow::setSigmaX(int value)           { m_sigmaX = value; updateImageProcessing(); }
void ImageWindow::setClaheClipLimit(int value)   { m_claheClipLimit = value; updateImageProcessing(); }
void ImageWindow::setSeparationKsize(int value)  { m_separationKsize = value; updateImageProcessing(); }
void ImageWindow::setFillHolesKsize(int value)   { m_fillHolesKsize = value; updateImageProcessing(); }
void ImageWindow::setContourMinArea(int value)   { m_contourMinArea = value; updateImageProcessing(); }

/**
 * @brief Segmente une image en niveaux de gris en utilisant le seuillage adaptatif.
 * Choisit entre THRESH_BINARY et THRESH_BINARY_INV en comparant le nombre de contours.
 * @param img_gray Image d'entrée en niveaux de gris.
 * @param thresholded Image de sortie seuillée.
 * @param inverted Booléen indiquant si le seuillage a été inversé.
 */
void ImageWindow::segmentByAdaptiveThresholding(const Mat& img_gray, Mat& thresholded, bool& inverted) {
    Mat thresh_binary, thresh_binary_inv;
    adaptiveThreshold(img_gray, thresh_binary, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 15, 10);
    adaptiveThreshold(img_gray, thresh_binary_inv, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 15, 10);

    vector<vector<Point>> contours_bin, contours_inv;
    vector<Vec4i> hierarchy;

    findContours(thresh_binary, contours_bin, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    findContours(thresh_binary_inv, contours_inv, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    if (contours_bin.size() > contours_inv.size()) {
        thresholded = thresh_binary;
        inverted = false;
    } else {
        thresholded = thresh_binary_inv;
        inverted = true;
    }
}

/**
 * @brief Segmente une image en niveaux de gris en utilisant le seuillage global (fixe et Otsu).
 * @param img_gray Image d'entrée en niveaux de gris.
 * @param simple_thresholded Image de sortie seuillée avec un seuil fixe.
 * @param otsu_thresholded Image de sortie seuillée avec la méthode d'Otsu.
 * @param otsu_thresh Valeur du seuil calculée par la méthode d'Otsu.
 */
void ImageWindow::segmentByGlobalThresholding(const Mat& img_gray, Mat& simple_thresholded, Mat& otsu_thresholded, double& otsu_thresh) {
    threshold(img_gray, simple_thresholded, 117, 255, THRESH_BINARY);
    otsu_thresh = threshold(img_gray, otsu_thresholded, 0, 255, THRESH_BINARY + THRESH_OTSU);
}

/**
 * @brief Lance le pipeline complet de traitement d'image et de détection de composants.
 * Applique diverses opérations (flou, CLAHE, seuillage, morphologie, détection de contours)
 * et prépare les résultats pour l'émission via les signaux Qt.
 */
void ImageWindow::updateImageProcessing() {
    if (m_originalImage.empty()) {
        qDebug() << "ImageWindow::updateImageProcessing: L'image originale est vide. Impossible de traiter.";
        return;
    }

    Mat img_color_processed = m_originalImage.clone();
    Mat img_gray_processed;
    cvtColor(img_color_processed, img_gray_processed, COLOR_BGR2GRAY);

    // 1. Application du Flou Gaussien
    int ksize_val = m_blurKsize * 2 + 1;
    double sigmaX_val = m_sigmaX / 10.0;
    if (sigmaX_val < 0.1) sigmaX_val = 0.1;
    if (ksize_val > 0) { // S'assurer que ksize_val est valide
        GaussianBlur(img_gray_processed, img_gray_processed, Size(ksize_val, ksize_val), sigmaX_val);
    }

    // 2. Amélioration du Contraste avec CLAHE
    Ptr<CLAHE> clahe = createCLAHE();
    clahe->setClipLimit(m_claheClipLimit / 10.0);
    clahe->apply(img_gray_processed, img_gray_processed);

    Scalar mean_val = mean(img_gray_processed);

    Mat main_thresholded_binary;
    bool inverted_main_threshold = false;
    double otsu_thresh_dummy;

    if (mean_val[0] > 140) {
        segmentByAdaptiveThresholding(img_gray_processed, main_thresholded_binary, inverted_main_threshold);
    } else {
        Mat simple_thresh_dummy;
        segmentByGlobalThresholding(img_gray_processed, simple_thresh_dummy, main_thresholded_binary, otsu_thresh_dummy);
    }

    // Détection des zones sombres (composants noirs) dans l'espace couleur HSV
    Mat img_hsv;
    cvtColor(img_color_processed, img_hsv, COLOR_BGR2HSV);
    Mat mask_black_areas;
    inRange(img_hsv, Scalar(0, 0, 0), Scalar(180, 255, 40), mask_black_areas);
    Mat kernel_ellipse_5x5 = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
    morphologyEx(mask_black_areas, mask_black_areas, MORPH_CLOSE, kernel_ellipse_5x5, Point(-1, -1), 3);

    Mat combined_binary_mask;
    bitwise_or(main_thresholded_binary, mask_black_areas, combined_binary_mask);

    // --- APPLICATION DES OPÉRATIONS MORPHOLOGIQUES ---
    int separation_ksize = m_separationKsize * 2 + 1;
    int fill_holes_ksize = m_fillHolesKsize * 2 + 1;

    Mat kernel_separation = getStructuringElement(MORPH_RECT, Size(separation_ksize, separation_ksize));
    Mat kernel_fill_holes = getStructuringElement(MORPH_RECT, Size(fill_holes_ksize, fill_holes_ksize));

    if (fill_holes_ksize > 1) {
        morphologyEx(combined_binary_mask, combined_binary_mask, cv::MORPH_CLOSE, kernel_fill_holes, cv::Point(-1, -1), 1);
    }
    if (separation_ksize > 1) {
        morphologyEx(combined_binary_mask, combined_binary_mask, cv::MORPH_OPEN, kernel_separation, cv::Point(-1, -1), 1);
    }

    // Détection finale des contours sur le masque binaire traité
    vector<vector<Point>> final_contours;
    vector<Vec4i> hierarchy_final;
    findContours(combined_binary_mask, final_contours, hierarchy_final, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // Initialisation des images de sortie
    m_processedContoursImage = m_originalImage.clone(); // Image avec contours et BBoxes
    m_extractedComponentsOnBlank = Mat(m_originalImage.size(), m_originalImage.type(), Scalar(255, 255, 255)); // Composants extraits sur fond blanc

    QList<Composant> detectedComponents; // Liste des composants à émettre

    string output_folder = "extracted_components";
    fs::create_directories(output_folder); // Crée le répertoire s'il n'existe pas

    int index = 0;
    for (const auto& contour : final_contours) {
        double area = contourArea(contour);
        if (area > m_contourMinArea) { // Filtrer par aire minimale
            Rect box = boundingRect(contour); // Boîte englobante

            // Assurez-vous que la boîte est dans les limites de l'image
            if (box.x >= 0 && box.y >= 0 && box.width > 0 && box.height > 0 &&
                box.x + box.width <= m_originalImage.cols &&
                box.y + box.height <= m_originalImage.rows) {

                // Dessine le rectangle rouge et le numéro du composant sur l'image des contours
                rectangle(m_processedContoursImage, box, Scalar(0, 0, 255), 2); // Rouge, épaisseur 2
                putText(m_processedContoursImage, to_string(index), box.tl(), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0, 255, 0), 1); // Vert

                // Extrait l'image du composant de l'originale
                Mat component_roi = m_originalImage(box); // Région d'intérêt
                // Sauvegarde l'image du composant individuellement
                string component_filename = output_folder + "/component_" + to_string(index) + ".png";
                imwrite(component_filename, component_roi);

                // Convertit l'image du composant en QPixmap pour Composant
                QPixmap pixmap_component = cvMatToQPixmap(component_roi);

                // Crée un objet Composant et l'ajoute à la liste
                detectedComponents.append(Composant(index, box, area, pixmap_component));

                // Copie le composant extrait sur l'image des composants extraits sur fond blanc
                // Vérifie si component_roi est compatible avec la région de destination
                if(component_roi.channels() == m_extractedComponentsOnBlank.channels() &&
                    component_roi.type() == m_extractedComponentsOnBlank.type()) {
                    component_roi.copyTo(m_extractedComponentsOnBlank(box));
                } else {
                    // Si les types ne correspondent pas, convertissez-les
                    cv::Mat temp_component_roi;
                    component_roi.convertTo(temp_component_roi, m_extractedComponentsOnBlank.type());
                    cv::cvtColor(temp_component_roi, temp_component_roi, cv::COLOR_BGR2BGRA); // Exemple de conversion si nécessaire
                    temp_component_roi.copyTo(m_extractedComponentsOnBlank(box));
                }

                index++;
            }
        }
    }

    // Met à jour l'affichage de l'image principale de cette fenêtre ImageWindow (si elle est visible)
    // C'est m_currentProcessedImage qui est affichée par la méthode setImage()
    m_currentProcessedImage = m_processedContoursImage; // Mettre à jour l'image interne de la fenêtre
    setImage(cvMatToQPixmap(m_currentProcessedImage)); // Affiche le résultat final dans ImageWindow

    // Émet les signaux pour notifier MainWindow
    emit imageProcessed(cvMatToQPixmap(m_processedContoursImage)); // Signal avec l'image des contours (pour labelImage_contours_2)
    emit extractedComponentsImageReady(cvMatToQPixmap(m_extractedComponentsOnBlank)); // Signal avec l'image des composants extraits (pour labelResult_2)
    emit componentsDetected(detectedComponents); // Signal avec la liste des composants (pour listWidgetComponents et label_3)
}

/**
 * @brief Retourne l'image en niveaux de gris prétraitée (le "masque").
 * @return L'image prétraitée (cv::Mat). Retourne une Mat vide si le masque n'a pas été généré.
 */
cv::Mat ImageWindow::getPreprocessedGray() const {
    return m_preprocessedMaskImage.clone(); // Retourne une copie de l'image stockée après setmaskImage
}

/**
 * @brief Retourne l'image avec les contours de composants et les boîtes englobantes.
 * @return L'image annotée (cv::Mat). Retourne une Mat vide si le traitement n'a pas été effectué.
 */
cv::Mat ImageWindow::getContoursImage() const {
    return m_processedContoursImage.clone(); // Retourne une copie de l'image avec les annotations
}

/**
 * @brief Slot pour sauvegarder l'image actuellement affichée dans cette ImageWindow.
 */
void ImageWindow::saveImage()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Enregistrer l'image affichée", "", "Images PNG (*.png);;Images JPG (*.jpg);;Tous les fichiers (*)");
    if (!fileName.isEmpty()) {
        if (!m_currentProcessedImage.empty()) { // Sauvegarde l'image actuellement affichée dans cette fenêtre
            if (cv::imwrite(fileName.toStdString(), m_currentProcessedImage)) {
                QMessageBox::information(this, "Sauvegarde réussie", "L'image a été sauvegardée avec succès à : " + fileName);
            } else {
                QMessageBox::critical(this, "Erreur de sauvegarde", "Impossible de sauvegarder l'image à : " + fileName);
            }
        } else {
            QMessageBox::warning(this, "Image vide", "Il n'y a pas d'image à sauvegarder.");
        }
    }
}
