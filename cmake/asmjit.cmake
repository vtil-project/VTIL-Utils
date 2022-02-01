CPMAddPackage(
    NAME asmjit
    GIT_REPOSITORY https://github.com/asmjit/asmjit.git
    GIT_TAG 9a92d2f97260749f6f29dc93e53c743448f0137a
    OPTIONS
        "ASMJIT_STATIC ON"
        "ASMJIT_NO_INSTALL ON"
)