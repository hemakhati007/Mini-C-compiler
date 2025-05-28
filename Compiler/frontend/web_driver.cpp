#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include <map>
#include <iostream>
#include <emscripten/emscripten.h>
#include <cstdio>
#include <unordered_map>
#include <iomanip>  // for std::setprecision, std::fixed

std::unordered_map<std::string, std::string> globalSymbolTable;
std::vector<std::string> semanticErrors;

std::string result;
std::map<std::string, std::string> namedValues;
// -------------------- Lexer --------------------
struct Token {
    std::string type, value;
};

std::string removeComments(const std::string& code) {
    std::string result;
    bool in_single = false, in_multi = false;

    for (size_t i = 0; i < code.size(); ++i) {
        if (in_single && code[i] == '\n') { in_single = false; result += '\n'; continue; }
        if (in_multi && code[i] == '*' && code[i+1] == '/') { in_multi = false; ++i; continue; }

        if (!in_single && !in_multi) {
            if (code[i] == '/' && code[i+1] == '/') { in_single = true; ++i; continue; }
            if (code[i] == '/' && code[i+1] == '*') { in_multi = true; ++i; continue; }
            result += code[i];
        }
    }
    return result;
}
// std::vector<Token> tokens;
std::vector<Token> tokenizeStructured(const std::string& input) {
    std::string clean = removeComments(input);
    std::vector<Token> tokens;

    // Match patterns: whitespace, float literals, char literals, multi-char symbols, integers, identifiers, single-char symbols
    std::regex pattern(R"(\s+|==|!=|<=|>=|[+\-*/=<>(){};]|'[^']'|\d+\.\d+|\d+|[a-zA-Z_][a-zA-Z0-9_]*)");

    std::map<std::string, std::string> keywords = {
        {"int", "KEYWORD"},
        {"float", "KEYWORD"},
        {"char", "KEYWORD"},
        {"return", "KEYWORD"},
        {"if", "KEYWORD"},
        {"else", "KEYWORD"},
        {"while", "KEYWORD"},
        {"for", "KEYWORD"}
    };

    for (auto it = std::sregex_iterator(clean.begin(), clean.end(), pattern); it != std::sregex_iterator(); ++it) {
        std::string tok = (*it).str();

        // Skip whitespace
        if (std::regex_match(tok, std::regex(R"(\s+)"))) continue;

        std::string type;
        if (keywords.count(tok)) {
            type = keywords[tok];
        } else if (std::regex_match(tok, std::regex(R"(\d+\.\d+)"))) {
            type = "FLOAT";
        } else if (std::regex_match(tok, std::regex(R"(\d+)"))) {
            type = "INTEGER";
        } else if (std::regex_match(tok, std::regex(R"('([^']|\\')')"))) {
            type = "CHAR";
        } else if (std::regex_match(tok, std::regex(R"([a-zA-Z_][a-zA-Z0-9_]*)"))) {
            type = "IDENTIFIER";
        } else {
            type = "SYMBOL";
        }

        tokens.push_back({type, tok});
    }

    return tokens;
}


std::string serializeTokens(const std::vector<Token>& tokens) {
    std::stringstream ss;
    for (const auto& t : tokens)
        ss << "TOKEN(" << t.type << ", \"" << t.value << "\")\n";
    return ss.str();
}

// -------------------- AST --------------------
struct ASTNode {
    std::string type;
    std::string value;
    std::vector<ASTNode*> children;

    // Optional metadata for semantic analysis
    std::string inferredType;
    bool isDeclared = false;

    ASTNode(std::string t, std::string v = "") : type(t), value(v) {}
    ~ASTNode() {
        for (auto* child : children) delete child;
    }
};

size_t current = 0;
std::vector<Token> tokens;

Token peek() {
    if (current < tokens.size()) return tokens[current];
    return {"EOF", ""};
}

Token advance() {
    if (current < tokens.size()) return tokens[current++];
    return {"EOF", ""};
}

