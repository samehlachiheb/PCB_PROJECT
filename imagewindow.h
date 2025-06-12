// imagewindow.h
#ifndef IMAGEWINDOW_H
#define IMAGEWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include <QPixmap>
#include <QList>
#include "composant.h" // Incluez Composant.h pour la classe Composant

// Déclaration anticipée de la classe Ui::ImageWindow pour éviter les dépendances circulaires
namespace Ui {
class ImageWindow;
}

/**
 * @brief La classe ImageWindow est une fenêtre utilitaire pour afficher des images
 * brutes, masquées ou traitées. Elle contient également la logique de traitement
 * d'image OpenCV et la détection de composants, et émet des signaux
 * vers MainWindow avec les résultats.
 */
class ImageWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur de ImageWindow.
     * @param parent Pointeur vers le widget parent (généralement MainWindow).
     */
    explicit ImageWindow(QWidget *parent = nullptr);

    /**
     * @brief Destructeur de ImageWindow.
     */
    ~ImageWindow();

    /**
     * @brief Définit l'image originale à traiter et lance le pipeline complet.
     * @param originalImage L'image OpenCV d'entrée.
     */
    void setOriginalImage(const cv::Mat& originalImage);

    /**
     * @brief Définit l'image à afficher dans le QLabel principal de cette fenêtre (interne).
     * @param pixmap L'image QPixmap à afficher.
     */
    void setImage(const QPixmap& pixmap);

    /**
     * @brief Effectue un prétraitement pour créer une "image masquée" (niveaux de gris prétraités).
     * Utilisé pour l'affichage du masque dans MainWindow.
     * @param img L'image d'entrée pour le masquage.
     */
    void setmaskImage(const cv::Mat& img);

    /**
     * @brief Affiche une image OpenCV brute dans cette fenêtre sans traitement supplémentaire.
     * Utilisé pour afficher l'image originale dans une nouvelle fenêtre.
     * @param img L'image OpenCV brute à afficher.
     */
    void showRawImage(const cv::Mat& img);

    /**
     * @brief Affiche une QPixmap brute dans cette fenêtre sans traitement supplémentaire.
     * Utile pour afficher les QPixmaps reçues de MainWindow.
     * @param pixmap Le QPixmap à afficher.
     */
    void showPixmap(const QPixmap& pixmap); // Nouvelle méthode pour afficher un QPixmap


    // Slots publics pour définir les paramètres du pipeline de traitement (connectés aux sliders de MainWindow)
    void setBlurKsize(int value);
    void setSigmaX(int value);
    void setClaheClipLimit(int value);
    void setSeparationKsize(int value);
    void setFillHolesKsize(int value);
    void setContourMinArea(int value);

    /**
     * @brief Lance le pipeline complet de traitement d'image et de détection de composants.
     * Cette fonction est appelée chaque fois qu'un paramètre est modifié ou un traitement est déclenché.
     */
    void updateImageProcessing();

    /**
     * @brief Retourne l'image en niveaux de gris prétraitée (le "masque").
     * @return L'image prétraitée (cv::Mat).
     */
    cv::Mat getPreprocessedGray() const;

    /**
     * @brief Retourne l'image avec les contours de composants et les boîtes englobantes.
     * @return L'image annotée (cv::Mat).
     */
    cv::Mat getContoursImage() const;

signals:
    /**
     * @brief Signal émis lorsque la liste des composants détectés est prête.
     * @param components La liste des objets Composant détectés.
     */
    void componentsDetected(const QList<Composant>& components);

    /**
     * @brief Signal émis lorsque l'image des contours est prête.
     * @param resultPixmap La QPixmap de l'image avec les contours et boîtes.
     */
    void imageProcessed(const QPixmap& resultPixmap);

    /**
     * @brief Signal émis lorsque l'image des composants extraits sur fond blanc est prête.
     * @param extractedComponentsPixmap La QPixmap de l'image.
     */
    void extractedComponentsImageReady(const QPixmap& extractedComponentsPixmap);

private slots:
    /**
     * @brief Slot pour sauvegarder l'image actuellement affichée dans cette fenêtre.
     */
    void saveImage();

private:
    Ui::ImageWindow *ui; // Pointeur vers l'UI générée pour cette fenêtre

    cv::Mat m_originalImage;        // L'image originale non modifiée
    cv::Mat m_currentProcessedImage; // L'image résultante du dernier traitement, affichée dans cette fenêtre
    cv::Mat m_preprocessedMaskImage; // Stocke l'image grise du "masque" pour getPreprocessedGray()
    cv::Mat m_processedContoursImage; // Image avec les contours et BBoxes, émise via imageProcessed
    cv::Mat m_extractedComponentsOnBlank; // Image des composants extraits sur fond blanc, émise via extractedComponentsImageReady

    // Membres pour les paramètres du pipeline de traitement des composants
    int m_blurKsize;
    int m_sigmaX;
    int m_claheClipLimit;
    int m_separationKsize;
    int m_fillHolesKsize;
    int m_contourMinArea;

    // Fonctions de Segmentation (internes au pipeline)
    void segmentByAdaptiveThresholding(const cv::Mat& img_gray, cv::Mat& thresholded, bool& inverted);
    void segmentByGlobalThresholding(const cv::Mat& img_gray, cv::Mat& simple_thresholded, cv::Mat& otsu_thresholded, double& otsu_thresh);

    /**
     * @brief Helper pour convertir une cv::Mat en QPixmap.
     * @param mat La cv::Mat à convertir.
     * @return La QPixmap résultante.
     */
    QPixmap cvMatToQPixmap(const cv::Mat& mat);
};

#endif // IMAGEWINDOW_H
