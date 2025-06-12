//Cette classe C++ nommée Composant est conçue pour représenter et stocker les informations relatives
//à un composant électronique individuel qui a été détecté
//et extrait d'une image de circuit imprimé (PCB). En d'autres termes,
//c'est comme une "fiche d'identité" pour chaque pièce trouvée sur la carte.
// composant.cpp
#include "composant.h"

/**
 * @brief Constructeur de la classe Composant.
 * @param id Identifiant unique du composant.
 * @param boundingBox Boîte englobante du composant dans l'image (cv::Rect).
 * @param area Aire du contour du composant en pixels carrés.
 * @param image QPixmap représentant la petite image extraite du composant.
 */
Composant::Composant(int id, const cv::Rect& boundingBox, double area, const QPixmap& image)
    : m_id(id), m_boundingBox(boundingBox), m_area(area), m_image(image)
{
}

/**
 * @brief Retourne une chaîne de caractères détaillée sur le composant.
 * Formatte les informations pour un affichage lisible dans l'interface utilisateur.
 * @return QString contenant l'ID, l'aire et les coordonnées de la boîte englobante.
 */
QString Composant::getDetails() const
{
    // Formatte les détails du composant pour un affichage clair dans la liste
    return QString("Composant %1 (Aire: %2 px²), (X: %3, Y: %4, L: %5, H: %6)")
        .arg(m_id)
        .arg(QString::number(m_area, 'f', 2)) // Formatte l'aire à 2 décimales
        .arg(m_boundingBox.x)
        .arg(m_boundingBox.y)
        .arg(m_boundingBox.width)
        .arg(m_boundingBox.height);
}