bool check(const std::string& type) {
    return peek().type == type || peek().value == type;
}

bool match(const std::string& expected) {
    if (check(expected)) {
        advance();
        return true;
    }
    return false;
}

bool consume(const std::string& expected) {
    if (match(expected)) return true;
    semanticErrors.push_back("Expected '" + expected + "' but got '" + peek().value + "'");
    return false;
}

ASTNode* parsePrimary() {
    if (tokens[current].type == "INTEGER") return new ASTNode("Literal", tokens[current++].value);
    if (tokens[current].type == "IDENTIFIER") {
        std::string id = tokens[current++].value;
        if (tokens[current].value == "(") {
            current++;
            ASTNode* call = new ASTNode("Call", id);
            while (tokens[current].value != ")") {
                call->children.push_back(parsePrimary());
                if (tokens[current].value == ",") current++;
            }
            current++; // skip ')'
            return call;
        }
        return new ASTNode("Identifier", id);
    }
    return nullptr;
}
ASTNode* parseExpression() {
    // Parse left operand
    Token leftTok = advance();
    ASTNode* left = nullptr;

    if (leftTok.type == "IDENTIFIER") {
        left = new ASTNode("Identifier", leftTok.value);
    } else if (leftTok.type == "INTEGER" || leftTok.type == "FLOAT" || leftTok.type == "CHAR") {
        left = new ASTNode("Literal", leftTok.value);
    } else {
        return nullptr;
    }

    // Check if next token is a binary operator
    if (peek().type == "SYMBOL" && (peek().value == "+" || peek().value == "-" || peek().value == "*" || peek().value == "/")) {
        Token opTok = advance();
        Token rightTok = advance();

        ASTNode* right = nullptr;
        if (rightTok.type == "IDENTIFIER") {
            right = new ASTNode("Identifier", rightTok.value);
        } else if (rightTok.type == "INTEGER" || rightTok.type == "FLOAT" || rightTok.type == "CHAR") {
            right = new ASTNode("Literal", rightTok.value);
        } else {
            return nullptr;
        }

        ASTNode* binOp = new ASTNode("BinaryOp", opTok.value);
        binOp->children.push_back(left);
        binOp->children.push_back(right);
        return binOp;
    }

    return left; // no operator, just a single operand
}





ASTNode* parseVarDecl() {
    // Check for variable type keyword
    if (!(peek().value == "int" || peek().value == "float" || peek().value == "char")) return nullptr;

    Token typeTok = advance(); // int, float, char
    std:: cout<<typeTok.value;
    Token nameTok = advance();
    std::cout<<nameTok.value;


    if (nameTok.type != "IDENTIFIER") return nullptr;

    ASTNode* varDecl = new ASTNode("VarDecl");
    varDecl->children.push_back(new ASTNode("Type", typeTok.value));
    varDecl->children.push_back(new ASTNode("Name", nameTok.value));

    // Optional initialization
    if (peek().value == "=") {
        advance(); // consume '='
        ASTNode* expr = parseExpression();
        if (!expr) return nullptr;
        varDecl->children.push_back(expr);
    }

    if (!match(";")) return nullptr; // expect ';' at end

    return varDecl;
}

ASTNode* parseStatement() {
    if (match("int")) {
        // Variable declaration
        if (check("Identifier")) {
            std::string varName = advance().value;
            if (globalSymbolTable.count(varName)) {
                semanticErrors.push_back("Variable '" + varName + "' re-declared.");
            } else {
                globalSymbolTable[varName] = "int";
            }
            consume(";");
            ASTNode* varDecl = new ASTNode("VarDecl", varName);
            varDecl->children.push_back(new ASTNode("Type", "int"));
            return varDecl;
        } else {
            semanticErrors.push_back("Expected variable name after 'int'.");
            return nullptr;
        }
    } else if (check("Identifier")) {
        // Assignment
        std::string varName = advance().value;
        if (!globalSymbolTable.count(varName)) {
            semanticErrors.push_back("Undeclared variable: " + varName);
        }
        if (match("=")) {
            ASTNode* expr = parseExpression();
            consume(";");
            ASTNode* assign = new ASTNode("Assignment", varName);
            assign->children.push_back(expr);
            return assign;
        } else {
            semanticErrors.push_back("Expected '=' after identifier.");
            return nullptr;
        }
    } else if (match("return")) {
        // Return statement
        ASTNode* expr = parseExpression();
        consume(";");
        ASTNode* ret = new ASTNode("Return");
        ret->children.push_back(expr);
        return ret;
    }
    return nullptr;
}

