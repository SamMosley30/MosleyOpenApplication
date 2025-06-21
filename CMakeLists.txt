cmake_minimum_required(VERSION 3.10.0)
project(MosleyOpen VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Test)
qt_standard_project_setup()

set(APP_SOURCES 
    main.cpp
    utils.h
    CommonStructs.h
    MainWindow.h
    MainWindow.cpp
    PlayerDialog.h
    PlayerDialog.cpp
    CoursesDialog.h
    CoursesDialog.cpp
    SpinBoxDelegate.h
    SpinBoxDelegate.cpp
    CheckBoxDelegate.h
    CheckBoxDelegate.cpp
    HolesTransposedModel.h
    HolesTransposedModel.cpp
    ScoreTableModel.h
    ScoreTableModel.cpp
    ScoreEntryDialog.h
    ScoreEntryDialog.cpp
    TournamentLeaderboardModel.h
    TournamentLeaderboardModel.cpp
    TournamentLeaderboardDialog.h
    TournamentLeaderboardDialog.cpp
    TournamentLeaderboardWidget.h
    TournamentLeaderboardWidget.cpp
    DailyLeaderboardModel.h
    DailyLeaderboardModel.cpp
    DailyLeaderboardWidget.h
    DailyLeaderboardWidget.cpp
    TeamLeaderboardModel.h
    TeamLeaderboardModel.cpp
    TeamLeaderboardWidget.h
    TeamLeaderboardWidget.cpp
    TeamAssemblyDialog.h
    TeamAssemblyDialog.cpp
    PlayerListWidget.h
    PlayerListWidget.cpp
)

qt_add_executable(MosleyOpen ${APP_SOURCES})

target_compile_options(MosleyOpen PRIVATE -fmodules-ts)

target_link_libraries(MosleyOpen PRIVATE Qt6::Widgets Qt6::Sql)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

# --- CPack Installer Configuration ---
if(WIN32)
    # Set basic package information
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VERSION "${CPACK_PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Mosley Open Golf Tournament Manager")
    set(CPACK_PACKAGE_VENDOR "Sammos")
    set(CPACK_PACKAGE_CONTACT "your.email@example.com")
    set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME} ${CPACK_PACKAGE_VERSION}")
    set(CPACK_NSIS_INSTALLER_ICON_CODE "") # You can add an icon here
    set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME} ${CPACK_PACKAGE_VERSION}")

    # Tell CPack to install the executable and the database file
    install(TARGETS ${PROJECT_NAME} DESTINATION bin)
    # Make sure tournament.db is in your project's root directory or provide the full path
    install(FILES tournament.db DESTINATION bin)

    # This is the magic part: a post-install script that runs windeployqt
    # and copies the necessary MinGW DLLs automatically.
    install(CODE "
        message(STATUS \"Running windeployqt on $<TARGET_FILE:${PROJECT_NAME}>\")
        execute_process(COMMAND
            \"${Qt6_WINDEPLOYQT_EXECUTABLE}\"
            --dir \"\${CMAKE_INSTALL_PREFIX}\"
            --sql
            --release
            \"$<TARGET_FILE:${PROJECT_NAME}>\"
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR \"windeployqt failed with exit code \${result}\")
        endif()

        message(STATUS \"Copying MinGW runtime DLLs...\")
        set(MINGW_BIN_DIR \"C:/Qt/Tools/mingw1310_64/bin\")
        file(COPY \${MINGW_BIN_DIR}/libgcc_s_seh-1.dll DESTINATION \${CMAKE_INSTALL_PREFIX}/bin)
        file(COPY \${MINGW_BIN_DIR}/libstdc++-6.dll DESTINATION \${CMAKE_INSTALL_PREFIX}/bin)
        file(COPY \${MINGW_BIN_DIR}/libwinpthread-1.dll DESTINATION \${CMAKE_INSTALL_PREFIX}/bin)
    ")

    # Include the CPack module to enable the 'package' target
    include(CPack)
endif()