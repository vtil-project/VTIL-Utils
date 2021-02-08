CPMAddPackage(
    NAME args
    GIT_REPOSITORY https://github.com/Taywee/args
    GIT_TAG ae22269df734a2b0957a9ab4e37be41f61866dbe
    DOWNLOAD_ONLY ON
)
if(args_ADDED)
    add_library(args INTERFACE)
    target_include_directories(args INTERFACE ${args_SOURCE_DIR})
endif()