import qbs
import qbs.Probes
import qbs.FileInfo
import qbs.Environment

Project {
    property path ide_source_tree: path
    property string libDirName: "lib"
    property string includeDirName: "include"
    property string binDirName: "bin"
    name: "Open Async Game Engine"
    property bool withAutotests: qbs.buildVariant === "debug"
    property bool testsEnabled: Environment.getEnv("TEST") || qbs.buildVariant === "debug"
    qbsSearchPaths: "qbs"
    references: [
        "src/src.qbs",
        "tests/tests.qbs"
    ]
    AutotestRunner {
        Depends { name: "Qt.core" }
    }
}