ASTNode* parseReturn() {
    current++; // skip 'return'
    ASTNode* ret = new ASTNode("Return");
    ASTNode* expr = parseExpression();
    if (expr) ret->children.push_back(expr);
    if (tokens[current].value == ";") current++;
    return ret;
}


ASTNode* parseBlock() {
    current++; // skip {
    ASTNode* block = new ASTNode("Block");
    while (tokens[current].value != "}") {
        if (tokens[current].value == "return") block->children.push_back(parseReturn());
        else current++;
    }
    current++; // skip }
    return block;
}

ASTNode* parseFunction() {
    if (!(peek().value == "int" && tokens[current + 1].value == "main")) return nullptr;
    
    advance(); // int
    Token fname = advance(); // main
    match("("); match(")"); match("{");

    ASTNode* func = new ASTNode("Function", fname.value);
    func->children.push_back(new ASTNode("ReturnType", "int"));

    ASTNode* block = new ASTNode("Block");

    while (peek().value != "}") {
        ASTNode* stmt = parseVarDecl();
        if (!stmt) stmt = parseReturn();
        if (stmt) block->children.push_back(stmt);
        else current++; // skip unknown
    }

    match("}"); // consume }
    func->children.push_back(block);
    return func;
}


std::string getNodeType(ASTNode* node) {
    if (node->type == "Literal") {
        if (node->value.find('.') != std::string::npos) return "float";
        else return "int";
    } else if (node->type == "Identifier") {
        if (globalSymbolTable.count(node->value)) {
            return globalSymbolTable[node->value];
        } else {
            return "unknown";
        }
    } else if (node->type == "BinaryOp") {
        std::string left = getNodeType(node->children[0]);
        std::string right = getNodeType(node->children[1]);
        if (left == "float" || right == "float") return "float";
        if (left == "int" && right == "int") return "int";
        return "unknown";
    }
    return "unknown";
}


// void analyzeSemantics(ASTNode* node) {
//     if (!node) return;

//     if (node->type == "VarDecl") {
//         std::string varName = node->value;
//         std::string varType = node->children[0]->value; // First child = type

//         if (globalSymbolTable.count(varName)>1) {
//             semanticErrors.push_back("Variable '" + varName + "' re-declared.");
//         } else {
//             globalSymbolTable[varName] = varType;
//         }
//     }

//     else if (node->type == "Identifier") {
//         if (globalSymbolTable.count(node->value)<1) {
//             semanticErrors.push_back("Undeclared variable: " + node->value);
//         }
//     }

//     else if (node->type == "BinaryOp") {
//         ASTNode* left = node->children[0];
//         ASTNode* right = node->children[1];

//         analyzeSemantics(left);
//         analyzeSemantics(right);

//         // Optional: Check type compatibility
//         std::string leftType = getNodeType(left);
//         std::string rightType = getNodeType(right);
//         if (leftType != rightType) {
//             semanticErrors.push_back("Type mismatch in binary operation: " + leftType + " vs " + rightType);
//         }
//     }

//     else if (node->type == "Assignment") {
//         std::string varName = node->value;
//         if (globalSymbolTable.count(varName)==0) {
//             semanticErrors.push_back("Assignment to undeclared variable: " + varName);
//         } else {
//             ASTNode* expr = node->children[0];
//             analyzeSemantics(expr);
//             // parseExpression(expr);

