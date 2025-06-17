#include "imageviewer.h" // Inclut le fichier d'en-tête de la classe ImageViewer
#include <QDebug>          // Inclut la classe QDebug pour les messages de débogage (par exemple, qWarning, qDebug)
#include <QPainter>        // Inclut la classe QPainter pour dessiner sur le widget
#include <QMouseEvent>     // Inclut les événements de souris
#include <QCursor>         // Inclut la classe QCursor pour obtenir la position du curseur
#include <QSize>           // Inclut la classe QSize pour gérer les dimensions
#include <QtMath>          // Inclut des fonctions mathématiques de Qt (comme qBound)

// Constructeur de la classe ImageViewer
// Initialise le widget avec un parent, et met le drapeau 'drawing' à false (pas de dessin en cours)
ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent), drawing(false) {
    // Définit la taille minimale du widget pour assurer une visibilité de base
    setMinimumSize(400, 300);
    // Active le suivi de la souris même sans bouton enfoncé, nécessaire pour dessiner en temps réel
    setMouseTracking(true);
}

// Définit l'image originale à afficher dans l'ImageViewer
void ImageViewer::setImage(const cv::Mat& image) {
    // Clone l'image OpenCV fournie pour ne pas modifier l'originale extérieurement
    original_image_cv = image.clone();
    // Vérifie si l'image n'est pas vide
    if (!original_image_cv.empty()) {
        // Convertit l'image OpenCV du format BGR (utilisé par OpenCV par défaut) en RGB (préféré par Qt)
        cv::cvtColor(original_image_cv, original_image_cv, cv::COLOR_BGR2RGB);
        // Convertit l'image OpenCV (cv::Mat) en QImage pour l'affichage avec Qt
        current_image_qt = cvMatToQImage(original_image_cv);
        // Efface tous les rectangles déjà dessinés lorsque une nouvelle image est chargée
        drawn_rectangles.clear();
        // Vide l'historique d'annulation pour la nouvelle image
        while (!undo_history.empty()) {
            undo_history.pop();
        }
        // Force le rafraîchissement du widget, ce qui déclenchera paintEvent
        update();
    } else {
        // Affiche un avertissement si l'image fournie est vide
        qWarning() << "ImageViewer received empty image.";
        // Efface la QImage si l'image OpenCV est vide
        current_image_qt = QImage();
        // Efface les rectangles et l'historique dans ce cas également
        drawn_rectangles.clear();
        while (!undo_history.empty()) {
            undo_history.pop();
        }
        // Rafraîchit l'affichage
        update();
    }
}

// Convertit une image OpenCV (cv::Mat) en QImage
QImage ImageViewer::cvMatToQImage(const cv::Mat &mat) {
    // Gère le cas des images couleur (3 canaux, 8 bits par canal, non signés)
    if (mat.type() == CV_8UC3) {
        // Crée une QImage directement à partir des données de la cv::Mat
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
    }
    // Gère le cas des images en niveaux de gris (1 canal, 8 bits par canal, non signés)
    else if (mat.type() == CV_8UC1) {
        // Crée une QImage à partir des données de la cv::Mat pour le format niveaux de gris
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
    }
    // Gère les types d'images non supportés
    else {
        qWarning("Unsupported cv::Mat type for QImage conversion");
        return QImage(); // Retourne une QImage nulle
    }
}

// Retourne l'image originale avec tous les rectangles dessinés par l'utilisateur
cv::Mat ImageViewer::getImageWithRectangles() const {
    // Vérifie si l'image originale est vide
    if (original_image_cv.empty()) {
        qWarning() << "Original image is empty in ImageViewer, cannot get image with rectangles.";
        return cv::Mat(); // Retourne une image OpenCV vide
    }
    // Clone l'image originale pour y dessiner les rectangles sans la modifier directement
    cv::Mat image_with_rects = original_image_cv.clone();
    // Reconvertit l'image de RGB à BGR avant de dessiner avec OpenCV et de la retourner
    cv::cvtColor(image_with_rects, image_with_rects, cv::COLOR_RGB2BGR);

    // Parcourt tous les rectangles stockés
    for (const auto& rect : drawn_rectangles) {
        // Dessine chaque rectangle sur l'image avec une couleur bleue (0,0,255) et une épaisseur de 2
        cv::rectangle(image_with_rects, rect, cv::Scalar(0, 0, 255), 2);
    }
    return image_with_rects; // Retourne l'image avec les rectangles dessinés
}

