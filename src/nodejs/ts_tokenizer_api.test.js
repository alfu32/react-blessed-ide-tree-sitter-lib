const { SourceCodeParser, getLanguages } = require('./build/Release/tokenizer_addon');

console.log("Available languages:", getLanguages());

const pm = new SourceCodeParser('c','source.c');
const code = `
int main() {
  int x = 42;
  return x;
}
`;
pm.setSource(code);

const tokens = pm.getAllVisibleTokens();
console.log("Token count:", tokens.length);
console.log(tokens.slice(0, 10));

// The SourceCodeParser stays alive across multiple calls
pm.setSource('int y = 5;');
const tokens2 = pm.getAllVisibleTokens();
console.log("Token count (2):", tokens2.length);

// Explicit cleanup (optional — GC will also handle it)
pm.close();
console.log("Closed SourceCodeParser");

