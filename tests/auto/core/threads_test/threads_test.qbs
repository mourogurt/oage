import qbs
CppApplication {
    files: [
        "*.cpp",
    ]
    type: ["application", "autotest"]
    consoleApplication: true
    name: "Threads autotest"
    Depends { name: "Threads System" }
    Depends { name: "cpp" }
    Depends { name: "Qt.test" }
}

