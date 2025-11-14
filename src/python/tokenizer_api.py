import ctypes
import os

# Path to the compiled shared library
LIB_PATH = os.path.join(os.path.dirname(__file__), "build", "libtree_sitter_tokenizer.so")
#lib = ctypes.CDLL(LIB_PATH)
lib = ctypes.CDLL("libtree_sitter_tokenizer.so")

# -----------------------------------------------------------------------------
# Define C structures
# -----------------------------------------------------------------------------

class token_t(ctypes.Structure):
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

class token_stream_t(ctypes.Structure):
    _fields_ = [
        ("tokens", ctypes.POINTER(token_t)),
        ("count", ctypes.c_int),
    ]

class source_code_parser_t(ctypes.Structure):
    pass


# -----------------------------------------------------------------------------
# Function prototypes
# -----------------------------------------------------------------------------

lib.source_code_parser__new.argtypes = [ctypes.c_char_p,ctypes.c_char_p]
lib.source_code_parser__new.restype = ctypes.POINTER(source_code_parser_t)

lib.source_code_parser__set_lang.argtypes = [ctypes.POINTER(source_code_parser_t), ctypes.c_char_p]
lib.source_code_parser__set_lang.restype = ctypes.c_int

lib.source_code_parser__set_source.argtypes = [ctypes.POINTER(source_code_parser_t), ctypes.c_char_p]
lib.source_code_parser__set_source.restype = None

lib.source_code_parser__free.argtypes = [ctypes.POINTER(source_code_parser_t)]
lib.source_code_parser__free.restype = None

lib.source_code_parser__get_all_visible_tokens.argtypes = [
    ctypes.POINTER(source_code_parser_t),
    ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
]
lib.source_code_parser__get_all_visible_tokens.restype = ctypes.POINTER(token_stream_t)

lib.source_code_parser__update.argtypes = [
    ctypes.POINTER(source_code_parser_t),
    ctypes.c_char_p,
    ctypes.POINTER(token_stream_t),
]
lib.source_code_parser__update.restype = ctypes.c_int

lib.get_languages.argtypes = [ctypes.POINTER(ctypes.POINTER(ctypes.c_char_p)), ctypes.POINTER(ctypes.c_int)]
lib.get_languages.restype = ctypes.c_int

lib.token_stream__free.argtypes = [ctypes.POINTER(token_stream_t)]
lib.token_stream__free.restype = None


# -----------------------------------------------------------------------------
# High-level Python wrappers
# -----------------------------------------------------------------------------

def source_code_parser__new(lang_id: str,filename:str):
    pm = lib.source_code_parser__new(lang_id.encode("utf-8"),filename.encode("utf-8"))
    if not pm:
        raise RuntimeError(f"Failed to create ParserManager for {lang_id} with name {filename}")
    return pm

def source_code_parser__set_source(parser, source: str):
    lib.source_code_parser__set_source(parser, source.encode("utf-8"))

def source_code_parser__free(pm):
    if pm:
        lib.source_code_parser__free(pm)

def get_languages():
    arr = ctypes.POINTER(ctypes.c_char_p)()
    n = ctypes.c_int(0)
    res = lib.get_languages(ctypes.byref(arr), ctypes.byref(n))
    if res != 0:
        return []
    return [arr[i].decode("utf-8") for i in range(n.value)]

def source_code_parser__get_all_visible_tokens(pm, x0=0, y0=0, x1=9999, y1=9999) -> list[dict[str,any]]:
    stream_ptr = lib.source_code_parser__get_all_visible_tokens(
        pm, x0, y0, x1, y1
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
    token_stream__free(stream_ptr)
    return tokens

def source_code_parser__update(pm, token_stream=None):
    ts_ptr = ctypes.pointer(token_stream) if token_stream else None
    return lib.source_code_parser__update(pm, ts_ptr)


def token_stream__free(token_stream):
    if token_stream:
        lib.token_stream__free(token_stream)