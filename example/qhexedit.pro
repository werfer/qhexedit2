lessThan(QT_MAJOR_VERSION, 5): error("This app requires Qt 5.2+")
equals(QT_MAJOR_VERSION, 5): lessThan(QT_MINOR_VERSION, 2): error("This app requires Qt 5.2+")

QT += widgets

CONFIG += c++14

HEADERS = \
    mainwindow.h \
    optionsdialog.h \
    ../src/qhexedit.h \
    ../src/qhexedit_p.h \
    ../src/xbytearray.h \
    ../src/commands.h \
    ../src/qhexeditdata.h \
    searchdialog.h


SOURCES = \
    main.cpp \
    mainwindow.cpp \
    optionsdialog.cpp \
    ../src/qhexedit.cpp \
    ../src/qhexedit_p.cpp \
    ../src/xbytearray.cpp \
    ../src/commands.cpp \
    ../src/qhexeditdata.cpp \
    searchdialog.cpp


RESOURCES = \
    qhexedit.qrc

FORMS += \
    optionsdialog.ui \
    searchdialog.ui

OTHER_FILES += \
    ../doc/release.txt

TRANSLATIONS += \
    translations/qhexedit_cs.ts \
    translations/qhexedit_de.ts
