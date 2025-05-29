let compileLexer, compileAST, compileIR, compileOptimizedIR, compileCodegen,runCodegen;
let editor;



function showStats(stage, timeMs = 0, success = true) {
  document.getElementById("status").textContent = `Status: ✅ ${stage} completed`;
  document.getElementById("performance").textContent = `Time: ${timeMs.toFixed(2)} ms`;
  document.getElementById("successRate").textContent = `Success Rate: ${success ? "100%" : "0%"}`;
  document.getElementById("timeComplexity").textContent = "Time Complexity: O(1)";
  document.getElementById("spaceComplexity").textContent = "Space Complexity: O(1)";
}

document.addEventListener("DOMContentLoaded", () => {
  require.config({ paths: { vs: "https://unpkg.com/monaco-editor@latest/min/vs" } });
  require(["vs/editor/editor.main"], () => {
    editor = monaco.editor.create(document.getElementById("editor"), {
      value: "// Sample C code\nint main() { return 42; }",
      language: "c",
      theme: "vs-dark",
      automaticLayout: true,
    });
  });
  Module().then((Module) => {
    console.log("WASM loaded");
   

    const runLexer = Module.cwrap('run_lexer', 'string', ['string']);
    const runAST = Module.cwrap('run_ast', 'string', ['string']);
    const runIR = Module.cwrap('run_ir', 'string', ['string']);
    const runOptimizedIR = Module.cwrap('run_optimized_ir', 'string', ['string']);
    // const runCodegen = Module.cwrap('run_codegen', 'string', ['string']);

    compileLexer = () => {
      const code = document.getElementById("codeInput").value;
      const start = performance.now();
      const result = runLexer(code);
      const time = performance.now() - start;

      document.getElementById("output").textContent = result;
      showStats("Token Generation", time);
    };

    compileAST = () => {
      const code = document.getElementById("codeInput").value;
      const start = performance.now();
      const result = runAST(code);
      const time = performance.now() - start;

      document.getElementById("output").textContent = result;
      showStats("AST Generation", time);
    };

    compileIR = () => {
      const code = document.getElementById("codeInput").value;
      const start = performance.now();
      const result = runIR(code);
      const time = performance.now() - start;

      document.getElementById("output").textContent = "LLVM IR:\n" + result;
      showStats("IR Generation", time);
    };

    compileOptimizedIR = () => {
      const code = document.getElementById("codeInput").value;
      const start = performance.now();
      const ir = runIR(code);
      const optimized = runOptimizedIR(ir);
      //we got optmizrd ir now we want it to be convert to assambly
      const time = performance.now() - start;

      document.getElementById("output").textContent = optimized;
      showStats("Optimized IR Generation", time);
    };


    const compileBtn = document.getElementById("compileBtn");
    compileBtn.onclick = async (event) => {
      console.log("Compile button clicked");
      event.preventDefault(); // just in case, won't hurt
      await compileCodegen();
      console.log("compileCodegen finished");
    };

    async function runCodegen(ir) {
      try {
        const response = await fetch('http://localhost:3000/compile-ir', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ ir })
        });

        if (!response.ok) {
          throw new Error(`HTTP error! Status: ${response.status}`);
        }
        const result = await response.json();
        if (result.asm) {
          const formattedAsm = result.asm
            .replace(/\\t/g, '\t')
            .replace(/\\r\\n|\\n|\\r/g, '\n');

          return formattedAsm;
        } else {
          return `Error: ${result.error}`;
        }
      } catch (error) {
        return `Error: ${error.message}`;
      }
    };
    compileCodegen = async () => {
      const code = document.getElementById("codeInput").value;
      const start = performance.now();
      const ir = runIR(code);  // always regenerate IR for consistency
      const optimized = runOptimizedIR(ir);
      const output = await runCodegen(optimized);
      const time = performance.now() - start;

      document.getElementById("output").textContent = output;

      // document.getElementById("outputWASM").textContent = output;

      const success = !output.includes("error") && output.includes("Execution result");

      showStats("Code Execution", time, success);
    };

    
  }
  ); 
}
);
// Voice Assistant Logic
  const micButton = document.getElementById("micButton");
  const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;

  if (SpeechRecognition) {
    const recognition = new SpeechRecognition();
    recognition.continuous = false;
    recognition.lang = 'en-US';

    micButton.onclick = () => {
      recognition.start();
      micButton.textContent = "🎙️ Listening...";
    };

    recognition.onresult = async (event) => {
      micButton.textContent = "🎤 Voice Prompt";
      const prompt = event.results[0][0].transcript;
      console.log("Prompt:", prompt);

      const aiResponse = await fetch("https://api.together.xyz/v1/chat/completions", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          "Authorization": "Bearer  tgp_v1_8lk3sb6ZQ6IjNwWwXMKj-qGCwMWiECxBh9OSgI9pcZU"
        },
        body: JSON.stringify({
          model: "mistralai/Mixtral-8x7B-Instruct-v0.1",
          prompt: `Write a C program for: ${prompt}`,
          max_tokens: 300
        })
      });

      const json = await aiResponse.json();
      if (json.output) {
        editor.setValue(json.output.trim());
      } else {
        alert("AI response failed: " + (json.error || "Unknown error"));
      }
    };

    recognition.onerror = () => {
      micButton.textContent = "🎤 Voice Prompt";
      alert("Voice recognition failed. Please try again.");
    };
  } else {
    micButton.disabled = true;
    micButton.textContent = "🎤 Not Supported";
  }
function toggleTheme() {
  const style = document.getElementById("themeStyle");
  const isDark = style.innerHTML.includes("background-color: #121212");

  if (isDark) {
    editor.updateOptions({ theme: "vs-light" });
  } else {
    editor.updateOptions({ theme: "vs-dark" });
  }
}
