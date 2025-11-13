const ffi = require('ffi-napi');
const ref = require('ref-napi');
const Struct = require('ref-struct-napi');
const ArrayType = require('ref-array-napi');
const path = require('path');

// -----------------------------------------------------------------------------
// Define C structs
// -----------------------------------------------------------------------------
const Token = Struct({
    x: 'int',
    y: 'int',
    content: 'string',         // char*
    full_path: 'string',       // char*
    full_path_length: 'int'
});

const TokenPtr = ref.refType(Token);

const TokenStream = Struct({
    tokens: ref.refType(Token),
    count: 'int'
});
const TokenStreamPtr = ref.refType(TokenStream);

const ParserManager = ref.types.void;  // opaque
const ParserManagerPtr = ref.refType(ParserManager);

// -----------------------------------------------------------------------------
// Load the native library
// -----------------------------------------------------------------------------
const libPath = path.join(__dirname, 'build', 'libtree_sitter_tokenizer.so');
const lib = ffi.Library('libtree_sitter_tokenizer.so', {
    // ParserManager lifecycle
    'pm_get': [ParserManagerPtr, ['string']],
    'pm_release': ['void', [ParserManagerPtr]],

    // Token stream operations
    'get_all_visible_tokens': [TokenStreamPtr, [
        ParserManagerPtr, 'string',
        'int', 'int', 'int', 'int'
    ]],
    'free_token_stream': ['void', [TokenStreamPtr]],

    // Misc
    'get_languages': ['int', ['pointer', 'pointer']],
});

// -----------------------------------------------------------------------------
// JS wrappers
// -----------------------------------------------------------------------------
function pmGet(langId) {
    const ptr = lib.pm_get(langId);
    if (ref.isNull(ptr)) throw new Error(`Failed to get ParserManager for ${langId}`);
    return ptr;
}

function pmRelease(pm) {
    if (!ref.isNull(pm)) lib.pm_release(pm);
}

function getLanguages() {
    const arrPtrPtr = ref.alloc(ref.refType('char *'));
    const sizePtr = ref.alloc('int');
    const res = lib.get_languages(arrPtrPtr, sizePtr);
    if (res !== 0) return [];

    const size = sizePtr.deref();
    const arrPtr = arrPtrPtr.deref();
    const arr = [];
    for (let i = 0; i < size; i++) {
        const strPtr = arrPtr.readPointer(i * ref.sizeof.pointer);
        arr.push(strPtr.readCString());
    }
    return arr;
}

function getAllVisibleTokens(pm, source, x0 = 0, y0 = 0, x1 = 9999, y1 = 9999) {
    const streamPtr = lib.get_all_visible_tokens(pm, source, x0, y0, x1, y1);
    if (ref.isNull(streamPtr)) return [];

    const stream = streamPtr.deref();
    const count = stream.count;
    const tokenBase = stream.tokens;
    const tokens = [];

    for (let i = 0; i < count; i++) {
        const tokPtr = tokenBase.readPointer(i * ref.sizeof.pointer, ref.sizeof.pointer);
        const tok = ref.get(tokPtr, 0, Token);
        tokens.push({
            x: tok.x,
            y: tok.y,
            content: tok.content || '',
            full_path: tok.full_path || '',
            full_path_length: tok.full_path_length
        });
    }

    lib.free_token_stream(streamPtr);
    return tokens;
}

// -----------------------------------------------------------------------------
// Exported API
// -----------------------------------------------------------------------------
module.exports = {
    pmGet,
    pmRelease,
    getLanguages,
    getAllVisibleTokens
};
