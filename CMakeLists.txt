cmake_minimum_required(VERSION 3.19)
project(TinyLangIDE LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)

qt_standard_project_setup()

qt_add_executable(TinyLangIDE
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    LineNumberArea.h
        Pages/EditorPage.h Pages/EditorPage.cpp Pages/EditorPage.ui
    CodeEditor.h CodeEditor.cpp
        Pages/HomePage.cpp Pages/HomePage.h Pages/HomePage.ui
        TinyLangKnowledge/TinyLangUtils.cpp TinyLangKnowledge/TinyLangUtils.h
        IMenuProvider.h
        TinyLangKnowledge/TLSyntaxHighlighter.cpp
        TinyLangKnowledge/TLSyntaxHighlighter.h
        Widgets/Problems.cpp Widgets/Problems.h  Widgets/Problems.ui
        Widgets/FileItem.h
        Widgets/FileTree.cpp Widgets/FileTree.h
)

target_compile_features(TinyLangIDE PRIVATE cxx_std_23)

target_include_directories(TinyLangIDE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Pages
        ${CMAKE_CURRENT_SOURCE_DIR}/TinyLangKnowledge
)

qt_add_translations(
    TARGETS TinyLangIDE
    TS_FILES TinyLangIDE_en_GB.ts
)

target_link_libraries(TinyLangIDE
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS TinyLangIDE
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET TinyLangIDE
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
