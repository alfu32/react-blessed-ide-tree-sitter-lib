import ctypes
import os


# Path to built library
LIB_PATH = os.path.join(os.path.dirname(__file__), "build", "libtree_sitter_tokenizer.so")
lib = ctypes.CDLL(LIB_PATH)

# --- Structs ---
class Token(ctypes.Structure):
    _fields_ = [
        ("x", ctypes.c_int),
        ("y", ctypes.c_int),
        ("content", ctypes.c_char_p),
        ("qualifiers", ctypes.POINTER(ctypes.c_char_p)),
        ("qualifier_count", ctypes.c_int),
    ]
    def __str__(self) -> str:
        content = self.content.decode("utf-8") if self.content else ""
        quals = []
        if self.qualifiers and self.qualifier_count > 0:
            for i in range(self.qualifier_count):
                qptr = self.qualifiers[i]
                if qptr:
                    quals.append(qptr.decode("utf-8"))
        return f"x:{self.x}, y:{self.y}, content:'{content}', qualifiers:{quals}"

class TokenStream(ctypes.Structure):
    _fields_ = [("tokens", ctypes.POINTER(Token)), ("count", ctypes.c_int)]


# --- Function prototypes ---
lib.get_all_visible_tokens.argtypes = [
    ctypes.c_char_p, ctypes.c_char_p,
    ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
]
lib.get_all_visible_tokens.restype = TokenStream

lib.query_all_visible_tokens.argtypes = [
    ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p,
    ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
]
lib.query_all_visible_tokens.restype = TokenStream

lib.update.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.POINTER(TokenStream)]
lib.update.restype = ctypes.c_int

lib.get_languages.argtypes = [
    ctypes.POINTER(ctypes.POINTER(ctypes.c_char_p)),
    ctypes.POINTER(ctypes.c_int),
]
lib.get_languages.restype = ctypes.c_int

lib.free_token_stream.argtypes = [ctypes.POINTER(TokenStream)]
lib.free_token_stream.restype = None


# --- Python wrappers ---
def get_all_visible_tokens(source: str, lang_id: str, x0=0, y0=0, x1=9999, y1=9999) -> list[Token] :
    stream = lib.get_all_visible_tokens(
        source.encode("utf-8"),
        lang_id.encode("utf-8"),
        x0, y0, x1, y1,
    )
    tokens = []
    for i in range(stream.count):
        t = stream.tokens[i]
        content = t.content.decode("utf-8") if t.content else ""
        tokens.append((t.x, t.y, content))
    lib.free_token_stream(ctypes.byref(stream))
    return tokens


def query_all_visible_tokens(source: str, lang_id: str, query: str, x0=0, y0=0, x1=9999, y1=9999) -> list[Token] :
    stream = lib.query_all_visible_tokens(
        source.encode("utf-8"),
        lang_id.encode("utf-8"),
        query.encode("utf-8"),
        x0, y0, x1, y1,
    )
    tokens = []
    for i in range(stream.count):
        t = stream.tokens[i]
        content = t.content.decode("utf-8") if t.content else ""
        tokens.append((t.x, t.y, content))
    lib.free_token_stream(ctypes.byref(stream))
    return tokens


def update(source: str, lang_id: str, stream: TokenStream):
    return lib.update(source.encode("utf-8"), lang_id.encode("utf-8"), ctypes.byref(stream))


def get_languages()-> list[str] :
    lang_list = ctypes.POINTER(ctypes.c_char_p)()
    size = ctypes.c_int(0)
    res = lib.get_languages(ctypes.byref(lang_list), ctypes.byref(size))
    langs = [lang_list[i].decode("utf-8") for i in range(size.value)]
    return langs
