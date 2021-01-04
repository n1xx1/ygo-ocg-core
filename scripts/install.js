const fs = require("fs");
const path = require("path");

fs.copyFileSync(
    path.join(__dirname, `../build/Release/native.node`),
    path.join(__dirname, `../lib/native.node`)
);

if (process.platform == "win32" && process.arch == "x64") {
    fs.copyFileSync(
        path.join(__dirname, `../build/Release/native.node`),
        path.join(__dirname, `../lib/native-win64.node`)
    );
} else if (process.platform == "win32" && process.arch == "ia32") {
    fs.copyFileSync(
        path.join(__dirname, `../build/Release/native.node`),
        path.join(__dirname, `../lib/native-win32.node`)
    );
}
