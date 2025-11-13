import ctypes
import os

# Path to the compiled shared library
LIB_PATH = os.path.join(os.path.dirname(__file__), "build", "libtree_sitter_tokenizer.so")
#lib = ctypes.CDLL(LIB_PATH)
lib = ctypes.CDLL("libtree_sitter_tokenizer.so")

# -----------------------------------------------------------------------------
# Define C structures
# -----------------------------------------------------------------------------

class Token(ctypes.Structure):
    _fields_ = [
        ("x", ctypes.c_int),
        ("y", ctypes.c_int),
        ("content", ctypes.c_char_p),
        ("node_type", ctypes.c_char_p),
        ("full_path", ctypes.c_char_p),
        ("full_path_length", ctypes.c_int),
    ]


    def toMap(self) -> dict[str, object]:
        content = self.content.decode("utf-8") if self.content else ""
        node_type = self.node_type.decode("utf-8") if self.node_type else ""
        return {
            "x": self.x,
            "y": self.y,
            "x1": self.x + len(content),
            "content": content,
            "node_type": node_type,
            "full_path": self.full_path.decode("utf-8") if self.full_path else "",
            "full_path_length": self.full_path_length,
        }

class TokenStream(ctypes.Structure):
    _fields_ = [
        ("tokens", ctypes.POINTER(Token)),
        ("count", ctypes.c_int),
    ]

class ParserManager(ctypes.Structure):
    pass


# -----------------------------------------------------------------------------
# Function prototypes
# -----------------------------------------------------------------------------

lib.pm_get.argtypes = [ctypes.c_char_p]
lib.pm_get.restype = ctypes.POINTER(ParserManager)

lib.pm_set_lang.argtypes = [ctypes.POINTER(ParserManager), ctypes.c_char_p]
lib.pm_set_lang.restype = ctypes.c_int

lib.pm_release.argtypes = [ctypes.POINTER(ParserManager)]
lib.pm_release.restype = None

lib.get_all_visible_tokens.argtypes = [
    ctypes.POINTER(ParserManager),
    ctypes.c_char_p,
    ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
]
lib.get_all_visible_tokens.restype = ctypes.POINTER(TokenStream)

lib.update.argtypes = [
    ctypes.POINTER(ParserManager),
    ctypes.c_char_p,
    ctypes.POINTER(TokenStream),
]
lib.update.restype = ctypes.c_int

lib.get_languages.argtypes = [ctypes.POINTER(ctypes.POINTER(ctypes.c_char_p)), ctypes.POINTER(ctypes.c_int)]
lib.get_languages.restype = ctypes.c_int

lib.free_token_stream.argtypes = [ctypes.POINTER(TokenStream)]
lib.free_token_stream.restype = None


# -----------------------------------------------------------------------------
# High-level Python wrappers
# -----------------------------------------------------------------------------

def pm_get(lang_id: str):
    pm = lib.pm_get(lang_id.encode("utf-8"))
    if not pm:
        raise RuntimeError(f"Failed to create ParserManager for {lang_id}")
    return pm

def pm_release(pm):
    if pm:
        lib.pm_release(pm)

def get_languages():
    arr = ctypes.POINTER(ctypes.c_char_p)()
    n = ctypes.c_int(0)
    res = lib.get_languages(ctypes.byref(arr), ctypes.byref(n))
    if res != 0:
        return []
    return [arr[i].decode("utf-8") for i in range(n.value)]

def get_all_visible_tokens(pm, source: str, x0=0, y0=0, x1=9999, y1=9999):
    stream_ptr = lib.get_all_visible_tokens(
        pm, source.encode("utf-8"), x0, y0, x1, y1
    )
    if not stream_ptr:
        return []

    stream = stream_ptr.contents
    tokens = []

    # COPY all values *before* freeing
    for i in range(stream.count):
        t = stream.tokens[i]
        tokens.append(t.toMap())

    # now it's safe to free
    lib.free_token_stream(stream_ptr)
    return tokens

def update(pm, source: str, token_stream=None):
    ts_ptr = ctypes.pointer(token_stream) if token_stream else None
    return lib.update(pm, source.encode("utf-8"), ts_ptr)