// Gère l'événement de peinture, responsable du dessin de l'image et des rectangles
void ImageViewer::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event); // Marque l'argument 'event' comme inutilisé pour éviter les avertissements de compilation
    QPainter painter(this); // Crée un objet QPainter pour dessiner sur ce widget

    // Si une image Qt est disponible (non nulle)
    if (!current_image_qt.isNull()) {
        QSize widgetSize = size(); // Obtient la taille actuelle du widget
        // Redimensionne l'image pour qu'elle s'adapte au widget, en conservant le ratio et en lissant
        QImage scaledImage = current_image_qt.scaled(widgetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // Calcule les décalages (offsets) pour centrer l'image dans le widget
        int x_offset = (widgetSize.width() - scaledImage.width()) / 2;
        int y_offset = (widgetSize.height() - scaledImage.height()) / 2;

        // Dessine l'image redimensionnée et centrée sur le widget
        painter.drawImage(x_offset, y_offset, scaledImage);

        // Dessine les rectangles déjà tracés
        for (const auto& rect_cv : drawn_rectangles) {
            // Calcule les facteurs d'échelle pour convertir les coordonnées de l'image originale
            // aux coordonnées de l'image redimensionnée affichée
            double scale_w = (double)scaledImage.width() / original_image_cv.cols;
            double scale_h = (double)scaledImage.height() / original_image_cv.rows;

            // Convertit les coordonnées et la taille du rectangle de l'espace de l'image originale
            // à l'espace de l'image affichée (en tenant compte du centrage)
            int rect_x = x_offset + static_cast<int>(rect_cv.x * scale_w);
            int rect_y = y_offset + static_cast<int>(rect_cv.y * scale_h);
            int rect_width = static_cast<int>(rect_cv.width * scale_w);
            int rect_height = static_cast<int>(rect_cv.height * scale_h);

            // Définit le stylo pour dessiner le rectangle (rouge, épaisseur 2)
            painter.setPen(QPen(Qt::red, 2));
            // Dessine le rectangle
            painter.drawRect(rect_x, rect_y, rect_width, rect_height);
        }

        // Si l'utilisateur est en train de dessiner un nouveau rectangle
        if (drawing) {
            QPoint current_mouse_pos = mapFromGlobal(QCursor::pos()); // Obtient la position actuelle de la souris (globale puis convertie en locale)

            // Recalcule les facteurs d'échelle (identiques à ceux utilisés pour les rectangles existants)
            double scale_w = (double)scaledImage.width() / original_image_cv.cols;
            double scale_h = (double)scaledImage.height() / original_image_cv.rows;

            // Convertit le point de départ du dessin (qui est en coordonnées de l'image originale)
            // en coordonnées de l'image affichée
            int draw_start_x_scaled = x_offset + static_cast<int>(start_point.x() * scale_w);
            int draw_start_y_scaled = y_offset + static_cast<int>(start_point.y() * scale_h);

            // Coordonnées actuelles de la souris, déjà en coordonnées du widget
            int draw_current_x_scaled = current_mouse_pos.x();
            int draw_current_y_scaled = current_mouse_pos.y();

            // Limite les coordonnées actuelles de la souris à l'intérieur des bords de l'image affichée
            draw_current_x_scaled = qBound(x_offset, draw_current_x_scaled, x_offset + scaledImage.width());
            draw_current_y_scaled = qBound(y_offset, draw_current_y_scaled, y_offset + scaledImage.height());

            // Définit le stylo pour dessiner le rectangle en cours (rouge, épaisseur 2)
            painter.setPen(QPen(Qt::red, 2));
            // Dessine le rectangle temporaire pendant le dessin
            painter.drawRect(QRect(QPoint(draw_start_x_scaled, draw_start_y_scaled), QPoint(draw_current_x_scaled, draw_current_y_scaled)));
        }
    }
}

