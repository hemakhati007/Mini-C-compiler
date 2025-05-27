let compileLexer, compileAST, compileIR, compileOptimizedIR, compileCodegen,runCodegen;




function showStats(stage, timeMs = 0, success = true) {
  document.getElementById("status").textContent = `Status: ✅ ${stage} completed`;
  document.getElementById("performance").textContent = `Time: ${timeMs.toFixed(2)} ms`;
  document.getElementById("successRate").textContent = `Success Rate: ${success ? "100%" : "0%"}`;
  document.getElementById("timeComplexity").textContent = "Time Complexity: O(1)";
  document.getElementById("spaceComplexity").textContent = "Space Complexity: O(1)";
}

document.addEventListener("DOMContentLoaded", () => {

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
function toggleTheme() {
  const style = document.getElementById("themeStyle");
  const isDark = style.innerHTML.includes("background-color: #121212");

  if (isDark) {
    // Light theme
    style.innerHTML = `
      body {
        font-family: 'Segoe UI', sans-serif;
        background-color: #f9f9f9;
        color: #111;
        margin: 0;
        padding: 2rem;
      }

      .container {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 2rem;
      }

      textarea, pre {
        width: 100%;
        padding: 1rem;
        border-radius: 8px;
        border: 1px solid #ccc;
        background: #fff;
        color: #000;
        font-family: monospace;
        font-size: 1rem;
      }

      button {
        margin-top: 1rem;
        margin-right: 0.5rem;
        padding: 0.5rem 1rem;
        font-size: 1rem;
        cursor: pointer;
        background: #6200ee;
        color: #fff;
        border: none;
        border-radius: 6px;
      }

      .output-section h4 {
        margin-bottom: 0.5rem;
        color: #6200ee;
      }

      .result-stats {
        margin-top: 1rem;
        background-color: #eeeeee;
        padding: 1rem;
        border-left: 4px solid #6200ee;
        border-radius: 6px;
      }

      .result-stats p {
        margin: 0.4rem 0;
      }
    `;
  } else {
    // Dark theme (reset to original)
    style.innerHTML = `
      body {
        font-family: 'Segoe UI', sans-serif;
        background-color: #121212;
        color: #e0e0e0;
        margin: 0;
        padding: 2rem;
      }

      .container {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 2rem;
      }

      textarea, pre {
        width: 100%;
        padding: 1rem;
        border-radius: 8px;
        border: 1px solid #333;
        background: #1e1e1e;
        color: #fff;
        font-family: monospace;
        font-size: 1rem;
      }

      button {
        margin-top: 1rem;
        margin-right: 0.5rem;
        padding: 0.5rem 1rem;
        font-size: 1rem;
        cursor: pointer;
        background: #03dac6;
        color: #000;
        border: none;
        border-radius: 6px;
      }

      .output-section h4 {
        margin-bottom: 0.5rem;
        color: #bb86fc;
      }

      .result-stats {
        margin-top: 1rem;
        background-color: #212121;
        padding: 1rem;
        border-left: 4px solid #03dac6;
        border-radius: 6px;
      }

      .result-stats p {
        margin: 0.4rem 0;
      }
    `;
  }
}
