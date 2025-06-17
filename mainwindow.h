// mainwindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include "drawingwindow.h" // ADD THIS LINE: Include our new drawing window class
#include"imagewindow.h"
#include<QListWidget>
#include<QLabel>
#include<QMessageBox>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// Définition des placeholders pour les QLabels d'images
#define PLACEHOLDER_ORIGINAL_IMAGE "Original Image\n(Click to select)"
#define PLACEHOLDER_MASK_IMAGE "Preprocessed Image\n(Click 'Grayscale' or image to display)"
#define PLACEHOLDER_CONTOURS_IMAGE "Contours Image\n(Click 'Show Edges' or image to display)"


class Composant; // Déclaration anticipée pour éviter l'inclusion circulaire si Composant est utilisé dans l'en-tête

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override; // Surcharge de l'événement de redimensionnement

private slots:
    void loadImageFromFile(); // Slot pour charger une image
    void updateSliderValue1(int value); // Slots pour les sliders
    void updateSliderValue2(int value);
    void updateSliderValue3(int value);
    void updateSliderValue4(int value);
    void updateSliderValue5(int value);
    void updateSliderValue6(int value);
    void updateComponentsView(); // Slot pour mettre à jour l'affichage des composants

    // Slots pour l'affichage des résultats du traitement par ImageWindow
    void displayContoursImage(const QPixmap& resultPixmap);
    void displayExtractedComponentsImage(const QPixmap& extractedComponentsPixmap);
    void displayDetectedComponentsInList(const QList<Composant>& components);

    void showExtractedComponentsImageAndList(); // Nouveau slot pour le bouton "TraitementButton_2"
    void clearProcessedImageDisplays(); // Slot pour effacer les affichages
    void onOpenDrawingWindow(); // ADD THIS LINE: New slot for opening the drawing window

private:
    Ui::MainWindow *ui; // Pointeur vers l'interface utilisateur générée par Qt Designer
    cv::Mat image; // L'image OpenCV originale chargée (votre variable 'image' est ici)
    ImageWindow *maskWindow; // Fenêtre pour le masque/image pré-traitée
    ImageWindow *resultWindow; // Fenêtre pour les résultats complets (contours, composants)

    QListWidget *m_componentListWidget; // Pointeur vers le QListWidget pour la liste des composants
    QLabel *m_componentCountLabel; // Pointeur vers le QLabel pour le compte des composants

    QPixmap m_lastExtractedComponentsPixmap; // Stocke le dernier pixmap des composants extraits
    QList<Composant> m_lastDetectedComponents; // Stocke la dernière liste de composants détectés

    bool m_displayFullResults; // Flag pour contrôler l'affichage complet des résultats

    // Fonction utilitaire pour afficher des messages temporaires
    void afficherMessage(QWidget *parent, const QString &texte,
                         const QString &titre, QMessageBox::Icon style, int duree);
};

#endif // MAINWINDOW_H