// Gère l'événement de pression du bouton de la souris
void ImageViewer::mousePressEvent(QMouseEvent *event) {
    // Vérifie si le bouton gauche de la souris est pressé et si une image est chargée
    if (event->button() == Qt::LeftButton && !original_image_cv.empty()) {
        QPoint mouse_pos_widget = event->pos(); // Obtient la position de la souris relative au widget

        QSize widgetSize = size(); // Obtient la taille actuelle du widget
        QImage scaledImage = current_image_qt.scaled(widgetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int offset_x = (widgetSize.width() - scaledImage.width()) / 2; // Offset X de l'image affichée
        int offset_y = (widgetSize.height() - scaledImage.height()) / 2; // Offset Y de l'image affichée

        // Vérifie si la position de la souris est à l'intérieur de l'image affichée
        if (mouse_pos_widget.x() >= offset_x && mouse_pos_widget.x() < offset_x + scaledImage.width() &&
            mouse_pos_widget.y() >= offset_y && mouse_pos_widget.y() < offset_y + scaledImage.height()) {

            // Calcule les facteurs d'échelle pour convertir les coordonnées de l'image affichée
            // aux coordonnées de l'image originale
            double scale_w = (double)original_image_cv.cols / scaledImage.width();
            double scale_h = (double)original_image_cv.rows / scaledImage.height();

            // Convertit la position de la souris (en coordonnées du widget)
            // en coordonnées de l'image originale et la définit comme point de départ
            start_point.setX(static_cast<int>((mouse_pos_widget.x() - offset_x) * scale_w));
            start_point.setY(static_cast<int>((mouse_pos_widget.y() - offset_y) * scale_h));

            drawing = true; // Indique qu'un dessin est en cours
        }
    }
}

// Gère l'événement de mouvement de la souris
void ImageViewer::mouseMoveEvent(QMouseEvent *event) {
    Q_UNUSED(event); // Marque l'argument 'event' comme inutilisé
    // Si un dessin est en cours, force le rafraîchissement pour que le rectangle temporaire soit mis à jour
    if (drawing) {
        update();
    }
}

// Gère l'événement de relâchement du bouton de la souris
void ImageViewer::mouseReleaseEvent(QMouseEvent *event) {
    // Vérifie si le bouton gauche est relâché, si un dessin était en cours et si une image est chargée
    if (event->button() == Qt::LeftButton && drawing && !original_image_cv.empty()) {
        drawing = false; // Arrête le mode dessin

        QPoint end_point_widget = event->pos(); // Obtient la position de la souris lors du relâchement

        QSize widgetSize = size(); // Obtient la taille actuelle du widget
        QImage scaledImage = current_image_qt.scaled(widgetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int offset_x = (widgetSize.width() - scaledImage.width()) / 2; // Offset X de l'image affichée
        int offset_y = (widgetSize.height() - scaledImage.height()) / 2; // Offset Y de l'image affichée

        // Calcule les facteurs d'échelle pour convertir les coordonnées de l'image affichée
        // aux coordonnées de l'image originale
        double scale_w = (double)original_image_cv.cols / scaledImage.width();
        double scale_h = (double)original_image_cv.rows / scaledImage.height();

        QPoint end_point_original;
        // Convertit la position de la souris (en coordonnées du widget)
        // en coordonnées de l'image originale et la définit comme point de fin
        end_point_original.setX(static_cast<int>((end_point_widget.x() - offset_x) * scale_w));
        end_point_original.setY(static_cast<int>((end_point_widget.y() - offset_y) * scale_h));

        // Normalise les coordonnées de début et de fin pour s'assurer qu'elles sont dans les limites de l'image
        // et que le rectangle a une largeur/hauteur positive.
        // qBound limite une valeur entre une borne inférieure et une borne supérieure.
        int x1 = qBound(0, start_point.x(), original_image_cv.cols);
        int y1 = qBound(0, start_point.y(), original_image_cv.rows);
        int x2 = qBound(0, end_point_original.x(), original_image_cv.cols);
        int y2 = qBound(0, end_point_original.y(), original_image_cv.rows);

        // Crée un objet cv::Rect à partir des points de début et de fin normalisés
        // std::min(x1, x2) assure que le point de départ est toujours le coin supérieur gauche
        // std::abs(x1 - x2) calcule la largeur absolue
        cv::Rect new_rect(std::min(x1, x2), std::min(y1, y2),
                          std::abs(x1 - x2), std::abs(y1 - y2));

        // Si le rectangle a une largeur et une hauteur valides (non nuls)
        if (new_rect.width > 0 && new_rect.height > 0) {
            saveStateToUndoHistory(); // Sauvegarde l'état actuel (rectangles avant l'ajout) pour l'annulation
            drawn_rectangles.push_back(new_rect); // Ajoute le nouveau rectangle à la liste
            update(); // Rafraîchit l'affichage pour montrer le nouveau rectangle
        }
    }
}

// Implémentation de la fonction pour sauvegarder l'état actuel des rectangles dans l'historique d'annulation
void ImageViewer::saveStateToUndoHistory() {
    // Ne pousse dans l'historique que s'il y a des rectangles à sauvegarder,
    // ou si c'est le tout premier état (historique et rectangles vides).
    // Cela évite de stocker des états vides redondants.
    if (!drawn_rectangles.empty() || (undo_history.empty() && drawn_rectangles.empty())) {
        undo_history.push(drawn_rectangles); // Pousse une copie (par valeur) de la liste actuelle des rectangles
    }
}

// Implémentation de la fonction d'annulation du dernier rectangle
void ImageViewer::undoLastRectangle() {
    // Vérifie si l'historique d'annulation n'est pas vide
    if (!undo_history.empty()) {
        drawn_rectangles = undo_history.top(); // Rétablit la liste des rectangles à l'état précédent (le haut de la pile)
        undo_history.pop(); // Retire cet état de la pile d'historique
        update(); // Redessine l'image avec les rectangles mis à jour
    } else {
        qDebug() << "Undo history is empty. Cannot undo further."; // Message si l'historique est vide
        // Optionnel : si drawn_rectangles n'est pas vide mais l'historique l'est (cas où il n'y a plus rien à annuler)
        if (!drawn_rectangles.empty()) {
            drawn_rectangles.clear(); // Efface l'état actuel si l'historique est vide
            update(); // Redessine
        }
    }
}

// Implémentation de la fonction pour effacer tous les rectangles
void ImageViewer::clearAllRectangles() {
    // Vérifie s'il y a des rectangles à effacer
    if (!drawn_rectangles.empty()) {
        saveStateToUndoHistory(); // Sauvegarde l'état actuel avant d'effacer, pour permettre une annulation
        drawn_rectangles.clear(); // Efface tous les rectangles de la liste
        update(); // Rafraîchit l'affichage
        qDebug() << "All rectangles cleared."; // Message de débogage
    } else {
        qDebug() << "No rectangles to clear."; // Message si aucun rectangle à effacer
    }
}
