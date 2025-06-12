cmake_minimum_required(VERSION 3.16)

project(PCB_PROJECT VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 🧠 Chemin vers OpenCVConfig.cmake : à ajouter
set(OpenCV_DIR "C:/Users/HP/Desktop/opencv/build/x64/vc16/lib")

# 📦 Dépendances Qt et OpenCV
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Gui LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Gui LinguistTools)
find_package(OpenCV REQUIRED)

message(STATUS "OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS = ${OpenCV_LIBS}")

# 🌐 Fichier de traduction Qt
set(TS_FILES PCB_PROJECT_en_AS.ts)

# 📁 Fichiers source
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    imagewindow.h
    imagewindow.cpp
    imagewindow.ui
    clickablelabel.cpp
    clickablelabel.h
    ${TS_FILES}
)

# ⚙️ Création de l'exécutable
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(PCB_PROJECT
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}

        # Ces lignes sont déjà incluses via PROJECT_SOURCES, mais les laisser ici ne nuit pas.
        # Vous pourriez les retirer pour plus de concision.
        imagewindow.h imagewindow.cpp imagewindow.ui
        clickablelabel.h clickablelabel.cpp
        composant.h composant.cpp
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(PCB_PROJECT SHARED ${PROJECT_SOURCES})
    else()
        add_executable(PCB_PROJECT ${PROJECT_SOURCES})
    endif()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

# 📥 Inclusion des headers du projet et OpenCV
target_include_directories(PCB_PROJECT PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} # AJOUTEZ CETTE LIGNE !
    ${OpenCV_INCLUDE_DIRS}
)

# 🔗 Lier Qt et OpenCV
target_link_libraries(PCB_PROJECT PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    ${OpenCV_LIBS}
)

# ⚙️ Propriétés supplémentaires pour l’exécutable
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.PCB_PROJECT)
endif()

set_target_properties(PCB_PROJECT PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# 📦 Installation
include(GNUInstallDirs)
install(TARGETS PCB_PROJECT
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(CMAKE_VERBOSE_MAKEFILE ON)

# 🔚 Finalisation (Qt6 uniquement)
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(PCB_PROJECT)
endif()
#il faut changer le kit d'exécution de Qt MinGW 64-bit car
# lier OpenCV construit avec MSVC (.lib),  est incompatible avec MinGW.
#donc il faut changer en MSVC
