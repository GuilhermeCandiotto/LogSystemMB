cmake_minimum_required(VERSION 3.20)
project(LogSystemMB VERSION 2.0 LANGUAGES CXX)

# Requer C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configurações específicas do Windows
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# Detectar arquiteturas
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_NAME "x64")
else()
    set(PLATFORM_NAME "Win32")
endif()

# Fontes do projeto
set(SOURCES
    main.cpp
    LogSystem.cpp
)

set(HEADERS
    LogSystem.h
)

# Criar executável
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Incluir diretórios
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Bibliotecas do sistema Windows
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        wininet.lib
        crypt32.lib
    )
endif()

# Procurar ZLIB
find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARIES})
else()
    message(FATAL_ERROR "ZLIB não encontrado. Por favor, instale zlib.")
endif()

# Procurar Minizip (pode estar incluído em contrib/minizip do zlib)
# Se não encontrar automaticamente, especifique o caminho manualmente
find_path(MINIZIP_INCLUDE_DIR
    NAMES zip.h unzip.h
    PATHS
        ${ZLIB_INCLUDE_DIRS}/../contrib/minizip
        /usr/include/minizip
        /usr/local/include/minizip
    PATH_SUFFIXES minizip
)

find_library(MINIZIP_LIBRARY
    NAMES minizip libminizip
    PATHS
        ${ZLIB_LIBRARY_DIRS}
        /usr/lib
        /usr/local/lib
)

if(MINIZIP_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${MINIZIP_INCLUDE_DIR})
    message(STATUS "Minizip include encontrado: ${MINIZIP_INCLUDE_DIR}")
else()
    message(WARNING "Minizip include não encontrado. Pode ser necessário especificar manualmente.")
endif()

if(MINIZIP_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MINIZIP_LIBRARY})
    message(STATUS "Minizip library encontrada: ${MINIZIP_LIBRARY}")
else()
    message(WARNING "Minizip library não encontrada. Certifique-se de que está disponível.")
endif()

# Configurações de compilação
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4          # Warning level 4
        /WX-         # Não tratar warnings como erros (pode mudar para /WX se quiser)
        /permissive- # Conformidade com padrões
        /Zc:__cplusplus # Definir macro __cplusplus corretamente
    )
    
    # Configurações de runtime
    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
    
    # Subsistema Windows
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Criar diretório de output organizado
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# Copiar dependências após build (se necessário)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/Config"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/Log"
    COMMENT "Criando diretórios Config e Log"
)

# Mensagens de status
message(STATUS "")
message(STATUS "=== LogSystemMB Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Platform: ${PLATFORM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "ZLIB: ${ZLIB_LIBRARIES}")
message(STATUS "==================================")
message(STATUS "")

# Opções de instalação (opcional)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY Config/
    DESTINATION bin/Config
    OPTIONAL
)
