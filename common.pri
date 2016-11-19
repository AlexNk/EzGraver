DEFINES += EZ_VERSION=\\\"$$(EZ_VERSION)\\\"

CONFIG += c++11

CONFIG(release, debug|release): DEFINES += QT_NO_DEBUG_OUTPUT
