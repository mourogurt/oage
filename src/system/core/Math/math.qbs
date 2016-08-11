import qbs

Product {
    name: "Math Module"
    cpp.cxxLanguageVersion: "c++1z"
    files: [
        "*.hpp",
        "*.cpp"
    ]
    Depends { name: "cpp" }
    Export {
            Depends { name: "cpp" }
            cpp.includePaths: base.concat(product.sourceDirectory)
            cpp.cxxLanguageVersion: "c++1z"
    }

}