//             std::string expected = globalSymbolTable[varName];
//             std::string actual = getNodeType(expr);
//             if (expected != actual) {
//                 semanticErrors.push_back("Type mismatch in assignment to '" + varName + "': expected " + expected + ", got " + actual);
//             }
//         }
        
//         // Assuming Assignment node has 2 children: [lhs, rhs]

//     // ASTNode* lhs = node->children[0]; // variable (identifier)
//     // ASTNode* rhs = node->children[1]; // expression

//     // if (lhs->type != "Identifier") {
//     //     semanticErrors.push_back("Invalid assignment target");
//     //     return;
//     // }

//     // std::string varName = lhs->value;
//     // if (globalSymbolTable.count(varName) == 0) {
//     //     semanticErrors.push_back("Assignment to undeclared variable: " + varName);
//     // } else {
//     //     analyzeSemantics(rhs); // analyze the expression

//     //     std::string expected = globalSymbolTable[varName];
//     //     std::string actual = getNodeType(rhs);
//     //     if (expected != actual) {
//     //         semanticErrors.push_back("Type mismatch in assignment to '" + varName + "': expected " + expected + ", got " + actual);
//     //     }
//     // }
//     }

//     else if (node->type == "Function") {
//         std::string funcName = node->value;
//         if (funcName != "main" && funcName != "add" && funcName != "sub") {
//             semanticErrors.push_back("Function not defined: " + funcName);
//         }
//     }

//     // Recurse on children
//     for (ASTNode* child : node->children) {
//         analyzeSemantics(child);
//     }
// }

void analyzeSemantics(ASTNode* node) {
    if (!node) return;

    if (node->type == "VarDecl") {
        // Expect children: [Type, Name, OptionalInitializer]
        if (node->children.size() < 2) return;

        std::string varType = node->children[0]->value;  // Type node
        std::string varName = node->children[1]->value;  // Name node

        if (globalSymbolTable.count(varName)) {
            semanticErrors.push_back("Variable '" + varName + "' re-declared.");
        } else {
            globalSymbolTable[varName] = varType;
        }

        // Analyze initializer if it exists
        if (node->children.size() > 2) {
            ASTNode* expr = node->children[2];
            analyzeSemantics(expr);

            std::string exprType = getNodeType(expr);
            if (exprType != varType) {
                semanticErrors.push_back("Type mismatch in initialization of '" + varName + "': expected " + varType + ", got " + exprType);
            }
        }
    }

    else if (node->type == "Identifier") {
        if (globalSymbolTable.count(node->value) == 0) {
            semanticErrors.push_back("Undeclared variable: " + node->value);
        }
    }

    else if (node->type == "BinaryOp") {
        if (node->children.size() < 2) return;

        ASTNode* left = node->children[0];
        ASTNode* right = node->children[1];

        analyzeSemantics(left);
        analyzeSemantics(right);

        std::string leftType = getNodeType(left);
        std::string rightType = getNodeType(right);
        if (leftType != rightType) {
            semanticErrors.push_back("Type mismatch in binary operation: " + leftType + " vs " + rightType);
        }
    }

    else if (node->type == "Assignment") {
        std::string varName = node->value;
        if (globalSymbolTable.count(varName) == 0) {
            semanticErrors.push_back("Assignment to undeclared variable: " + varName);
        } else {
            ASTNode* expr = node->children[0];
            analyzeSemantics(expr);

            std::string expected = globalSymbolTable[varName];
            std::string actual = getNodeType(expr);
            if (expected != actual) {
                semanticErrors.push_back("Type mismatch in assignment to '" + varName + "': expected " + expected + ", got " + actual);
            }
        }
    }

    else if (node->type == "Function") {
        std::string funcName = node->value;
        if (funcName != "main" && funcName != "add" && funcName != "sub") {
            semanticErrors.push_back("Function not defined: " + funcName);
        }
    }

    // Recurse on children
    for (ASTNode* child : node->children) {
        analyzeSemantics(child);
    }
}




