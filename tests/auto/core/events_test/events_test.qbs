import qbs
CppApplication {
    files: [
        "*.cpp",
    ]
    type: ["application", "autotest"]
    consoleApplication: true
    name: "Events autotest"
    Depends { name: "Event System" }
    Depends { name: "cpp" }
    Depends { name: "Qt.test" }
}
