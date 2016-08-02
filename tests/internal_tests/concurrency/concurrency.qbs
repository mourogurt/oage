import qbs

CppApplication {
    consoleApplication: true
    files: "main.cpp"
    cpp.cxxLanguageVersion: "c++1z"
    cpp.cxxStandardLibrary: "libstdc++"
    Depends { name: "Threads System" }
    Depends { name: "Event System" }


    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }
}