std::string printASTTree(ASTNode* node, int indent = 0) {
    std::stringstream ss;
    if (!node) return "";

    // Indent based on tree depth
    ss << std::string(indent * 2, ' ') << "• " << node->type;

    // Include value if present
    if (!node->value.empty()) {
        ss << ": " << node->value;
    }

    ss << "\n";

    // Recurse for all children
    for (ASTNode* child : node->children) {
        ss << printASTTree(child, indent + 1);
    }

    return ss.str();
}

std::unordered_map<std::string, int> runtimeValues;

int evaluate(ASTNode* node) {
    if (!node) return 0;

    if (node->type == "Literal") {
        return std::stoi(node->value);
    }

    if (node->type == "Identifier") {
        return runtimeValues[node->value];  // assume declared and initialized
    }

    if (node->type == "BinaryOp") {
        int left = evaluate(node->children[0]);
        int right = evaluate(node->children[1]);
        std::string op = node->value;
        if (op == "+") return left + right;
        if (op == "-") return left - right;
        if (op == "*") return left * right;
        if (op == "/") return right != 0 ? left / right : 0;
    }

    return 0;
}

void execute(ASTNode* node) {
    if (!node) return;

    if (node->type == "VarDecl") {
        std::string varName = node->children[1]->value;
        if (node->children.size() > 2) {
            int val = evaluate(node->children[2]);
            runtimeValues[varName] = val;
        }
    }

    for (ASTNode* child : node->children) {
        execute(child);
    }
}


std::string generateAST(const std::string& input) {
    tokens = tokenizeStructured(input);
    current = 0;
    semanticErrors.clear();
    globalSymbolTable.clear();

    ASTNode* root = new ASTNode("ROOT");

    while (current < tokens.size()) {
        ASTNode* node = parseFunction();
        if (!node) node = parseStatement();
        if (!node) node = parseVarDecl();
        if (node) root->children.push_back(node);
        else current++;
    }

    // Perform semantic analysis
    analyzeSemantics(root);

    if (semanticErrors.empty()) {
        execute(root);              // Runtime simulation
        std::cout << "Value of c: " << runtimeValues["c"] << std::endl; // should output 30
    }

    // Creates a string stream ss, which is used to build a multi-line string output. It works like std::cout, but writes to a string buffer.

    std::stringstream ss;
    ss << printASTTree(root);
    //Checks if any semantic errors were collected during semantic analysis.
    if (!semanticErrors.empty()) {
        ss << "\n--- Semantic Errors ---\n";
        for (const std::string& err : semanticErrors) {
            ss << "❌ " << err << "\n";
        }
    } else {
        ss << "\n✅ Semantic analysis passed.\n";
    }

    delete root;
    return ss.str(); //Returns the entire formatted string (AST + semantic messages) as a std::string.
}



std::string sanitizeVarName(const std::string& name) {
    std::string clean = name;
    clean.erase(std::remove_if(clean.begin(), clean.end(), ::isspace), clean.end());
    return clean;
}

