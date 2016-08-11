import qbs

Product {
    name: "Threads System"
    cpp.cxxLanguageVersion: "c++1z"
    files: [
        "*.hpp",
        "*.cpp"
    ]
    Depends { name: "cpp" }
    Depends { name: "Event System" }
    Depends { name: "Math Module" }
    Export {
            Depends { name: "cpp" }
            Depends { name: "Event System" }
            Depends { name: "Math Module" }
            cpp.includePaths: base.concat(product.sourceDirectory)
            cpp.dynamicLibraries: base.concat("pthread")
            cpp.cxxLanguageVersion: "c++1z"
    }

}
