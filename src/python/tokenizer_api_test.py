import unittest
from tokenizer_api import (
    source_code_parser__get_all_visible_tokens,
    get_languages, source_code_parser__new,
    source_code_parser__free,
    source_code_parser__set_source,
    token_stream__free
)


class TestTokenizer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.langs = get_languages()
        print("Available grammars:", cls.langs)

    # --- C language ---
    def test_c_tokens(self):
        pm = source_code_parser__new("c","source.test.c")
        code = """
        int main(int argc,const char **argv) {
            int x = 1;
            return x;
        }
        """
        source_code_parser__set_source(pm,code)
        tokens = source_code_parser__get_all_visible_tokens(pm)
        self.assertTrue(len(tokens) > 10)
        found_main = any("main" in t["content"] for t in tokens)
        self.assertTrue(found_main)
        for t in tokens:
            print(t)
        source_code_parser__free(pm)

    # --- Python language ---
    def test_python_tokens(self):
        pm = source_code_parser__new("python","python.test.py")
        code = """
        def add(a, b):
            return a + b

        x = add(2, 3)
        """
        source_code_parser__set_source(pm,code)
        tokens = source_code_parser__get_all_visible_tokens(pm)
        self.assertTrue(len(tokens) > 10)
        found_def = any("def" in t["content"] for t in tokens)
        found_add = any("add" in t["content"] for t in tokens)
        self.assertTrue(found_def and found_add)
        for t in tokens:
            print(t)
        source_code_parser__free(pm)

    # --- JavaScript language ---
    def test_javascript_tokens(self):
        pm = source_code_parser__new("javascript","javascript.test.js")
        code = """
        function sum(a, b) {
            return a + b;
        }

        let x = sum(1, 2);
        """
        source_code_parser__set_source(pm,code)
        tokens = source_code_parser__get_all_visible_tokens(pm)
        self.assertTrue(len(tokens) > 10)
        found_func = any("function" in t["content"] for t in tokens)
        found_sum = any("sum" in t["content"] for t in tokens)
        self.assertTrue(found_func and found_sum)
        for t in tokens:
            print(t)
        source_code_parser__free(pm)

    # --- HTML language ---
    def test_html_tokens(self):
        pm = source_code_parser__new("html","html.test.html")
        code = """
        <html>
          <head><title>Example</title></head>
          <body><h1>Hello World</h1></body>
        </html>
        """
        source_code_parser__set_source(pm,code)
        tokens = source_code_parser__get_all_visible_tokens(pm)
        self.assertTrue(len(tokens) > 10)
        found_tag = any("html" in t["content"] for t in tokens)
        found_title = any("title" in t["content"] for t in tokens)
        self.assertTrue(found_tag and found_title)
        for t in tokens:
            print(t)
        source_code_parser__free(pm)

    # --- JSON language ---
    def test_json_tokens(self):
        pm = source_code_parser__new("json","json.test.json")
        code = """
        {
          "name": "John",
          "age": 30,
          "active": true
        }
        """
        source_code_parser__set_source(pm,code)
        tokens = source_code_parser__get_all_visible_tokens(pm)
        self.assertTrue(len(tokens) > 5)
        found_name = any("name" in t["content"] for t in tokens)
        found_true = any("true" in t["content"] for t in tokens)
        self.assertTrue(found_name and found_true)
        for t in tokens:
            print(t)
        source_code_parser__free(pm)

    # --- Window filtering test (C language) ---
    def test_window_filter(self):
        pm = source_code_parser__new("c","c2.test.c")
        code = """
        int a = 0;
        int b = 1;
        int c = 2;
        """
        source_code_parser__set_source(pm,code)
        all_tokens = source_code_parser__get_all_visible_tokens(pm)
        filtered = source_code_parser__get_all_visible_tokens(pm, x0=0, y0=1, x1=80, y1=1)
        self.assertTrue(len(all_tokens) > len(filtered))
        for t in filtered:
            self.assertTrue(t["y"] == 1)
            print(t)
        source_code_parser__free(pm)

if __name__ == "__main__":
    unittest.main(verbosity=2)
    # tt=TestTokenizer()
    # TestTokenizer.setUpClass()
    # tt.test_c_tokens()
    # tt.test_python_tokens()
    # tt.test_javascript_tokens()
    # tt.test_html_tokens()
    # tt.test_json_tokens()
