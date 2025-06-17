// imagewindow.cpp

// Inclusion des en-têtes nécessaires pour la classe ImageWindow et ses fonctionnalités
#include "imagewindow.h"     // L'en-tête de la classe ImageWindow elle-même
#include "ui_imagewindow.h"  // Fichier généré par Qt Designer pour l'interface utilisateur de cette fenêtre
#include <QImage>            // Pour la manipulation d'images dans Qt
#include <QPixmap>           // Pour l'affichage d'images dans les widgets Qt
#include <QMessageBox>       // Pour afficher des boîtes de message d'information ou d'erreur
#include <QFileDialog>       // Pour ouvrir la boîte de dialogue de sélection de fichier (sauvegarde)
#include <QAction>           // Pour créer des actions (par exemple, pour les raccourcis clavier)
#include <QKeySequence>      // Pour définir des raccourcis clavier
#include <QDebug>            // Pour les messages de débogage dans la console
#include <vector>            // Pour std::vector, utilisé notamment pour les contours OpenCV
#include <filesystem>        // Pour les opérations sur les systèmes de fichiers (création de répertoires, C++17)

// Assurez-vous d'inclure les headers OpenCV nécessaires pour les fonctions de traitement d'image
#include <opencv2/imgproc.hpp>   // Contient des fonctions de traitement d'image (cvtColor, GaussianBlur, threshold, findContours, drawContours, morphologyEx, createCLAHE, etc.)
#include <opencv2/highgui.hpp>   // Contient des fonctions pour l'interface graphique (imshow, imwrite, etc.)
#include <opencv2/imgcodecs.hpp> // Contient des fonctions pour la lecture/écriture d'images (imread, imwrite)

// Alias pour le namespace filesystem afin de simplifier son utilisation
namespace fs = std::filesystem;
// Directives using pour éviter de préfixer les fonctions OpenCV et STL avec 'cv::' et 'std::'
using namespace cv;
using namespace std;

/**
 * @brief Constructeur de la classe ImageWindow.
 * Initialise les paramètres de traitement par défaut et configure l'interface utilisateur.
 * @param parent Pointeur vers le widget parent (généralement la MainWindow).
 */
ImageWindow::ImageWindow(QWidget *parent) :
    QMainWindow(parent),          // Appelle le constructeur de la classe de base QMainWindow
    ui(new Ui::ImageWindow),      // Initialise l'objet UI généré par Qt Designer pour cette fenêtre
    // Initialisation des membres de la classe avec des valeurs par défaut pour les paramètres de traitement
    m_blurKsize(1),               // Taille du noyau pour le flou gaussien (sera convertie en 2*N+1)
    m_sigmaX(5),                  // Écart-type en X pour le flou gaussien (sera divisé par 10.0)
    m_claheClipLimit(15),         // Limite de coupure pour l'algorithme CLAHE (sera divisée par 10.0)
    m_separationKsize(3),         // Taille du noyau pour l'opération morphologique d'ouverture (séparation)
    m_fillHolesKsize(1),          // Taille du noyau pour l'opération morphologique de fermeture (remplissage des trous)
    m_contourMinArea(50)          // Aire minimale pour filtrer les contours détectés
{
    ui->setupUi(this); // Configure l'interface utilisateur de cette fenêtre à partir du fichier .ui

    // Configuration du bouton de sauvegarde et de son raccourci clavier
    // Vérifie si le bouton "SaveButton" est bien présent dans le fichier .ui de ImageWindow.
    // Cette vérification est une bonne pratique car `ui` peut ne pas contenir tous les widgets attendus.
    if (ui->SaveButton) {
        ui->SaveButton->setToolTip("Sauvegarder (Ctrl+S)"); // Ajoute une infobulle (tooltip) au bouton
        // Connecte le clic du bouton "SaveButton" au slot `saveImage` de cette classe.
        connect(ui->SaveButton, &QPushButton::clicked, this, &ImageWindow::saveImage);
    } else {
        // Message d'avertissement si le bouton n'est pas trouvé dans l'UI.
        // Le raccourci clavier est configuré séparément et fonctionnera indépendamment de la présence physique du bouton.
        qWarning() << "SaveButton non trouvé dans l'UI de ImageWindow. Le raccourci clavier fonctionnera toujours.";
    }

    // Crée une QAction pour la sauvegarde avec un raccourci clavier (Ctrl+S)
    QAction *saveAction = new QAction(tr("Sauvegarder"), this); // Crée une action avec le texte "Sauvegarder"
    saveAction->setShortcut(QKeySequence("Ctrl+S"));             // Définit le raccourci clavier Ctrl+S
    // Connecte l'action déclenchée (triggered) au slot `saveImage`.
    connect(saveAction, &QAction::triggered, this, &ImageWindow::saveImage);
    this->addAction(saveAction); // Ajoute l'action à la fenêtre principale (permet l'utilisation du raccourci clavier globalement)
}

