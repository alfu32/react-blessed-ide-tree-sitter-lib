import unittest
from tokenizer_api import get_all_visible_tokens, get_languages

class TestTokenizer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.langs = get_languages()
        print("Available grammars:", cls.langs)

    # --- C language ---
    def test_c_tokens(self):
        code = """
        int main(int argc,const char **argv) {
            int x = 1;
            return x;
        }
        """
        tokens = get_all_visible_tokens(code, "c")
        self.assertTrue(len(tokens) > 10)
        found_main = any("main" in t.toMap()["content"] for t in tokens)
        self.assertTrue(found_main)
        for t in tokens:
            print(t)

    # --- Python language ---
    def test_python_tokens(self):
        code = """
        def add(a, b):
            return a + b

        x = add(2, 3)
        """
        tokens = get_all_visible_tokens(code, "python")
        self.assertTrue(len(tokens) > 10)
        found_def = any("def" in t.toMap()["content"] for t in tokens)
        found_add = any("add" in t.toMap()["content"] for t in tokens)
        self.assertTrue(found_def and found_add)
        for t in tokens:
            print(t)

    # --- JavaScript language ---
    def test_javascript_tokens(self):
        code = """
        function sum(a, b) {
            return a + b;
        }

        let x = sum(1, 2);
        """
        tokens = get_all_visible_tokens(code, "javascript")
        self.assertTrue(len(tokens) > 10)
        found_func = any("function" in t.toMap()["content"] for t in tokens)
        found_sum = any("sum" in t.toMap()["content"] for t in tokens)
        self.assertTrue(found_func and found_sum)
        for t in tokens:
            print(t)

    # --- HTML language ---
    def test_html_tokens(self):
        code = """
        <html>
          <head><title>Example</title></head>
          <body><h1>Hello World</h1></body>
        </html>
        """
        tokens = get_all_visible_tokens(code, "html")
        self.assertTrue(len(tokens) > 10)
        found_tag = any("html" in t.toMap()["content"] for t in tokens)
        found_title = any("title" in t.toMap()["content"] for t in tokens)
        self.assertTrue(found_tag and found_title)
        for t in tokens:
            print(t)

    # --- JSON language ---
    def test_json_tokens(self):
        code = """
        {
          "name": "John",
          "age": 30,
          "active": true
        }
        """
        tokens = get_all_visible_tokens(code, "json")
        self.assertTrue(len(tokens) > 5)
        found_name = any("name" in t.toMap()["content"] for t in tokens)
        found_true = any("true" in t.toMap()["content"] for t in tokens)
        self.assertTrue(found_name and found_true)
        for t in tokens:
            print(t)

    # --- Window filtering test (C language) ---
    def test_window_filter(self):
        code = """
        int a = 0;
        int b = 1;
        int c = 2;
        """
        all_tokens = get_all_visible_tokens(code, "c")
        filtered = get_all_visible_tokens(code, "c", x0=0, y0=1, x1=80, y1=1)
        self.assertTrue(len(all_tokens) > len(filtered))
        for t in filtered:
            self.assertTrue(t.y == 1)
            print(t)

if __name__ == "__main__":
    unittest.main(verbosity=2)
    # tt=TestTokenizer()
    # TestTokenizer.setUpClass()
    # tt.test_c_tokens()
    # tt.test_python_tokens()
    # tt.test_javascript_tokens()
    # tt.test_html_tokens()
    # tt.test_json_tokens()
