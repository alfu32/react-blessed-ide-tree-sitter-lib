const { ParserManager, getLanguages } = require('./build/Release/tokenizer_addon');

console.log("Available languages:", getLanguages());

const pm = new ParserManager('c');
const code = `
int main() {
  int x = 42;
  return x;
}
`;

const tokens = pm.getAllVisibleTokens(code);
console.log("Token count:", tokens.length);
console.log(tokens.slice(0, 10));

// The ParserManager stays alive across multiple calls
const tokens2 = pm.getAllVisibleTokens('int y = 5;');
console.log("Token count (2):", tokens2.length);

// Explicit cleanup (optional — GC will also handle it)
pm.close();
console.log("Closed ParserManager");

