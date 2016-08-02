import qbs

Project {
    name: "Autotests"
    condition: project.withAutotests
    references: [
        "core/core.qbs"
    ]
}