/**
 * @brief Destructeur de la classe ImageWindow.
 * Libère les ressources allouées dynamiquement, notamment l'objet `ui`.
 */
ImageWindow::~ImageWindow()
{
    delete ui; // Supprime l'objet UI alloué dynamiquement dans le constructeur pour éviter les fuites de mémoire
}

/**
 * @brief Helper pour convertir une cv::Mat en QPixmap.
 * Cette fonction est essentielle pour afficher les images traitées par OpenCV dans les widgets Qt.
 * Elle gère la conversion des images en niveaux de gris ou BGR vers le format RGB attendu par QImage.
 * @param mat La cv::Mat (image OpenCV) à convertir.
 * @return La QPixmap résultante. Retourne un QPixmap nul si la Mat est vide ou a un format non supporté.
 */
QPixmap ImageWindow::cvMatToQPixmap(const cv::Mat& mat) {
    if (mat.empty()) {
        qDebug() << "cvMatToQPixmap: Input Mat is empty.";
        return QPixmap(); // Retourne un QPixmap vide si l'image OpenCV d'entrée est vide
    }

    cv::Mat rgb; // Matrice temporaire pour stocker l'image convertie en format RGB
    // Convertit l'image en RGB si elle est en niveaux de gris (1 canal) ou BGR (3 canaux).
    // Qt s'attend généralement à des images RGB pour le format QImage::Format_RGB888.
    if (mat.channels() == 1) {
        cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB); // Conversion Niveaux de gris vers RGB
    } else if (mat.channels() == 3) {
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB); // Conversion BGR vers RGB (format par défaut d'OpenCV pour les images couleur)
    } else {
        qWarning() << "cvMatToQPixmap: Nombre de canaux non supporté : " << mat.channels();
        return QPixmap(); // Retourne un QPixmap vide pour les formats d'images non gérés
    }

    // Crée une QImage à partir des données RGB de la Mat.
    // Les arguments sont : pointeur vers les données, largeur, hauteur, taille de la ligne en octets (stride), format.
    QImage qimg(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);
    // QPixmap::fromImage fait une copie des données de la QImage, ce qui est sûr pour l'affichage asynchrone dans Qt.
    return QPixmap::fromImage(qimg);
}

/**
 * @brief Définit l'image originale à traiter et lance le pipeline complet de détection de composants.
 * Cette fonction est le point d'entrée principal pour le traitement d'une nouvelle image.
 * Elle clone l'image pour s'assurer que les modifications internes n'affectent pas l'originale passée en paramètre.
 * @param originalImage L'image OpenCV d'entrée (cv::Mat).
 */
void ImageWindow::setOriginalImage(const cv::Mat& originalImage)
{
    m_originalImage = originalImage.clone(); // Clone l'image fournie pour travailler sur une copie et protéger l'originale
    if (!m_originalImage.empty()) {
        updateImageProcessing(); // Lance le pipeline complet de traitement d'image dès que l'image est définie
    } else {
        qDebug() << "ImageWindow::setOriginalImage: L'image fournie est vide.";
    }
}

/**
 * @brief Définit l'image à afficher dans le QLabel principal de cette fenêtre (`ui->label`).
 * Redimensionne le pixmap pour qu'il s'adapte au QLabel tout en conservant les proportions.
 * @param pixmap L'image QPixmap à afficher.
 */
