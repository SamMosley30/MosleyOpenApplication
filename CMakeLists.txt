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

# Define a variable for our clean installation staging area
set(STAGING_DIR "${CMAKE_BINARY_DIR}/stage")

set(WINDEPLOYQT_EXE "C:/Qt/6.9.0/mingw_64/bin/windeployqt.exe")

add_custom_command(
    TARGET MosleyOpen POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "--- Debugging Deployment Variables ---"
    COMMAND ${CMAKE_COMMAND} -E echo "1. CMAKE_COMMAND = ${CMAKE_COMMAND}"
    COMMAND ${CMAKE_COMMAND} -E echo "2. Qt6_WINDEPLOYQT_EXECUTABLE = ${WINDEPLOYQT_EXE}"
    COMMAND ${CMAKE_COMMAND} -E echo "3. STAGING_DIR = ${STAGING_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "4. Path to QMake = $<TARGET_FILE:Qt6::qmake>"
    COMMAND ${CMAKE_COMMAND} -E echo "5. Path to Exe = $<TARGET_FILE:MosleyOpen>"
    COMMENT "Printing debug variables for deployment..."
)


# This custom command runs after MosleyOpen.exe is built.
# It prepares a full, runnable installation in the STAGING_DIR.
add_custom_command(
    TARGET MosleyOpen POST_BUILD
    # Step 1: Create the staging bin directory
    COMMAND ${CMAKE_COMMAND} -E make_directory "${STAGING_DIR}/bin"
    # Step 2: Copy our executable to the staging bin directory
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:MosleyOpen>" "${STAGING_DIR}/bin"
    # Step 3: Run windeployqt on the executable now in the staging directory
    COMMAND ${WINDEPLOYQT_EXE}
        --dir "${STAGING_DIR}/bin"
        --sql
        --release
        "${STAGING_DIR}/bin/MosleyOpen.exe"
    # Step 4: Copy the MinGW runtime DLLs to the staging bin directory
    COMMAND ${CMAKE_COMMAND} -E copy "C:/Qt/Tools/mingw1310_64/bin/libgcc_s_seh-1.dll" "${STAGING_DIR}/bin"
    COMMAND ${CMAKE_COMMAND} -E copy "C:/Qt/Tools/mingw1310_64/bin/libstdc++-6.dll" "${STAGING_DIR}/bin"
    COMMAND ${CMAKE_COMMAND} -E copy "C:/Qt/Tools/mingw1310_64/bin/libwinpthread-1.dll" "${STAGING_DIR}/bin"
    # Step 5: Copy our database to the staging bin directory
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/tournament.db" "${STAGING_DIR}/bin"
    COMMENT "Staging application and dependencies for packaging..."
)

# The install command is now very simple: just install the contents of the staging directory.
# This avoids all recursion and dependency-finding issues.
install(
    DIRECTORY "${STAGING_DIR}/"
    DESTINATION "."
)

include(CPack)
