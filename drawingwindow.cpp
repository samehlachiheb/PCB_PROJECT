#include "drawingwindow.h" // Inclut le fichier d'en-tête pour la classe DrawingWindow
#include <QVBoxLayout>     // Inclut la classe QVBoxLayout pour l'organisation verticale des widgets
#include <QHBoxLayout>     // Inclut la classe QHBoxLayout pour l'organisation horizontale des widgets (utilisé pour les boutons)
#include <QPushButton>     // Inclut la classe QPushButton pour créer des boutons
#include <QFileDialog>     // Inclut la classe QFileDialog pour ouvrir des boîtes de dialogue de fichier (par exemple, pour sauvegarder)
#include <QMessageBox>     // Inclut la classe QMessageBox pour afficher des messages d'information ou d'erreur

// Constructeur de la classe DrawingWindow
DrawingWindow::DrawingWindow(QWidget *parent)
    : QMainWindow(parent) { // Initialise la classe de base QMainWindow avec le parent fourni

    // Crée un widget central pour la fenêtre principale
    QWidget *centralWidget = new QWidget(this);
    // Crée un layout vertical pour organiser les widgets du widget central
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Crée une instance de ImageViewer (où l'image et les contours seront affichés)
    imageViewer = new ImageViewer(this);
    // Ajoute l'imageViewer au layout principal, il occupera la majeure partie de l'espace
    mainLayout->addWidget(imageViewer);

    // Crée un layout horizontal pour disposer les boutons côte à côte
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    // Crée le bouton "Save Contours Image"
    saveButton = new QPushButton("Save Contours Image", this);
    // Crée le bouton "Undo Last Contour" (pour annuler le dernier contour dessiné)
    undoButton = new QPushButton("Undo Last Contour", this);
    // Crée le bouton "Clear All Contours" (pour supprimer tous les contours dessinés)
    clearAllButton = new QPushButton("Clear All Contours", this);

    // Ajoute les boutons au layout horizontal
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(undoButton);
    buttonLayout->addWidget(clearAllButton);
    // Ajoute un espace étirable qui poussera les boutons vers la gauche du layout horizontal
    buttonLayout->addStretch();

    // Ajoute le layout des boutons (horizontal) au layout principal (vertical)
    mainLayout->addLayout(buttonLayout);

    // Connecte le signal 'clicked' du bouton 'saveButton' à la fonction 'onSaveContours'
    connect(saveButton, &QPushButton::clicked, this, &DrawingWindow::onSaveContours);
    // Connecte le signal 'clicked' du bouton 'undoButton' à la fonction 'onUndoLastContour'
    connect(undoButton, &QPushButton::clicked, this, &DrawingWindow::onUndoLastContour);
    // Connecte le signal 'clicked' du bouton 'clearAllButton' à la fonction 'onClearAllContours'
    connect(clearAllButton, &QPushButton::clicked, this, &DrawingWindow::onClearAllContours);

    // Définit le widget central de la fenêtre principale
    setCentralWidget(centralWidget);
    // Définit le titre de la fenêtre
    setWindowTitle("Draw Contours");
    // Définit la taille minimale de la fenêtre
    setMinimumSize(800, 600);
}

// Destructeur de la classe DrawingWindow
DrawingWindow::~DrawingWindow() {
    // Les objets enfants (imageViewer, saveButton, etc.) sont automatiquement supprimés par Qt
    // car ils ont 'this' (la fenêtre) comme parent. Aucune suppression explicite n'est nécessaire ici.
}

// Définit l'image originale à afficher dans l'ImageViewer
void DrawingWindow::setOriginalImage(const cv::Mat& image) {
    imageViewer->setImage(image); // Passe l'image à l'ImageViewer
}

// Récupère l'image avec les rectangles dessinés
cv::Mat DrawingWindow::getResultImage() const {
    return imageViewer->getImageWithRectangles(); // Retourne l'image modifiée par l'ImageViewer
}

// Slot appelé lorsque le bouton "Save Contours Image" est cliqué
void DrawingWindow::onSaveContours() {
    // Vérifie si des rectangles ont été dessinés. Si la liste est vide, affiche un message d'information.
    if (imageViewer->getDrawnRectangles().empty()) {
        QMessageBox::information(this, "No Contours", "No rectangles have been drawn yet.");
        return; // Quitte la fonction
    }

    // Ouvre une boîte de dialogue "Enregistrer sous" pour que l'utilisateur choisisse l'emplacement et le nom du fichier
    QString savePath = QFileDialog::getSaveFileName(this, "Save Image with Contours", "pcb_with_drawn_contours.jpg", "Image Files (*.png *.jpg *.jpeg *.bmp)");
    // Vérifie si l'utilisateur a sélectionné un chemin de sauvegarde (n'a pas annulé la boîte de dialogue)
    if (!savePath.isEmpty()) {
        // Récupère l'image actuelle de l'ImageViewer avec les rectangles dessinés
        cv::Mat resultImage = imageViewer->getImageWithRectangles();
        // Vérifie si l'image à sauvegarder n'est pas vide
        if (resultImage.empty()) {
            QMessageBox::critical(this, "Save Error", "The image to save is empty. This might indicate an issue with the original image or drawing process.");
            return; // Quitte la fonction
        }

        // Tente de sauvegarder l'image en utilisant OpenCV (cv::imwrite)
        if (cv::imwrite(savePath.toStdString(), resultImage)) {
            // Si la sauvegarde réussit, affiche un message de succès
            QMessageBox::information(this, "Save Successful", "Image with contours saved to: " + savePath);
        } else {
            // Si la sauvegarde échoue, affiche un message d'erreur
            QMessageBox::critical(this, "Save Error", "Failed to save image with contours. Check file permissions or disk space.");
        }
    }
}

// Slot appelé lorsque le bouton "Undo Last Contour" est cliqué
void DrawingWindow::onUndoLastContour() {
    imageViewer->undoLastRectangle(); // Demande à l'ImageViewer d'annuler le dernier rectangle dessiné
    // Affiche un bref message d'information à l'utilisateur
    QMessageBox::information(this, "Undo", "Last drawn rectangle has been undone.", QMessageBox::Ok, 500);
}

// Slot appelé lorsque le bouton "Clear All Contours" est cliqué
void DrawingWindow::onClearAllContours() {
    // Vérifie si des rectangles sont présents avant de proposer de les effacer
    if (imageViewer->getDrawnRectangles().empty()) {
        QMessageBox::information(this, "Clear All", "There are no rectangles to clear.");
        return;
    }
    // Déclare une variable pour stocker la réponse de l'utilisateur
    QMessageBox::StandardButton reply;
    // Affiche une boîte de dialogue de confirmation à l'utilisateur
    reply = QMessageBox::question(this, "Clear All", "Are you sure you want to clear all drawn rectangles?",
                                  QMessageBox::Yes|QMessageBox::No); // Propose les options Oui et Non
    // Si l'utilisateur clique sur "Oui"
    if (reply == QMessageBox::Yes) {
        imageViewer->clearAllRectangles(); // Demande à l'ImageViewer d'effacer tous les rectangles
        QMessageBox::information(this, "Clear All", "All rectangles cleared successfully!"); // Informe l'utilisateur du succès
    }
}
