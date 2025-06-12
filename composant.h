// composant.h
#ifndef COMPOSANT_H
#define COMPOSANT_H

#include <QString>
#include <opencv2/core.hpp> // Pour cv::Rect
#include <QPixmap>          // Pour stocker l'image du composant

/**
 * @brief La classe Composant représente un composant électronique détecté sur une carte PCB.
 * Elle stocke des informations telles que son identifiant, sa boîte englobante,
 * son aire et une petite image de lui-même.
 */
class Composant
{
public:
    /**
     * @brief Constructeur de la classe Composant.
     * @param id Identifiant unique du composant.
     * @param boundingBox Boîte englobante du composant dans l'image (cv::Rect).
     * @param area Aire du contour du composant en pixels carrés.
     * @param image QPixmap représentant la petite image extraite du composant.
     */
    Composant(int id, const cv::Rect& boundingBox, double area, const QPixmap& image);

    /**
     * @brief Retourne une chaîne de caractères détaillée sur le composant.
     * @return QString contenant l'ID, l'aire et les coordonnées de la boîte englobante.
     */
    QString getDetails() const;

    /**
     * @brief Retourne l'identifiant du composant.
     * @return L'identifiant (int).
     */
    int getId() const { return m_id; }

    /**
     * @brief Retourne la boîte englobante du composant.
     * @return La boîte englobante (cv::Rect).
     */
    cv::Rect getBoundingBox() const { return m_boundingBox; }

    /**
     * @brief Retourne l'aire du composant.
     * @return L'aire (double).
     */
    double getArea() const { return m_area; }

    /**
     * @brief Retourne la QPixmap représentant l'image du composant.
     * @return La QPixmap du composant.
     */
    QPixmap getImage() const { return m_image; }

private:
    int m_id;             // Identifiant unique du composant
    cv::Rect m_boundingBox; // Boîte englobante (x, y, largeur, hauteur)
    double m_area;        // Aire du contour
    QPixmap m_image;      // Image extraite du composant
};

#endif // COMPOSANT_H
