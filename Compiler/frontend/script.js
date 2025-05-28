// script.js

let compileLexer, compileAST, compileIR, compileOptimizedIR, compileCodegen;

function showStats(stage, timeMs = 0, success = true) {
  document.getElementById("status").textContent = `Status: ✅ ${stage} completed`;
  document.getElementById("performance").textContent = `Time: ${timeMs.toFixed(2)} ms`;
  document.getElementById("successRate").textContent = `Success Rate: ${success ? "100%" : "0%"}`;
  document.getElementById("timeComplexity").textContent = "Time Complexity: O(1)";
  document.getElementById("spaceComplexity").textContent = "Space Complexity: O(1)";
}

document.addEventListener("DOMContentLoaded", () => {
  // Monaco Editor Loader
  require.config({ paths: { vs: "https://cdnjs.cloudflare.com/ajax/libs/monaco-editor/0.44.0/min/vs" } });
  require(["vs/editor/editor.main"], () => {
    window.editor = monaco.editor.create(document.getElementById("editor"), {
      value: "// Sample C code\nint main() { return 42; }",
      language: "c",
      theme: "vs-dark",
      fontSize: 14,
      minimap: { enabled: false }
    });
  });

  Module().then((Module) => {
    const runLexer = Module.cwrap('run_lexer', 'string', ['string']);
    const runAST = Module.cwrap('run_ast', 'string', ['string']);
    const runIR = Module.cwrap('run_ir', 'string', ['string']);
    const runOptimizedIR = Module.cwrap('run_optimized_ir', 'string', ['string']);

    compileLexer = () => {
      const code = editor.getValue();
      const start = performance.now();
      const result = runLexer(code);
      const time = performance.now() - start;
      document.getElementById("output").textContent = result;
      showStats("Token Generation", time);
    };

    compileAST = () => {
      const code = editor.getValue();
      const start = performance.now();
      const result = runAST(code);
      const time = performance.now() - start;
      document.getElementById("output").textContent = result;
      showStats("AST Generation", time);
    };

    compileIR = () => {
      const code = editor.getValue();
      const start = performance.now();
      const result = runIR(code);
      const time = performance.now() - start;
      document.getElementById("output").textContent = "LLVM IR:\n" + result;
      showStats("IR Generation", time);
    };

    compileOptimizedIR = () => {
      const code = editor.getValue();
      const start = performance.now();
      const ir = runIR(code);
      const optimized = runOptimizedIR(ir);
      const time = performance.now() - start;
      document.getElementById("output").textContent = optimized;
      showStats("Optimized IR Generation", time);
    };

    compileCodegen = async () => {
      const code = editor.getValue();
      const start = performance.now();
      const ir = runIR(code);
      const optimized = runOptimizedIR(ir);
      const output = await runCodegen(optimized);
      const time = performance.now() - start;
      document.getElementById("output").textContent = output;
      const success = !output.includes("error") && output.includes("Execution result");
      showStats("Code Execution", time, success);
    };

    async function runCodegen(ir) {
      try {
        const response = await fetch('http://localhost:3000/compile-ir', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ ir })
        });
        if (!response.ok) throw new Error(`HTTP error! Status: ${response.status}`);
        const result = await response.json();
        return result.asm ? result.asm.replace(/\t/g, '\t').replace(/\r\n|\n|\r/g, '\n') : `Error: ${result.error}`;
      } catch (error) {
        return `Error: ${error.message}`;
      }
    }
  });
});

// Theme Toggle
function toggleTheme() {
  document.body.classList.toggle('light-theme');
}

// AI Assistant
document.getElementById("voiceAssistantBtn").addEventListener("click", () => {
  const recognition = new (window.SpeechRecognition || window.webkitSpeechRecognition)();
  recognition.lang = "en-US";
  recognition.interimResults = false;
  recognition.maxAlternatives = 1;

  recognition.start();

  recognition.onresult = async (event) => {
    const prompt = event.results[0][0].transcript;
    console.log("User Prompt:", prompt);
    const code = await generateCodeFromPrompt(prompt);
    monaco.editor.getModels()[0].setValue(code);
  };

  recognition.onerror = (event) => {
    alert("Speech recognition error: " + event.error);
  };
});

async function generateCodeFromPrompt(prompt) {
  const response = await fetch("https://api.together.ai/v1/chat/completions", {
    method: "POST",
    headers: {
      "Authorization": "Bearer tgp_v1_OYf0AAf6l347yHElz_bF1DgRl2NWzktUdJGazWruEw8",
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      model: "meta-llama/Llama-3-8b-chat-hf", // You can change to another chat model if preferred
      messages: [
        { role: "system", content: "You are an expert C programmer assistant." },
        { role: "user", content: `Write a C program for this request: ${prompt}` }
      ],
      temperature: 0.7,
      max_tokens: 500
    })
  });

  const data = await response.json();

  const aiResponse = data.choices?.[0]?.message?.content;
  return aiResponse || "// Failed to generate C code.";
}

