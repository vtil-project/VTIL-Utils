CPMAddPackage(
    NAME asmjit
    GIT_REPOSITORY https://github.com/asmjit/asmjit.git
    GIT_TAG d02235b83434943b52a6d7c57118205c5082de08
    OPTIONS
        "ASMJIT_STATIC ON"
        "ASMJIT_NO_INSTALL ON"
        "ASMJIT_NO_CUSTOM_FLAGS ON"
)