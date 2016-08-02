import qbs

Product {
    name: "Event System"
    cpp.cxxLanguageVersion: "c++1z"
    files: [
        "*.hpp",
        "*.cpp"
    ]
    Depends { name: "cpp" }
    Export {
            Depends { name: "cpp" }
            cpp.includePaths: base.concat(product.sourceDirectory)
            cpp.dynamicLibraries: base.concat("pthread")
            cpp.cxxLanguageVersion: "c++1z"
    }

}