// Helper function to recursively generate IR for expressions
std::string generateIRForExpr(ASTNode* expr, std::stringstream& ir,
    std::map<std::string, std::string>& varRegs, int& regCount,
    const std::map<std::string, std::string>& varTypes) {

    if (expr->type == "Literal") {
        // Check if the literal is a float (contains '.')
        if (expr->value.find('.') != std::string::npos) {
            float floatVal = std::stof(expr->value);
            std::ostringstream formatted;
            formatted << std::scientific << std::setprecision(6) << floatVal;
            return formatted.str();
        }
        else {
            // Handle char literal (single character string)
            if (expr->value.size() == 1 && !isdigit(expr->value[0])) {
                // Convert char to ASCII integer
                int asciiVal = static_cast<int>(expr->value[0]);
                return std::to_string(asciiVal);
            }
            // Otherwise, treat as integer literal
            return expr->value;
        }
    }

    else if (expr->type == "Identifier") {
        std::string varName = expr->value;
        std::string llvmType = varTypes.at(varName);

        std::string cleanVar = sanitizeVarName(varName);
        std::string reg = "%" + std::to_string(regCount++);
        ir << "  " << reg << " = load " << llvmType << ", " << llvmType << "* %" << cleanVar << "\n";
        return reg;
    }

    else if (expr->type == "BinaryOp") {
        ASTNode* left = expr->children[0];
        ASTNode* right = expr->children[1];

        std::string leftReg = generateIRForExpr(left, ir, varRegs, regCount, varTypes);
        std::string rightReg = generateIRForExpr(right, ir, varRegs, regCount, varTypes);

        // Infer type from one of the operands (you can improve this by checking both)
        std::string inferredType;
        if (left->type == "Identifier") inferredType = varTypes.at(left->value);
        else if (right->type == "Identifier") inferredType = varTypes.at(right->value);
        else if (left->type == "Literal" && left->value.find('.') != std::string::npos)
            inferredType = "float";
        else
            inferredType = "i32";

        std::string reg = "%" + std::to_string(regCount++);
        std::string op = expr->value;

        std::string llvmOp;
        if (op == "+") llvmOp = (inferredType == "float") ? "fadd" : "add";
        else if (op == "-") llvmOp = (inferredType == "float") ? "fsub" : "sub";
        else if (op == "*") llvmOp = (inferredType == "float") ? "fmul" : "mul";
        else if (op == "/") llvmOp = (inferredType == "float") ? "fdiv" : "sdiv";
        else llvmOp = (inferredType == "float") ? "fadd" : "add";

        ir << "  " << reg << " = " << llvmOp << " " << inferredType << " " << leftReg << ", " << rightReg << "\n";
        return reg;
    }

    // fallback
    return "0";
}


std::string generateIR(ASTNode* root) {
    std::stringstream ir;

    for (auto* child : root->children) {
        if (child->type == "Function") {
            std::string fname = child->value;
            ir << "define i32 @" << fname << "() {\n";

            ASTNode* block = nullptr;
            for (auto* c : child->children) {
                if (c->type == "Block") {
                    block = c;
                    break;
                }
            }

            if (!block) {
                ir << "  ret i32 0\n";
                ir << "}\n";
                continue;
            }

            std::map<std::string, std::string> varRegs;    // variable to current register holding its value
            std::map<std::string, std::string> varTypes;   // variable to LLVM type: i32, float, i8
            int regCount = 1;

            for (auto* stmt : block->children) {
                if (stmt->type == "VarDecl") {
                    // VarDecl children: [Type, Name, optional Expr]
                    std::string varTypeStr = stmt->children[0]->value;  // "int", "float", "char"
                    std::string varName = stmt->children[1]->value;

                    std::string llvmType;
                    if (varTypeStr == "int") llvmType = "i32";
                    else if (varTypeStr == "float") llvmType = "float";
                    else if (varTypeStr == "char") llvmType = "i8";
                    else llvmType = "i32"; // default fallback

                    varTypes[varName] = llvmType;

                    // Allocate variable
                    // ir << "  %" << varName << " = alloca " << llvmType << "\n";
                    ir << "  %" << sanitizeVarName(varName) << " = alloca " << llvmType << "\n";


                    // Initialization if present
                    if (stmt->children.size() == 3) {
                        ASTNode* expr = stmt->children[2];
                        std::string exprReg = generateIRForExpr(expr, ir, varRegs, regCount, varTypes);
                        
                        // Store the expr result into variable
                        ir << "  store " << llvmType << " " << exprReg << ", " << llvmType << "* %" <<  sanitizeVarName(varName) << "\n";
                    }
                }
                else if (stmt->type == "Return") {
                    ASTNode* retVal = stmt->children[0];
                    std::string retReg = generateIRForExpr(retVal, ir, varRegs, regCount, varTypes);
                    ir << "  ret i32 " << retReg << "\n"; // Assuming function returns int; for float functions you need to adapt.
                }
            }

            ir << "}\n";
        }
    }

    return ir.str();
}





