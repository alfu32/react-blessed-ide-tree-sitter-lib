const path = require("path");

// Load compiled addon
const tokenizer = require("./build/Release/tokenizer_addon.node");

console.log("Available grammars:");
const langs = tokenizer.getLanguages();
console.log(langs);

// Pick a language available in your build
const lang = langs.includes("c") ? "c" : langs[0];
console.log("Testing language:", lang);

// Create a parser manager for this language
const pm = tokenizer.createParserManager(lang);

// Sample source code
const code = `
int main(int argc, const char **argv) {
    int x = 42;
    return x;
}
`;

// Query tokens
const tokens = tokenizer.getAllVisibleTokens(pm, code, 0, 0, 9999, 9999);
console.log(`Token count: ${tokens.length}`);

// Print first few tokens
for (let i = 0; i < Math.min(tokens.length, 10); i++) {
    const t = tokens[i];
    // console.log(
    //     `${i}: (${t.x},${t.y}) '${t.content}' path='${t.full_path}' len=${t.full_path_length}`
    // );
    console.log(
        JSON.stringify(t)
    );
}

// Release parser manager
tokenizer.releaseParserManager(pm);

console.log("OK ✅");