void ImageWindow::setImage(const QPixmap& pixmap) {
    // Vérifie si le pixmap est valide (non nul) et si le QLabel cible (ui->label) existe
    if (!pixmap.isNull() && ui->label) {
        // Redimensionne le pixmap pour s'adapter à la taille actuelle du QLabel
        // Qt::KeepAspectRatio: maintient les proportions de l'image pour éviter la déformation
        // Qt::SmoothTransformation: utilise un algorithme de lissage pour un meilleur rendu visuel après redimensionnement
        QPixmap scaledPixmap = pixmap.scaled(ui->label->size(),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
        ui->label->setPixmap(scaledPixmap); // Définit le pixmap redimensionné sur le QLabel
        ui->label->setAlignment(Qt::AlignCenter); // Centre l'image à l'intérieur du QLabel
    } else if (!ui->label) {
        // Avertissement si le QLabel nommé 'label' n'est pas trouvé dans l'interface utilisateur
        qWarning() << "QLabel 'label' non trouvé dans l'UI de ImageWindow.";
    }
}

/**
 * @brief Effectue un prétraitement sur l'image fournie pour créer un "masque"
 * (image en niveaux de gris prétraitée).
 * Les opérations incluent la conversion en niveaux de gris, le flou gaussien et l'amélioration du contraste avec CLAHE.
 * Le résultat est stocké dans `m_preprocessedMaskImage` et affiché dans cette fenêtre.
 * @param img L'image d'entrée pour le masquage.
 */
void ImageWindow::setmaskImage(const cv::Mat& img) {
    if (img.empty()) {
        qDebug() << "ImageWindow::setmaskImage: L'image fournie est vide.";
        m_preprocessedMaskImage = cv::Mat(); // Assurez-vous que l'image interne est vide si l'entrée est vide
        return;
    }

    cv::Mat processed_gray;
    cv::cvtColor(img, processed_gray, cv::COLOR_BGR2GRAY); // Convertit l'image couleur en niveaux de gris

    // Appliquer le flou gaussien selon les paramètres `m_blurKsize` et `m_sigmaX`.
    // `m_blurKsize` est converti en une taille de noyau impaire (2*N+1) car `GaussianBlur` requiert un noyau impair.
    // `m_sigmaX` est divisé par 10.0 pour permettre des valeurs décimales plus fines via le slider.
    int ksize_val = m_blurKsize * 2 + 1; // Calcul de la taille du noyau (doit être impair)
    double sigmaX_val = m_sigmaX / 10.0; // Conversion de la valeur du slider en sigmaX réel
    if (sigmaX_val < 0.1) sigmaX_val = 0.1; // S'assurer que sigmaX n'est pas trop petit, pour éviter un comportement inattendu ou aucun flou
    if (ksize_val > 0) { // S'assurer que la taille du noyau est valide (positive)
        cv::GaussianBlur(processed_gray, processed_gray, cv::Size(ksize_val, ksize_val), sigmaX_val);
    }

    // Appliquer CLAHE (Contrast Limited Adaptive Histogram Equalization) pour l'amélioration du contraste local.
    Ptr<CLAHE> clahe = createCLAHE(); // Crée un objet CLAHE (pointeur intelligent)
    clahe->setClipLimit(m_claheClipLimit / 10.0); // Définit la limite de coupure (valeur décimale pour le slider)
    clahe->apply(processed_gray, processed_gray); // Applique CLAHE à l'image en niveaux de gris

    m_preprocessedMaskImage = processed_gray; // Stocke l'image prétraitée (le masque) en niveaux de gris
    // Affiche le masque dans cette ImageWindow (utile pour le débogage ou pour visualiser l'étape du masque).
    setImage(cvMatToQPixmap(m_preprocessedMaskImage));
}

/**
 * @brief Affiche une image OpenCV brute dans cette fenêtre sans traitement supplémentaire.
 * Utile pour afficher l'image originale ou une étape intermédiaire sans appliquer le pipeline complet.
 * @param img L'image OpenCV brute à afficher (cv::Mat).
 */
void ImageWindow::showRawImage(const cv::Mat& img) {
    if (img.empty()) {
        qDebug() << "ImageWindow::showRawImage: L'image fournie est vide.";
        m_currentProcessedImage = cv::Mat(); // Efface l'image interne si l'entrée est vide
        return;
    }
    m_currentProcessedImage = img.clone(); // Stocke une copie de l'image brute comme l'image actuellement affichée par cette fenêtre
    setImage(cvMatToQPixmap(m_currentProcessedImage)); // Convertit et affiche l'image brute dans le QLabel
}

/**
 * @brief Affiche une QPixmap brute dans cette fenêtre sans traitement supplémentaire.
 * C'est une surcharge pour afficher directement un QPixmap si disponible, sans passer par la conversion cv::Mat.
 * @param pixmap Le QPixmap à afficher.
 */
void ImageWindow::showPixmap(const QPixmap& pixmap) {
    // Réutilise la méthode `setImage` qui est déjà implémentée et gère le redimensionnement et l'affichage dans `ui->label`.
    setImage(pixmap);
}


// Implémentation des slots publics pour définir les paramètres du pipeline de traitement.
// Chaque setter met à jour le membre correspondant (m_blurKsize, etc.) et déclenche un nouvel appel à `updateImageProcessing()`
// pour re-traiter l'image avec les nouveaux paramètres. Cela permet une mise à jour dynamique de l'affichage.
void ImageWindow::setBlurKsize(int value)       { m_blurKsize = value; updateImageProcessing(); }
void ImageWindow::setSigmaX(int value)          { m_sigmaX = value; updateImageProcessing(); }
void ImageWindow::setClaheClipLimit(int value)  { m_claheClipLimit = value; updateImageProcessing(); }
void ImageWindow::setSeparationKsize(int value) { m_separationKsize = value; updateImageProcessing(); }
void ImageWindow::setFillHolesKsize(int value)  { m_fillHolesKsize = value; updateImageProcessing(); }
void ImageWindow::setContourMinArea(int value)  { m_contourMinArea = value; updateImageProcessing(); }

/**
 * @brief Segmente une image en niveaux de gris en utilisant le seuillage adaptatif.
 * Deux versions du seuillage adaptatif (binaire et binaire inverse) sont appliquées,
 * et la version qui produit le plus de contours est choisie, supposant qu'elle capture mieux les éléments d'intérêt.
 * @param img_gray Image d'entrée en niveaux de gris.
 * @param thresholded Image de sortie seuillée (le résultat sélectionné).
 * @param inverted Booléen de sortie indiquant si le seuillage a été inversé (true si THRESH_BINARY_INV a été choisi).
 */
void ImageWindow::segmentByAdaptiveThresholding(const Mat& img_gray, Mat& thresholded, bool& inverted) {
    Mat thresh_binary, thresh_binary_inv; // Matrices pour stocker les résultats des deux types de seuillage adaptatif
    // Applique le seuillage adaptatif de type MEAN_C (moyenne des pixels voisins)
    // - `img_gray`: image d'entrée
    // - `thresh_binary`: image de sortie binaire (pixels > seuil deviennent 255)
    // - `255`: valeur maximale à attribuer aux pixels qui passent le seuil
    // - `ADAPTIVE_THRESH_MEAN_C`: le seuil est la moyenne des voisins moins une constante
    // - `THRESH_BINARY`: type de seuil binaire
    // - `15`: taille du voisinage (bloc) pour calculer la moyenne (doit être impair)
    // - `10`: constante soustraite de la moyenne (C)
    adaptiveThreshold(img_gray, thresh_binary, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 15, 10);
    // Applique le seuillage adaptatif inverse (pixels > seuil deviennent 0, et inversement)
    adaptiveThreshold(img_gray, thresh_binary_inv, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 15, 10);

    vector<vector<Point>> contours_bin, contours_inv; // Vecteurs pour stocker les contours trouvés
    vector<Vec4i> hierarchy; // Hiérarchie des contours (non utilisée ici directement mais nécessaire pour `findContours`)

    // Trouve les contours sur l'image binaire normale
    // `RETR_EXTERNAL`: récupère uniquement les contours externes (pas les contours des trous)
    // `CHAIN_APPROX_SIMPLE`: compresse les segments de contour horizontaux, verticaux et diagonaux en gardant seulement les points d'extrémité
    findContours(thresh_binary, contours_bin, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    // Trouve les contours sur l'image binaire inversée
    findContours(thresh_binary_inv, contours_inv, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // Compare le nombre de contours trouvés dans chaque version pour choisir la meilleure segmentation.
    // L'idée est que la version qui révèle le plus de contours distincts est probablement plus pertinente.
    if (contours_bin.size() > contours_inv.size()) {
        thresholded = thresh_binary; // Choisit la version binaire si elle a plus de contours
        inverted = false;            // Indique que le seuillage n'a pas été inversé
    } else {
        thresholded = thresh_binary_inv; // Sinon, choisit la version binaire inverse
        inverted = true;             // Indique que le seuillage a été inversé
    }
}

/**
 * @brief Segmente une image en niveaux de gris en utilisant le seuillage global (fixe et Otsu).
 * Applique un seuil fixe et le seuil calculé par la méthode d'Otsu.
 * @param img_gray Image d'entrée en niveaux de gris.
 * @param simple_thresholded Image de sortie seuillée avec un seuil fixe (117).
 * @param otsu_thresholded Image de sortie seuillée avec la méthode d'Otsu.
 * @param otsu_thresh Valeur du seuil calculée par la méthode d'Otsu (sortie).
 */
void ImageWindow::segmentByGlobalThresholding(const Mat& img_gray, Mat& simple_thresholded, Mat& otsu_thresholded, double& otsu_thresh) {
    // Applique un seuil binaire fixe (117)
    // - `img_gray`: image d'entrée
    // - `simple_thresholded`: image de sortie
    // - `117`: valeur de seuil fixe
    // - `255`: valeur maximale à attribuer aux pixels qui passent le seuil
    // - `THRESH_BINARY`: type de seuil binaire
    threshold(img_gray, simple_thresholded, 117, 255, THRESH_BINARY);
    // Applique le seuillage d'Otsu pour trouver un seuil optimal automatiquement.
    // La valeur du seuil calculée par l'algorithme d'Otsu est retournée par la fonction
    // et stockée dans `otsu_thresh`.
    // - `THRESH_BINARY + THRESH_OTSU`: combine le seuillage binaire avec la méthode d'Otsu
    otsu_thresh = threshold(img_gray, otsu_thresholded, 0, 255, THRESH_BINARY + THRESH_OTSU);
}

/**
 * @brief Lance le pipeline complet de traitement d'image et de détection de composants.
 * Cette fonction est le cœur de la logique de traitement d'image.
 * Elle applique diverses opérations (flou, CLAHE, seuillage, morphologie, détection de contours)
 * et prépare les résultats (images et liste de composants) pour l'émission via les signaux Qt.
 */
void ImageWindow::updateImageProcessing() {
    if (m_originalImage.empty()) {
        qDebug() << "ImageWindow::updateImageProcessing: L'image originale est vide. Impossible de traiter.";
        return; // Quitte la fonction si aucune image n'est chargée
    }

    Mat img_color_processed = m_originalImage.clone(); // Crée une copie de l'image originale pour le traitement couleur
    Mat img_gray_processed;
    cv::cvtColor(img_color_processed, img_gray_processed, cv::COLOR_BGR2GRAY); // Convertit l'image couleur en niveaux de gris pour le traitement

    // 1. Application du Flou Gaussien (pour réduire le bruit et lisser l'image)
    // Calcul de la taille du noyau (doit être impaire) et de sigmaX pour le flou.
    int ksize_val = m_blurKsize * 2 + 1; // Taille du noyau (ex: 1 -> 3x3, 2 -> 5x5)
    double sigmaX_val = m_sigmaX / 10.0; // Écart-type pour le flou (valeur décimale plus fine)
    if (sigmaX_val < 0.1) sigmaX_val = 0.1; // S'assurer que sigmaX n'est pas trop petit
    if (ksize_val > 0) { // S'assurer que la taille du noyau est valide et positive
        cv::GaussianBlur(img_gray_processed, img_gray_processed, cv::Size(ksize_val, ksize_val), sigmaX_val);
    }

    // 2. Amélioration du Contraste avec CLAHE (Contrast Limited Adaptive Histogram Equalization)
    // Utile pour améliorer le contraste local dans les zones sombres ou lumineuses de l'image.
    Ptr<CLAHE> clahe = createCLAHE(); // Crée une instance de l'algorithme CLAHE
    clahe->setClipLimit(m_claheClipLimit / 10.0); // Définit la limite de coupure (valeur décimale)
    clahe->apply(img_gray_processed, img_gray_processed); // Applique CLAHE à l'image en niveaux de gris

    // Détermine le type de seuillage à appliquer (adaptatif ou global) en fonction de la luminosité moyenne de l'image
    Scalar mean_val = mean(img_gray_processed); // Calcule la valeur moyenne des pixels de l'image en niveaux de gris

    Mat main_thresholded_binary; // Matrice pour stocker le résultat du seuillage principal (final)
    bool inverted_main_threshold = false; // Drapeau pour savoir si le seuil principal a été inversé
    double otsu_thresh_dummy; // Variable factice pour le seuil d'Otsu (si non utilisé directement)

    if (mean_val[0] > 140) { // Si l'image est globalement lumineuse (valeur moyenne des pixels > 140)
        // Utilise le seuillage adaptatif, car il est plus efficace pour les images avec des variations d'éclairage
        segmentByAdaptiveThresholding(img_gray_processed, main_thresholded_binary, inverted_main_threshold);
    } else { // Si l'image est globalement sombre ou de luminosité moyenne
        // Utilise le seuillage global (méthode d'Otsu), souvent suffisant pour des images avec un contraste global
        Mat simple_thresh_dummy; // Matrice temporaire pour le seuil simple (non utilisée ici directement)
        segmentByGlobalThresholding(img_gray_processed, simple_thresh_dummy, main_thresholded_binary, otsu_thresh_dummy);
    }

    // Détection des zones sombres (composants noirs) dans l'espace couleur HSV
    // Cette étape est complémentaire au seuillage principal et vise à s'assurer que les composants sombres
    // sont bien capturés, même si le seuillage binaire général ne les a pas parfaitement isolés.
    Mat img_hsv;
    cv::cvtColor(img_color_processed, img_hsv, cv::COLOR_BGR2HSV); // Convertit l'image couleur en HSV (Hue, Saturation, Value)
    Mat mask_black_areas;
    // Applique un seuil sur les valeurs HSV pour isoler les pixels "noirs" (valeur V faible)
    // Scalar(0, 0, 0) : limite inférieure HSV (noir pur)
    // Scalar(180, 255, 40) : limite supérieure HSV (la valeur V jusqu'à 40 représente des teintes sombres)
    cv::inRange(img_hsv, Scalar(0, 0, 0), Scalar(180, 255, 40), mask_black_areas);
    Mat kernel_ellipse_5x5 = getStructuringElement(MORPH_ELLIPSE, Size(5, 5)); // Crée un élément structurant elliptique de 5x5
    // Applique une opération de fermeture morphologique (MORPH_CLOSE) pour connecter les petites zones noires adjacentes
    // et remplir les petits trous dans ces zones.
    cv::morphologyEx(mask_black_areas, mask_black_areas, MORPH_CLOSE, kernel_ellipse_5x5, Point(-1, -1), 3);

    // Combine le masque binaire principal (issu du seuillage adaptatif/global) avec le masque des zones noires (OR bit à bit)
    // Cela permet d'inclure tous les objets détectés par l'une ou l'autre des méthodes.
    Mat combined_binary_mask;
    cv::bitwise_or(main_thresholded_binary, mask_black_areas, combined_binary_mask);

    // --- APPLICATION DES OPÉRATIONS MORPHOLOGIQUES FINALES ---
    // Ces opérations (ouverture et fermeture) sont utilisées pour nettoyer le masque binaire :
    // - L'ouverture (MORPH_OPEN) enlève les petits objets isolés (bruit) et sépare les objets connectés par de fins ponts.
    // - La fermeture (MORPH_CLOSE) remplit les petits trous à l'intérieur des objets et ferme les petites brèches.
    // Les tailles des noyaux sont dérivées des paramètres des sliders (m_separationKsize, m_fillHolesKsize).
    // Les tailles doivent être impaires pour les noyaux morphologiques.
    int separation_ksize = m_separationKsize * 2 + 1; // Taille du noyau pour l'ouverture (séparation)
    int fill_holes_ksize = m_fillHolesKsize * 2 + 1;  // Taille du noyau pour la fermeture (remplissage des trous)

    // Crée les éléments structurants (noyaux) pour les opérations morphologiques
    Mat kernel_separation = getStructuringElement(MORPH_RECT, Size(separation_ksize, separation_ksize)); // Noyau rectangulaire
    Mat kernel_fill_holes = getStructuringElement(MORPH_RECT, Size(fill_holes_ksize, fill_holes_ksize)); // Noyau rectangulaire

    // Application de la fermeture pour remplir les petits trous ou connexions.
    if (fill_holes_ksize > 1) { // Applique l'opération seulement si la taille du noyau est supérieure à 1 (pour avoir un effet)
        cv::morphologyEx(combined_binary_mask, combined_binary_mask, cv::MORPH_CLOSE, kernel_fill_holes, cv::Point(-1, -1), 1);
    }
    // Application de l'ouverture pour séparer les objets connectés par de petits ponts ou enlever du bruit.
    if (separation_ksize > 1) { // Applique l'opération seulement si la taille du noyau est supérieure à 1
        cv::morphologyEx(combined_binary_mask, combined_binary_mask, cv::MORPH_OPEN, kernel_separation, cv::Point(-1, -1), 1);
    }

    // Détection finale des contours sur le masque binaire traité et nettoyé
    vector<vector<Point>> final_contours; // Vecteur pour stocker les contours détectés
    vector<Vec4i> hierarchy_final;      // Hiérarchie des contours

    // `cv::findContours`: trouve les contours dans l'image binaire
    // `RETR_EXTERNAL`: récupère uniquement les contours externes (utile pour les objets, pas les trous internes)
    // `CHAIN_APPROX_SIMPLE`: compresse les segments de contour, réduisant le nombre de points pour économiser de la mémoire
    cv::findContours(combined_binary_mask, final_contours, hierarchy_final, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // Initialisation des images de sortie
    // `m_processedContoursImage` affichera l'image originale avec les contours et boîtes englobantes dessinés.
    m_processedContoursImage = m_originalImage.clone(); // Clone l'image originale pour dessiner les annotations
    // `m_extractedComponentsOnBlank` est une image blanche sur laquelle les composants détectés seront copiés.
    m_extractedComponentsOnBlank = Mat(m_originalImage.size(), m_originalImage.type(), Scalar(255, 255, 255)); // Crée une image blanche de même taille et type que l'originale

    QList<Composant> detectedComponents; // Liste Qt pour stocker les objets `Composant` détectés (métadonnées et petite image)

    // Crée un répertoire pour sauvegarder les images individuelles des composants extraits
    string output_folder = "extracted_components";
    // `fs::create_directories`: crée le répertoire et tous les répertoires parents nécessaires s'ils n'existent pas (nécessite C++17)
    fs::create_directories(output_folder);

    int index = 0; // Compteur pour assigner un ID unique à chaque composant détecté
    // Itère sur tous les contours finaux détectés par `findContours`
    for (const auto& contour : final_contours) {
        double area = contourArea(contour); // Calcule l'aire du contour (nombre de pixels qu'il englobe)
        if (area > m_contourMinArea) { // Filtre les contours par aire minimale (les petits bruits sont éliminés)
            Rect box = boundingRect(contour); // Calcule la boîte englobante rectangulaire minimale autour du contour

            // S'assurer que la boîte englobante est entièrement contenue dans les limites de l'image originale.
            // Cela est crucial pour éviter les erreurs d'accès mémoire (segmentation fault) lors de l'extraction de ROI.
            if (box.x >= 0 && box.y >= 0 && box.width > 0 && box.height > 0 &&
                box.x + box.width <= m_originalImage.cols &&
                box.y + box.height <= m_originalImage.rows) {

                // Dessine le rectangle rouge et le numéro du composant sur l'image des contours traités (`m_processedContoursImage`)
                cv::rectangle(m_processedContoursImage, box, Scalar(0, 0, 255), 2); // Dessine un rectangle rouge (BGR) avec une épaisseur de 2 pixels
                cv::putText(m_processedContoursImage, to_string(index), box.tl(), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0, 255, 0), 1); // Ajoute le numéro du composant en vert, à l'origine du rectangle

                // Extrait l'image du composant de l'image originale en utilisant la région d'intérêt (ROI) définie par `box`
                Mat component_roi = m_originalImage(box);
                // Sauvegarde l'image du composant individuellement dans le répertoire `extracted_components`
                string component_filename = output_folder + "/component_" + to_string(index) + ".png";
                cv::imwrite(component_filename, component_roi); // Sauvegarde au format PNG

                // Convertit l'image du composant (ROI) en QPixmap pour la stocker dans l'objet `Composant`
                QPixmap pixmap_component = cvMatToQPixmap(component_roi);

                // Crée un nouvel objet `Composant` avec son ID, sa boîte englobante, son aire et sa petite image.
                // Cet objet sera ajouté à la liste des composants détectés.
                detectedComponents.append(Composant(index, box, area, pixmap_component));

                // Copie le composant extrait sur l'image des composants extraits sur fond blanc (`m_extractedComponentsOnBlank`).
                // Cela permet de visualiser tous les composants extraits sur une seule image.
                // Vérifie la compatibilité des types et canaux avant de copier pour éviter les erreurs.
                if(component_roi.channels() == m_extractedComponentsOnBlank.channels() &&
                    component_roi.type() == m_extractedComponentsOnBlank.type()) {
                    component_roi.copyTo(m_extractedComponentsOnBlank(box)); // Copie directement la ROI
                } else {
                    // Si les types ou canaux ne correspondent pas (ce qui devrait être rare si les images de base sont bien gérées),
                    // une conversion est tentée.
                    cv::Mat temp_component_roi;
                    component_roi.convertTo(temp_component_roi, m_extractedComponentsOnBlank.type()); // Convertit le type
                    // Si les canaux ne correspondent pas non plus (ex: si le fond blanc est BGRA et le composant est BGR)
                    if(temp_component_roi.channels() != m_extractedComponentsOnBlank.channels()){
                        cv::cvtColor(temp_component_roi, temp_component_roi, cv::COLOR_BGR2BGRA); // Exemple: conversion de BGR vers BGRA
                    }
                    temp_component_roi.copyTo(m_extractedComponentsOnBlank(box)); // Copie la ROI convertie
                }

                index++; // Incrémente le compteur de composants pour le prochain ID
            }
        }
    }

    // Met à jour l'affichage de l'image principale de cette fenêtre ImageWindow (si elle est visible).
    // `m_currentProcessedImage` est la Matrice qui représente ce que cette fenêtre est censée afficher.
    m_currentProcessedImage = m_processedContoursImage; // Définit l'image des contours comme l'image principale à afficher
    setImage(cvMatToQPixmap(m_currentProcessedImage)); // Convertit cette image en QPixmap et l'affiche dans le QLabel de la fenêtre

    // Émet les signaux pour notifier la MainWindow (le parent) des résultats du traitement.
    // Ces signaux permettent à MainWindow de mettre à jour ses propres QLabel et QListWidget avec les images et la liste de composants.
    emit imageProcessed(cvMatToQPixmap(m_processedContoursImage)); // Signal avec l'image des contours (pour un autre QLabel dans MainWindow)
    emit extractedComponentsImageReady(cvMatToQPixmap(m_extractedComponentsOnBlank)); // Signal avec l'image des composants extraits sur fond blanc (pour un autre QLabel dans MainWindow)
    emit componentsDetected(detectedComponents); // Signal avec la liste des objets Composant détectés (pour un QListWidget dans MainWindow)
}

/**
 * @brief Retourne l'image en niveaux de gris prétraitée .
 * Cette fonction est utile pour permettre à la MainWindow d'afficher le masque généré à une autre étape.
 * @return L'image prétraitée (cv::Mat). Retourne une Mat vide si le masque n'a pas été généré (par `setmaskImage`).
 */
cv::Mat ImageWindow::getPreprocessedGray() const {
    return m_preprocessedMaskImage.clone(); // Retourne une copie de l'image prétraitée pour éviter les modifications externes
}

/**
 * @brief Retourne l'image avec les contours de composants et les boîtes englobantes dessinés.
 * Cette fonction est utile pour permettre à la MainWindow d'afficher cette étape du traitement.
 * @return L'image annotée (cv::Mat). Retourne une Mat vide si le traitement n'a pas été effectué.
 */
cv::Mat ImageWindow::getContoursImage() const {
    return m_processedContoursImage.clone(); // Retourne une copie de l'image avec les annotations (contours et rectangles)
}

/**
 * @brief Slot pour sauvegarder l'image actuellement affichée dans cette ImageWindow.
 * Ouvre une boîte de dialogue "Enregistrer sous" pour permettre à l'utilisateur de choisir le nom et le format du fichier.
 */
void ImageWindow::saveImage()
{
    // Ouvre une boîte de dialogue pour sélectionner le nom du fichier de sauvegarde.
    // Le premier argument est le parent, le deuxième est le titre de la boîte de dialogue.
    // Le troisième est le chemin initial/nom de fichier suggéré, le quatrième sont les filtres de fichiers.
    QString fileName = QFileDialog::getSaveFileName(this, "Enregistrer l'image affichée", "", "Images PNG (*.png);;Images JPG (*.jpg);;Tous les fichiers (*)");
    if (!fileName.isEmpty()) { // Si l'utilisateur a sélectionné un nom de fichier (n'a pas annulé la boîte de dialogue)
        if (!m_currentProcessedImage.empty()) { // Vérifie si une image est actuellement chargée et affichée dans cette fenêtre
            // Tente de sauvegarder l'image OpenCV. `fileName.toStdString()` convertit QString en std::string.
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
cv::Mat ImageWindow::getExtractedComponentsOnBlankMat() const {
    return m_extractedComponentsOnBlank.clone();
}
