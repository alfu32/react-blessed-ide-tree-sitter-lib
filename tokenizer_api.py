import ctypes
import os

# --- Load the compiled shared library ---
LIB_PATH = os.path.join(os.path.dirname(__file__), "build", "libtree_sitter_tokenizer.so")
lib = ctypes.CDLL(LIB_PATH)

# --- ctypes struct definitions ---

class Token(ctypes.Structure):
    _fields_ = [
        ("x", ctypes.c_int),
        ("y", ctypes.c_int),
        ("content", ctypes.c_char_p),
        ("qualifiers", ctypes.POINTER(ctypes.c_char_p)),
        ("qualifier_count", ctypes.c_int),
    ]

    def toMap(self) -> dict[str, object]:
        content = self.content.decode("utf-8") if self.content else ""
        quals = []
        if self.qualifiers and self.qualifier_count > 0:
            for i in range(self.qualifier_count):
                qptr = self.qualifiers[i]
                if qptr:
                    quals.append(qptr.decode("utf-8"))
        return {
            "x": self.x,
            "y": self.y,
            "content": content,
            "qualifiers": quals,
        }

    def __str__(self) -> str:
        return str(self.toMap())


class TokenStream(ctypes.Structure):
    _fields_ = [
        ("tokens", ctypes.POINTER(Token)),
        ("count", ctypes.c_int),
    ]


# --- Define function signatures for the C API ---

lib.get_all_visible_tokens.argtypes = [
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
]
lib.get_all_visible_tokens.restype = TokenStream

lib.update.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.POINTER(TokenStream)]
lib.update.restype = ctypes.c_int

lib.get_languages.argtypes = [
    ctypes.POINTER(ctypes.POINTER(ctypes.c_char_p)),
    ctypes.POINTER(ctypes.c_int),
]
lib.get_languages.restype = ctypes.c_int

lib.free_token_stream.argtypes = [ctypes.POINTER(TokenStream)]
lib.free_token_stream.restype = None


# --- Python-friendly wrapper functions ---

def get_all_visible_tokens(source: str, lang_id: str, x0=0, y0=0, x1=9999, y1=9999):
    stream = lib.get_all_visible_tokens(
        source.encode("utf-8"), lang_id.encode("utf-8"),
        x0, y0, x1, y1
    )

    # Explicitly keep a reference to avoid freeing a temporary
    # sref = TokenStream(stream.tokens, stream.count)

    tokens = []
    for i in range(stream.count):
        t = stream.tokens[i]
        tokens.append({
            "x": t.x,
            "y": t.y,
            "content": t.content.decode("utf-8") if t.content else "",
            "full_path": t.full_path.decode("utf-8") if t.full_path else "",
        })

    # lib.free_token_stream(ctypes.pointer(sref))
    return tokens


def update(source: str, lang_id: str, stream: TokenStream):
    """Reparse source incrementally using the parser manager."""
    return lib.update(
        source.encode("utf-8"),
        lang_id.encode("utf-8"),
        ctypes.byref(stream),
    )


def get_languages() -> list[str]:
    """Return the list of languages compiled into the library."""
    lang_list = ctypes.POINTER(ctypes.c_char_p)()
    size = ctypes.c_int(0)
    res = lib.get_languages(ctypes.byref(lang_list), ctypes.byref(size))
    if res != 0 or not lang_list:
        return []
    langs = []
    for i in range(size.value):
        ptr = lang_list[i]
        if ptr:
            langs.append(ptr.decode("utf-8"))
    # ctypes.CDLL(None).free(lang_list)
    return langs

class ParserManager(ctypes.Structure):
    _fields_ = [
        ("lang_id", ctypes.c_char_p),
        ("parser", ctypes.c_void_p),
        ("tree", ctypes.c_void_p),
        ("next", ctypes.c_void_p),
    ]

lib.pm_get.argtypes = [ctypes.c_char_p]
lib.pm_get.restype = ctypes.POINTER(ParserManager)

lib.pm_set_lang.argtypes = [ctypes.c_char_p]
lib.pm_set_lang.restype = ctypes.c_int

lib.pm_release.argtypes = [ctypes.POINTER(ParserManager)]
lib.pm_release.restype = None
