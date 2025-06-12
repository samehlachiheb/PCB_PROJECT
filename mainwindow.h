// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include <QList>
#include <QPixmap>
#include<QMessageBox>
// Définitions des placeholders (inchangées)
#define PLACEHOLDER_ORIGINAL_IMAGE "Image Originale"
#define PLACEHOLDER_MASK_IMAGE "Masque"
#define PLACEHOLDER_CONTOURS_IMAGE "Contours & Composants"
#define PLACEHOLDER_RESULT_IMAGE "Composants détectés"

// Déclarations anticipées (inchangées)
class ClickableLabel;
class ImageWindow;
class Composant;
class QListWidget;
class QLabel;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void loadImageFromFile();
    void afficherMessage(QWidget *parent, const QString &texte,
                         const QString &titre, QMessageBox::Icon style, int duree);
    void clearProcessedImageDisplays();

    // Slots pour les sliders (value_1 à value_6)
    void updateSliderValue1(int value);
    void updateSliderValue2(int value);
    void updateSliderValue3(int value);
    void updateSliderValue4(int value);
    void updateSliderValue5(int value);
    void updateSliderValue6(int value);

    void updateComponentsView();

    // Slots pour recevoir les images et la liste des composants de ImageWindow
    void displayContoursImage(const QPixmap& resultPixmap);
    //  Ce slot va maintenant stocker la liste et afficher CONDITIONNELLEMENT
    void displayDetectedComponentsInList(const QList<Composant>& components);

    //  Ce slot va maintenant stocker le pixmap et afficher CONDITIONNELLEMENT
    void displayExtractedComponentsImage(const QPixmap& extractedComponentsPixmap);

    // Ce slot est appelé par TraitementButton_2 pour afficher les résultats complets
    void showExtractedComponentsImageAndList(); // Renommé pour plus de clarté

private:
    Ui::MainWindow *ui;
    cv::Mat image;
    ImageWindow *maskWindow;
    ImageWindow *resultWindow;

    QListWidget *m_componentListWidget;
    QLabel *m_componentCountLabel;

    QPixmap m_lastExtractedComponentsPixmap;
    QList<Composant> m_lastDetectedComponents;


    bool m_displayFullResults;


};

#endif // MAINWINDOW_H
