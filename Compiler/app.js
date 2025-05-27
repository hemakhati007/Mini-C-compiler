const express = require("express");
const fs = require("fs");
const { exec } = require("child_process");
const path = require("path");
const app = express();


const cors = require('cors');
app.use(cors());

app.use(express.json());
app.use(express.static(path.join(__dirname, './frontend')));


app.post("/compile-ir", (req, res) => {
    const irCode = req.body.ir;
    console.log("Received IR code:", irCode);
    const inputPath = path.join(__dirname, "temp.ll");
    const outputPath = path.join(__dirname, "temp.s");

    fs.writeFileSync(inputPath, irCode);

    exec(`llc ${inputPath} -o ${outputPath}`, (err, stdout, stderr) => {
        if (err || stderr) {
            console.error("llc error:", err);
            res.status(500).json({ error: stderr || err.message });
        } else {
            const asmCode = fs.readFileSync(outputPath, "utf8");
            console.log("Read assembly code, sending response");
            res.json({ asm: asmCode });
        }
    });
});


app.listen(3000, () => {
    console.log("Server running on port 3000");
});