// -------------------- Exports --------------------
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    const char* run_lexer(const char* input) {
        static std::string result;
        result = serializeTokens(tokenizeStructured(std::string(input)));
        return result.c_str();
    }

    EMSCRIPTEN_KEEPALIVE
   const char* run_ast(const char* input) {
    static std::string result;
    globalSymbolTable.clear();
    semanticErrors.clear();

    result = generateAST(input);
    if (!semanticErrors.empty()) {
        result += "\nSemantic Errors:\n";
        for (auto& err : semanticErrors)
            result += "  - " + err + "\n";
    }
    return result.c_str();
   }


   EMSCRIPTEN_KEEPALIVE
const char* run_ir(const char* input) {
    static std::string result;

    tokens = tokenizeStructured(input);
    current = 0;
    semanticErrors.clear();
    globalSymbolTable.clear();

    ASTNode* root = new ASTNode("ROOT");

    while (current < tokens.size()) {
        ASTNode* node = parseFunction();
        if (!node) node = parseStatement();
        if (!node) node = parseVarDecl();
        if (node) root->children.push_back(node);
        else current++;
    }
      result = generateIR(root);
    delete root;
    return result.c_str();

   
}


   EMSCRIPTEN_KEEPALIVE
const char* run_optimized_ir(const char* inputIR) {
    static std::string optimized;

    std::string ir(inputIR);

    std::stringstream inputStream(ir);
    std::stringstream outputStream;
    std::string line;

    // Very simple optimization: Replace "ret i32 40 + 2" with "ret i32 42"
    std::regex constOpRegex(R"(^(\s*%[\w\d]+ = )(\w+) i32 (-?\d+), (-?\d+))");
    std::smatch match;
    while (std::getline(inputStream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, constOpRegex)) {
            std::string regAssign = match[1]; // e.g. "  %3 = "
            std::string op = match[2];        // e.g. "add"
            int lhs = std::stoi(match[3]);
            int rhs = std::stoi(match[4]);
            int result = 0;

            if (op == "add") result = lhs + rhs;
            else if (op == "sub") result = lhs - rhs;
            else if (op == "mul") result = lhs * rhs;
            else if (op == "sdiv") {
                if (rhs == 0) {  // avoid division by zero
                    outputStream << line << "\n";
                    continue;
                }
                result = lhs / rhs;
            }
            else {
                outputStream << line << "\n";
                continue;
            }

            // Replace line with folded constant
            outputStream << regAssign << "add i32 " << result << "\n";
        } else {
            outputStream << line << "\n";
        }
    }
    optimized = "; Optimized IR\n" + outputStream.str();
    return optimized.c_str();

}


EMSCRIPTEN_KEEPALIVE
const char* run_codegen(const char* ir) {
    static std::string result;

    std::string input(ir);
    std::smatch match;

    // 1. Direct return: ret i32 42
    std::regex directRet(R"(ret i32 (\d+))");
    if (std::regex_search(input, match, directRet)) {
        result = "Execution result: " + match[1].str();
        return result.c_str();
    }

    // 2. Function call: call i32 @func(i32 X, i32 Y)
    std::regex callPattern(R"(call i32 @(\w+)\(i32 (\d+), i32 (\d+)\))");
    if (std::regex_search(input, match, callPattern)) {
        std::string func = match[1].str();
        int a = std::stoi(match[2].str());
        int b = std::stoi(match[3].str());

        if (func == "add") result = "Execution result: " + std::to_string(a + b);
        else if (func == "sub") result = "Execution result: " + std::to_string(a - b);
        else if (func == "mul") result = "Execution result: " + std::to_string(a * b);
        else if (func == "div" || func == "divide") result = "Execution result: " + std::to_string(a / b);
        else result = "Execution error: unsupported function '" + func + "'";
        return result.c_str();
    }

    result = "Execution error: no recognizable return.";
    return result.c_str();
}


}
