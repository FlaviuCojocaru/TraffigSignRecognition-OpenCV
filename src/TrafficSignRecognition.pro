TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp

# add open CV
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4
}

#sudo apt-get install libopencv-dev  pentru a vedea librariile opencv4


